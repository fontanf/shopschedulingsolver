import * as fs from "fs";
import * as CP from '@scheduleopt/optalcp';
import { writeFile } from "fs/promises";

let params = {
  usage: "Usage: node jobshop.mjs INSTANCE_PATH PARAMETERS_PATH OUTPUT_PATH CERTIFICATE_PATH"
};

async function main()
{
    const instance_path = process.argv[2];
    const parameters_path = process.argv[3];
    const output_path = process.argv[4];
    const certificate_path = process.argv[5];

    const instance_raw = fs.readFileSync(instance_path, "utf-8");
    const instance = JSON.parse(instance_raw);

    let model = new CP.Model("shopscheduling");
    const number_of_jobs = instance.jobs.length;
    const number_of_machines = instance.machines.length;

    ///////////////
    // Variables //
    ///////////////

    // Variables: interval variables for each operation.
    // Stored by machine for the machine non-overlap constraint.
    let machines_alternatives: CP.IntervalVar[][] = [];
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id)
        machines_alternatives[machine_id] = [];
    // Stored by job.
    let jobs_operations: CP.IntervalVar[][] = [];
    // Job alternatives, used to retrieve the solution.
    let jobs_alternatives: CP.IntervalVar[][][] = [];
    for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
        let job = instance.jobs[job_id];
        jobs_operations[job_id] = [];
        jobs_alternatives[job_id] = [];
        for (let operation_id = 0;
                operation_id < job.operations.length;
                ++operation_id) {
            let operation = job.operations[operation_id];
            jobs_alternatives[job_id][operation_id] = [];
            if (operation.alternatives.length == 1) {
                // Not flexible.

                // Create a new operation:
                let alternative = operation.alternatives[0];
                const duration_max = (!instance.operations_arbitrary_order && instance.blocking && operation_id < job.operations.length - 1)?
                    CP.IntervalMax: alternative.processing_time;
                let optalcp_operation = model.intervalVar({
                    length: [alternative.processing_time, duration_max],
                    name: "J" + (job_id) + "O" + (operation_id)
                });
                // Operation requires some machine:
                machines_alternatives[alternative.machine].push(optalcp_operation);
                jobs_operations[job_id].push(optalcp_operation)
                jobs_alternatives[job_id][operation_id].push(optalcp_operation)

            } else {
                // Flexible.

                const optalcp_operation = model.intervalVar()
                jobs_operations[job_id].push(optalcp_operation)
                jobs_alternatives[job_id][operation_id] = []
                const optalcp_alternatives = []
                for (let alternative_id = 0;
                        alternative_id < operation.alternatives.length;
                        ++alternative_id) {
                    let alternative = operation.alternatives[alternative_id];
                    const duration_max = (!instance.operations_arbitrary_order && instance.blocking && operation_id < job.operations.length - 1)?
                        CP.IntervalMax: alternative.processing_time;
                    const optalcp_alternative = model.intervalVar({
                        length: [alternative.processing_time, duration_max],
                        name: "J" + (job_id) + "O" + (operation_id) + "M" + (alternative_id),
                        optional : true,
                    });
                    optalcp_alternatives.push(optalcp_alternative);
                    machines_alternatives[alternative.machine].push(optalcp_alternative);
                    jobs_alternatives[job_id][operation_id].push(optalcp_alternative)
                }
                model.alternative(optalcp_operation, optalcp_alternatives)
            }
        }
    }
    let machines_sequences: CP.SequenceVar[] = [];
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id) {
        const optalcp_sequence = model.sequenceVar(machines_alternatives[machine_id]);
        machines_sequences.push(optalcp_sequence);
    }
    let jobs_sequences: CP.SequenceVar[] = [];
    if (instance.operations_arbitrary_order) {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            const optalcp_sequence = model.sequenceVar(jobs_operations[job_id]);
            jobs_sequences.push(optalcp_sequence);
        }
    }

    // Position variables (for permutation flow shop).
    let jobs_positions: CP.IntVar[] = [];
    if (instance.permutation) {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            jobs_positions.push(model.intVar({ name: "position_" + (job_id) }));
        }
    }

    // Jobs interval variables (for no-wait and blocking).
    // For each job, one interval variable starting at the job start and ending
    // at the job end.
    let jobs_intervals: CP.IntervalVar[] = [];
    if (instance.operations_arbitrary_order && (instance.no_wait || instance.blocking)) {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            let optalcp_interval = model.intervalVar({
                length: [0, CP.IntervalMax],
                name: "J" + (job_id)
            });
            jobs_intervals[job_id] = optalcp_interval;
        }
    }

    // Machines interval variables (for no-idle).
    // For each machine, one interval variable starting at the start time of
    // the first operation scheduled on the machine and ending at the end time
    // of the last operation scheduled on the machine.
    let machines_intervals: CP.IntervalVar[] = [];
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id) {
        let machine = instance.machines[machine_id];
        if (!machine.no_idle)
            continue;
        let optalcp_interval = model.intervalVar({
            length: [0, CP.IntervalMax],
            name: "M" + (machine_id)
        });
        machines_intervals[machine_id] = optalcp_interval;
    }

    // Define jobs ends variables.
    let jobs_ends: CP.IntExpr[] = [];
    if (instance.objective == "Total flow time"
            || instance.objective == "Total tardiness") {
        if (instance.operations_arbitrary_order) {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                let ends: CP.IntExpr[] = [];
                for (let operation_id = 0;
                        operation_id < job.operations.length;
                        ++operation_id) {
                    let operation = jobs_operations[job_id][operation_id];
                    ends.push((operation as CP.IntervalVar).end());
                }
                let job_end = model.max(ends);
                jobs_ends.push(job_end);
            }
        }
    }

    let jobs_tardiness: CP.IntExpr[] = [];
    if (instance.objective == "Total tardiness") {
        if (!instance.operations_arbitrary_order) {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                const due_date = job.due_date;
                let operation = jobs_operations[job_id][job.operations.length - 1];
                let job_tardiness = model.max([
                    model.sum([
                        (operation as CP.IntervalVar).end(),
                        -due_date]),
                    0]);
                jobs_tardiness.push(job_tardiness);
            }
        } else {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                const due_date = job.due_date;
                let job_tardiness = model.max([
                    model.sum([
                        jobs_ends[job_id],
                        -due_date]),
                    0]);
                jobs_tardiness.push(job_tardiness);
            }
        }
    }

    ///////////////
    // Objective //
    ///////////////

    if (instance.objective == "Makespan") {
        // Objective: minimize the makespan.
        let ends: CP.IntExpr[] = [];
        if (!instance.operations_arbitrary_order) {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                let operation = jobs_operations[job_id][job.operations.length - 1];
                ends.push((operation as CP.IntervalVar).end());
            }
        } else {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                for (let operation_id = 0;
                        operation_id < job.operations.length;
                        ++operation_id) {
                    let operation = jobs_operations[job_id][operation_id];
                    ends.push((operation as CP.IntervalVar).end());
                }
            }
        }
        let makespan = model.max(ends);
        makespan.minimize();
    } else if (instance.objective == "Total flow time") {
        // Objective: minimize the total weighted flow time.
        let weighted_flow_times: CP.IntExpr[] = [];
        if (!instance.operations_arbitrary_order) {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                let operation = jobs_operations[job_id][job.operations.length - 1];
                weighted_flow_times.push(model.times(
                    (operation as CP.IntervalVar).end(),
                    job.weight));
            }
        } else {
            for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
                let job = instance.jobs[job_id];
                weighted_flow_times.push(model.times(
                    jobs_ends[job_id],
                    job.weight));
            }
        }
        let total_weighted_flow_time = model.sum(weighted_flow_times);
        total_weighted_flow_time.minimize();
    } else if (instance.objective == "Total tardiness") {
        // Objective: minimize the total weighted tardiness.
        let weighted_tardiness: CP.IntExpr[] = [];
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            let job = instance.jobs[job_id];
            weighted_tardiness.push(model.times(
                jobs_tardiness[job_id],
                job.weight));
        }
        let total_weighted_tardiness = model.sum(weighted_tardiness);
        total_weighted_tardiness.minimize();
    }

    /////////////////
    // Constraints //
    /////////////////

    if (!instance.operations_arbitrary_order) {
        // Constraints: precedence constraints between operations of a job (job shop).
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            let job = instance.jobs[job_id];
            for (let operation_id = 1;
                    operation_id < job.operations.length;
                    ++operation_id) {
                 let operation = jobs_operations[job_id][operation_id];
                 let operation_prev = jobs_operations[job_id][operation_id - 1];
                 if (instance.no_wait) {
                     operation_prev.endAtStart(operation);
                 } else if (instance.blocking) {
                     operation_prev.endAtStart(operation);
                 } else {
                     operation_prev.endBeforeStart(operation);
                 }
            }
        }
    } else {
        // Constraints: operations of each job must not overlap.
        for (let job_id = 0; job_id < number_of_jobs; ++job_id)
            model.noOverlap(jobs_sequences[job_id]);
    }

    // Constraints: operations on each machine must not overlap.
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id)
        model.noOverlap(machines_sequences[machine_id]);

    // Permutation constraints.
    if (instance.permutation) {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            for (let machine_id = 0; machine_id < number_of_machines; ++machine_id) {
                let pos = model.position(machines_alternatives[machine_id][job_id], machines_sequences[machine_id]);
                model.constraint(jobs_positions[job_id].eq(pos));
            }
        }
    }

    // No-wait and blocking constraints for open shop.
    if (instance.operations_arbitrary_order && (instance.no_wait || instance.blocking)) {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            let job = instance.jobs[job_id];
            let starts: CP.IntExpr[] = [];
            let ends: CP.IntExpr[] = [];
            let lengths: CP.IntExpr[] = [];
            for (let operation_id = 0;
                    operation_id < job.operations.length;
                    ++operation_id) {
                let operation = jobs_operations[job_id][operation_id];
                starts.push((operation as CP.IntervalVar).start());
                ends.push((operation as CP.IntervalVar).end());
                lengths.push((operation as CP.IntervalVar).length());
            }
            let job_start = model.min(starts);
            let job_end = model.max(ends);
            let job_length = model.sum(lengths);
            model.constraint(jobs_intervals[job_id].start().eq(job_start));
            model.constraint(jobs_intervals[job_id].end().eq(job_end));
            model.constraint(jobs_intervals[job_id].length().eq(job_length));
        }
    }

    // No-idle constraints for open shop.
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id) {
        let machine = instance.machines[machine_id];
        if (!machine.no_idle)
            continue;
        let starts: CP.IntExpr[] = [];
        let ends: CP.IntExpr[] = [];
        let lengths: CP.IntExpr[] = [];
        for (let operation_id = 0;
                operation_id < machines_alternatives.length;
                ++operation_id) {
            let operation = machines_alternatives[machine_id][operation_id];
            starts.push((operation as CP.IntervalVar).start());
            ends.push((operation as CP.IntervalVar).end());
            lengths.push((operation as CP.IntervalVar).length());
        }
        let machine_start = model.min(starts);
        let machine_end = model.max(ends);
        let machine_length = model.sum(lengths);
        model.constraint(machines_intervals[machine_id].start().eq(machine_start));
        model.constraint(machines_intervals[machine_id].end().eq(machine_end));
        model.constraint(machines_intervals[machine_id].length().eq(machine_length));
    }

    ////////////////
    // Parameters //
    ////////////////

    const parameters_raw = fs.readFileSync(parameters_path, "utf-8");
    const parameters = JSON.parse(parameters_raw);

    const defaults = {
        "FDS": {
            searchType: "FDS",
            noOverlapPropagationLevel: 4,
            cumulPropagationLevel: 3,
            reservoirPropagationLevel: 2,
        },
        "FDSLB": {
            searchType: "FDS",
            noOverlapPropagationLevel: 4,
            cumulPropagationLevel: 3,
            reservoirPropagationLevel: 2,
            FDSLBStrategy: "Split"
        },
        "LNS": { searchType: "LNS" },
    } as const;

    type WorkerType = keyof typeof defaults;

    const cp_parameters: CP.Parameters = {
        ...(typeof parameters.TimeLimit === "number"?
            { timeLimit: parameters.TimeLimit }: {}),
        relativeGapTolerance: 0.0,
        workers: (["FDS", "FDSLB", "LNS", "LNS"] as WorkerType[]).map(v => defaults[v]),
        usePrecedenceEnergy : 1,
        packPropagationLevel : 2,
    };

    ///////////
    // Solve //
    ///////////

    let solve_result = await CP.solve(model, cp_parameters);

    //////////////////////
    // Retrieve results //
    //////////////////////

    // Retrieve solution.
    if (solve_result.bestSolution) {
        interface OperationSolution {
            job_id: number;
            operation_id: number;
            alternative_id: number;
            start: number | null;
        }

        const solution: { operations: OperationSolution[] } = {
            operations: [],
        };
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            let job = instance.jobs[job_id];
            for (let operation_id = 0;
                    operation_id < job.operations.length;
                    ++operation_id) {
                let operation = job.operations[operation_id];
                for (let alternative_id = 0;
                        alternative_id < operation.alternatives.length;
                        ++alternative_id) {
                    if (solve_result.bestSolution!.getStart(jobs_alternatives[job_id][operation_id][alternative_id]) == null)
                        continue;
                    let solution_operation: OperationSolution = {
                        job_id,
                        operation_id,
                        alternative_id,
                        start: solve_result.bestSolution!.getStart(jobs_alternatives[job_id][operation_id][alternative_id])
                    };
                    solution.operations.push(solution_operation)
                }
            }
        }
        const jsonString = JSON.stringify(solution, null, 2);
        fs.writeFileSync(certificate_path, jsonString, "utf-8");
    }

    const output = {
        bound: solve_result.lowerBoundHistory[solve_result.lowerBoundHistory.length - 1].value,
    };
    const jsonString = JSON.stringify(output, null, 2);
    fs.writeFileSync(output_path, jsonString, "utf-8");
}

main();
