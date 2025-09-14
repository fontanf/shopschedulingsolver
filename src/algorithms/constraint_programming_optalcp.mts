import * as fs from "fs";
import * as CP from '@scheduleopt/optalcp';

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
    const number_of_machines = instance.number_of_machines;

    ///////////////
    // Variables //
    ///////////////

    // Variables: interval variables for each operation.
    // Stored by machine.
    let machines: CP.IntervalVar[][] = [];
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id)
        machines[machine_id] = [];
    // Stored by job.
    let jobs: CP.IntervalVar[][] = [];
    for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
        jobs[job_id] = []
        for (let operation_id = 0;
                operation_id < instance.jobs[job_id].operations.length;
                ++operation_id) {
            // Create a new operation:
            const machine_id = instance.jobs[job_id].operations[operation_id].machines[0].machine;
            const duration = instance.jobs[job_id].operations[operation_id].machines[0].processing_time;
            let operation = model.intervalVar({
                length: duration,
                name: "J" + (job_id) + "O" + (operation_id)
            });
            // Operation requires some machine:
            machines[machine_id].push(operation);
            jobs[job_id].push(operation)
        }
    }

    ///////////////
    // Objective //
    ///////////////

    // Objective: minimize the makespan.
    let ends: CP.IntExpr[] = [];
    if (!instance.operations_arbitrary_order) {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            let operation = jobs[job_id][instance.jobs[job_id].operations.length - 1];
            ends.push((operation as CP.IntervalVar).end());
        }
    } else {
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            for (let operation_id = 0;
                    operation_id < instance.jobs[job_id].operations.length;
                    ++operation_id) {
                let operation = jobs[job_id][operation_id];
                ends.push((operation as CP.IntervalVar).end());
            }
        }
    }
    if (instance.objective == "Makespan") {
        let makespan = model.max(ends);
        makespan.minimize();
    } else if (instance.objective == "Total flow time") {
    } else if (instance.objective == "Total tardiness") {
    }

    /////////////////
    // Constraints //
    /////////////////

    if (!instance.operations_arbitrary_order) {
        // Constraints: precedence constraints between operations of a job (job shop).
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            for (let operation_id = 1;
                    operation_id < instance.jobs[job_id].operations.length;
                    ++operation_id) {
                 let operation = jobs[job_id][operation_id];
                 let operation_prev = jobs[job_id][operation_id - 1];
                 if (!instance.no_wait) {
                     operation_prev.endBeforeStart(operation);
                 } else {
                     operation_prev.endAtStart(operation);
                 }
            }
        }
    } else {
        // Constraints: operations of each job must not overlap.
        for (let job_id = 0; job_id < number_of_jobs; ++job_id)
            model.noOverlap(jobs[job_id]);
    }

    // Constraints: operations on each machine must not overlap.
    for (let machine_id = 0; machine_id < number_of_machines; ++machine_id)
        model.noOverlap(machines[machine_id]);

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
        timeLimit: parameters.TimeLimit,
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
            operation_machine_id: number;
            start: number | null;
        }

        const solution: { operations: OperationSolution[] } = {
            operations: [],
        };
        for (let job_id = 0; job_id < number_of_jobs; ++job_id) {
            for (let operation_id = 0;
                    operation_id < instance.jobs[job_id].operations.length;
                    ++operation_id) {
                let operation_machine_id = 0;
                let solution_operation: OperationSolution = {
                    job_id,
                    operation_id,
                    operation_machine_id,
                    start: solve_result.bestSolution!.getStart(jobs[job_id][operation_id])
                };
                solution.operations.push(solution_operation)
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
