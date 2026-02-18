#include "shopschedulingsolver/algorithms/milp_disjunctive.hpp"

#include "mathoptsolverscmake/milp.hpp"

using namespace shopschedulingsolver;

namespace
{

struct Model
{
    /** Model. */
    mathoptsolverscmake::MilpModel model;

    /**
     * Let (j1, o1) and (j2, o2) be two operation of the same machine
     * y_{j1, o1, j2, o2} = 1 iff o1 is scheduled before o2.
     */
    std::vector<std::vector<std::vector<int>>> y;

    /**
     * x_{j, o, k} == 1 iff alternative k is used for operation of of job j.
     *
     * These variables are only required in the flexible case.
     */
    std::vector<std::vector<std::vector<int>>> x;

    /**
     * z_{j, o1, o2} = 1 iff o1 is scheduled before o2
     *
     * These variables are only required for open shop; otherwise, the order
     * is already known.
     */
    std::vector<std::vector<std::vector<int>>> z;

    /** C_{j, o} the completion time of operation (j, o). */
    std::vector<std::vector<int>> co;

    /**
     * Ck_{j, o, k} the completion time of the k-th alternative of operation o of
     * job j.
     *
     * 0 if the alternative is not used.
     *
     * This is only required in the flexible case.
     */
    std::vector<std::vector<std::vector<int>>> ck;

    /**
     * p_{j, o} the duration that operation o stays on its machine.
     *
     * These variables are only required in the flexible case and in the
     * blocking case.
     */
    std::vector<std::vector<int>> p;

    /**
     * psum_{j} the sum of the durations that each operation of job j stays on
     * its machine.
     *
     * These variables are only required in the blocking and flexible case for
     * open shop.
     */
    std::vector<int> psum;

    /** Makespan. */
    int cmax = -1;

    /**
     * C_{j} the completion time of job j.
     *
     * These variables are only required in the open shop case with total
     * completion time objective.
     */
    std::vector<int> cj;

    /**
     * t_j the tardiness of job j.
     *
     * These variables are only required with total weighted tardiness
     * objective.
     */
    std::vector<int> t;

    /**
     * s_j the starting time of job j.
     *
     * These variables are only required in the no-wait and blocking open shop
     * case.
     */
    std::vector<int> s;

    /**
     * sm_i the starting time of machine i.
     *
     * These variables are only required in the no-idle case.
     */
    std::vector<int> sm;

    /**
     * pmsum_{i} the sum of the durations of the operations scheduled on machine
     * i.
     *
     * These variables are only required in the flexible no-idle case for.
     */
    std::vector<int> pmsum;
};

Model create_milp_model(
        const Instance& instance)
{
    Model model;

    /////////////////////////////
    // Variables and objective //
    /////////////////////////////

    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    // Variables y.
    model.y = std::vector<std::vector<std::vector<int>>>(instance.number_of_machines());
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        model.y[machine_id] = std::vector<std::vector<int>>(machine.operations.size());
        for (JobId pos = 0; pos < machine.operations.size(); ++pos) {
            const MachineOperation& machine_operation = machine.operations[pos];
            model.y[machine_id][pos] = std::vector<int>(pos);
            for (JobId pos_2 = 0; pos_2 < pos; ++pos_2) {
                model.y[machine_id][pos][pos_2] = model.model.variables_lower_bounds.size();
                model.model.variables_lower_bounds.push_back(0);
                model.model.variables_upper_bounds.push_back(1);
                model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
                model.model.objective_coefficients.push_back(0);
                model.model.variables_names.push_back("y_{" + std::to_string(machine_id) + "," + std::to_string(pos) + "," + std::to_string(pos_2) + "}");
            }
        }
    }

    // Variables x.
    if (instance.flexible()) {
        model.x = std::vector<std::vector<std::vector<int>>>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.x[job_id] = std::vector<std::vector<int>>(job.operations.size());
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                model.x[job_id][operation_id] = std::vector<int>(operation.alternatives.size());
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    model.x[job_id][operation_id][alternative_id] = model.model.variables_lower_bounds.size();
                    model.model.variables_lower_bounds.push_back(0);
                    model.model.variables_upper_bounds.push_back(1);
                    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
                    model.model.objective_coefficients.push_back(0);
                    model.model.variables_names.push_back("x_{" + std::to_string(job_id) + "," + std::to_string(operation_id) + "," + std::to_string(alternative_id) + "}");
                }
            }
        }
    }

    // Variables z.
    if (instance.operations_arbitrary_order()) {
        model.z = std::vector<std::vector<std::vector<int>>>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.z[job_id] = std::vector<std::vector<int>>(job.operations.size());
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                model.z[job_id][operation_id] = std::vector<int>(operation_id);
                for (OperationId operation_2_id = 0;
                        operation_2_id < operation_id;
                        ++operation_2_id) {
                    model.z[job_id][operation_id][operation_2_id] = model.model.variables_lower_bounds.size();
                    model.model.variables_lower_bounds.push_back(0);
                    model.model.variables_upper_bounds.push_back(1);
                    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
                    model.model.objective_coefficients.push_back(0);
                    model.model.variables_names.push_back("z_{" + std::to_string(job_id) + "," + std::to_string(operation_id) + "," + std::to_string(operation_2_id) + "}");
                }
            }
        }
    }

    // Varibles c.
    model.co = std::vector<std::vector<int>>(instance.number_of_jobs());
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        const Job& job = instance.job(job_id);

        Time lb = 0;
        model.co[job_id] = std::vector<int>(job.operations.size());
        for (OperationId operation_id = 0;
                operation_id < job.operations.size();
                ++operation_id) {
            const Operation& operation = job.operations[operation_id];

            Time pmin = operation.alternatives[0].processing_time;
            for (AlternativeId alternative_id = 0;
                    alternative_id < (AlternativeId)operation.alternatives.size();
                    ++alternative_id) {
                const Alternative& alternative = operation.alternatives[alternative_id];
                pmin = (std::min)(pmin, alternative.processing_time);
            }
            if (!instance.operations_arbitrary_order()) {
                lb += pmin;
            } else {
                lb = pmin;
            }

            model.co[job_id][operation_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(lb);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            if (instance.objective() == Objective::TotalFlowTime
                    && !instance.operations_arbitrary_order()
                    && operation_id == (JobId)job.operations.size() - 1) {
                model.model.objective_coefficients.push_back(job.weight);
            } else {
                model.model.objective_coefficients.push_back(0);
            }
            model.model.variables_names.push_back("co_{" + std::to_string(job_id) + "," + std::to_string(operation_id) + "}");
        }
    }

    // Variable ck.
    if (instance.flexible()) {
        model.ck = std::vector<std::vector<std::vector<int>>>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.ck[job_id] = std::vector<std::vector<int>>(job.operations.size());
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                model.ck[job_id][operation_id] = std::vector<int>(operation.alternatives.size());
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    model.ck[job_id][operation_id][alternative_id] = model.model.variables_lower_bounds.size();
                    model.model.variables_lower_bounds.push_back(0);
                    model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
                    model.model.objective_coefficients.push_back(0);
                    model.model.variables_names.push_back("ck_{" + std::to_string(job_id) + "," + std::to_string(operation_id) + "," + std::to_string(alternative_id) + "}");
                }
            }
        }
    }

    // Variables p.
    if (instance.flexible() || instance.blocking()) {
        model.p = std::vector<std::vector<int>>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.p[job_id] = std::vector<int>(job.operations.size());
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                // Compute pmin.
                Time pmin = operation.alternatives[0].processing_time;
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    pmin = (std::min)(pmin, alternative.processing_time);
                }

                model.p[job_id][operation_id] = model.model.variables_lower_bounds.size();
                model.model.variables_lower_bounds.push_back(pmin);
                model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
                model.model.objective_coefficients.push_back(0);
                model.model.variables_names.push_back("p_{" + std::to_string(job_id) + "," + std::to_string(operation_id) + "}");
            }
        }
    }

    // Variables psum.
    if ((instance.blocking() || instance.flexible()) && instance.operations_arbitrary_order()) {
        model.psum = std::vector<int>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.psum[job_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
            model.model.variables_names.push_back("psum_{" + std::to_string(job_id) + "}");
        }
    }

    // cmax.
    if (instance.objective() == Objective::Makespan) {
        model.cmax = model.model.variables_lower_bounds.size();
        model.model.variables_lower_bounds.push_back(0);
        model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
        model.model.objective_coefficients.push_back(1);
        model.model.variables_names.push_back("cmax");
    }

    // Varibles cj.
    if (instance.objective() == Objective::TotalFlowTime
            && instance.operations_arbitrary_order()) {
        model.cj = std::vector<int>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.cj[job_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(job.weight);
            model.model.variables_names.push_back("cj_{" + std::to_string(job_id) + "}");
        }
    }

    // Varibles t.
    if (instance.objective() == Objective::TotalTardiness) {
        model.t = std::vector<int>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.t[job_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(job.weight);
            model.model.variables_names.push_back("t_{" + std::to_string(job_id) + "}");
        }
    }

    // Varibles s.
    if ((instance.no_wait() || instance.blocking())
            && instance.operations_arbitrary_order()) {
        model.s = std::vector<int>(instance.number_of_jobs());
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            model.s[job_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
            model.model.variables_names.push_back("s_{" + std::to_string(job_id) + "}");
        }
    }

    // Variables sm.
    if (instance.mixed_no_idle()) {
        model.sm = std::vector<int>(instance.number_of_machines());
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            const Machine& machine = instance.machine(machine_id);
            if (!machine.no_idle)
                continue;
            model.sm[machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
            model.model.variables_names.push_back("sm_{" + std::to_string(machine_id) + "}");
        }
    }

    // Variables pmsum.
    if (instance.mixed_no_idle() && instance.flexible()) {
        model.pmsum = std::vector<int>(instance.number_of_machines());
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            const Machine& machine = instance.machine(machine_id);
            if (!machine.no_idle)
                continue;
            model.pmsum[machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
            model.model.variables_names.push_back("pmsum_{" + std::to_string(machine_id) + "}");
        }
    }

    /////////////////
    // Constraints //
    /////////////////

    // Makespan definition.
    if (instance.objective() == Objective::Makespan) {
        if (instance.operations_arbitrary_order()) {
            // Cmax >= C_{j, o}
            // <=>
            // 0 <= Cmax - C_{j, o} <= inf
            for (JobId job_id = 0;
                    job_id < instance.number_of_jobs();
                    ++job_id) {
                const Job& job = instance.job(job_id);
                for (OperationId operation_id = 0;
                        operation_id < (OperationId)job.operations.size();
                        ++operation_id) {
                    const Operation& operation = job.operations[operation_id];
                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.cmax);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                    model.model.elements_coefficients.push_back(-1.0);

                    model.model.constraints_lower_bounds.push_back(0);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        } else {
            // Cmax >= C_{j, o_last}
            // <=>
            // 0 <= Cmax - C_{j, o_last} <= inf
            for (JobId job_id = 0;
                    job_id < instance.number_of_jobs();
                    ++job_id) {
                const Job& job = instance.job(job_id);
                OperationId operation_id = job.operations.size() - 1;
                const Operation& operation = job.operations[operation_id];
                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.cmax);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);

                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // Definition of job completion times.
    if (instance.objective() == Objective::TotalFlowTime
            && instance.operations_arbitrary_order()) {
        // C_j >= C_{j, o}
        // <=>
        // 0 <= C_j - C_{j, o} <= inf
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);

            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.cj[job_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);

                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // Definition of tardiness.
    if (instance.objective() == Objective::TotalTardiness) {
        if (!instance.operations_arbitrary_order()) {
            // T_j >= C_{j, o_last} - d_j
            // <=>
            // -inf <= C_{j, o_last} - T_j <= d_j
            for (JobId job_id = 0;
                    job_id < instance.number_of_jobs();
                    ++job_id) {
                const Job& job = instance.job(job_id);
                OperationId operation_id = job.operations.size() - 1;
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.t[job_id]);
                model.model.elements_coefficients.push_back(-1.0);

                model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
                model.model.constraints_upper_bounds.push_back(job.due_date);
            }
        } else {
            // T_j >= C_{j, o} - d_j
            // <=>
            // -inf <= C_{j, o} - d_j - T_j <= d_j
            for (JobId job_id = 0;
                    job_id < instance.number_of_jobs();
                    ++job_id) {
                const Job& job = instance.job(job_id);

                for (OperationId operation_id = 0;
                        operation_id < (OperationId)job.operations.size();
                        ++operation_id) {
                    const Operation& operation = job.operations[operation_id];

                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.t[job_id]);
                    model.model.elements_coefficients.push_back(-1.0);

                    model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
                    model.model.constraints_upper_bounds.push_back(job.due_date);
                }
            }
        }
    }

    // At least one alternative must be selected for each operation.
    if (instance.flexible()) {
        // \sum_{j, o} x_{j, o, k} = 1
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    model.model.elements_variables.push_back(model.x[job_id][operation_id][alternative_id]);
                    model.model.elements_coefficients.push_back(1);
                }
                model.model.constraints_lower_bounds.push_back(1);
                model.model.constraints_upper_bounds.push_back(1);
            }
        }
    }

    // Definition of p.
    if (instance.flexible()) {
        // p_{j, o} = \sum p_{j, o, k} x_{j, o, k}
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                model.model.elements_variables.push_back(model.p[job_id][operation_id]);
                model.model.elements_coefficients.push_back(1);
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    model.model.elements_variables.push_back(model.x[job_id][operation_id][alternative_id]);
                    model.model.elements_coefficients.push_back(-alternative.processing_time);
                }
                model.model.constraints_lower_bounds.push_back(0);
                if (!instance.blocking()) {
                    model.model.constraints_upper_bounds.push_back(0);
                } else {
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        }
    }

    // Force ck_{j, o, k} to be 0 if alternative k is not the selected
    // alternative for operation o of job j.
    Time m = 0;
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        const Job& job = instance.job(job_id);
        for (OperationId operation_id = 0;
                operation_id < (OperationId)job.operations.size();
                ++operation_id) {
            const Operation& operation = job.operations[operation_id];
            Time pmax = 0;
            for (AlternativeId alternative_id = 0;
                    alternative_id < (AlternativeId)operation.alternatives.size();
                    ++alternative_id) {
                const Alternative& alternative = operation.alternatives[alternative_id];
                pmax = (std::max)(pmax, alternative.processing_time);
            }
            m += pmax;
        }
    }
    if (instance.flexible()) {
        // ck_{j, o, k} <= M x_{j, o, k}
        // <=>
        // ck_{j, o, k} - M x_{j, o, k} <= 0
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    model.model.constraints_starts.push_back(model.model.elements_variables.size());
                    model.model.elements_variables.push_back(model.ck[job_id][operation_id][alternative_id]);
                    model.model.elements_coefficients.push_back(1);
                    model.model.elements_variables.push_back(model.x[job_id][operation_id][alternative_id]);
                    model.model.elements_coefficients.push_back(-m);
                    model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
                    model.model.constraints_upper_bounds.push_back(0);
                }
            }
        }
    }

    // Link between co and ck.
    if (instance.flexible()) {
        // c_{j, o} = \sum ck_{j, o, k}
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(1);
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    model.model.elements_variables.push_back(model.ck[job_id][operation_id][alternative_id]);
                    model.model.elements_coefficients.push_back(-1);
                }
                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(0);
            }
        }
    }

    // Disjunction on machines.
    // C_{j1, o1} >= C_{j2, o2} + p_{j1, o1} - M y_{j1, o1, j2, o2}
    //                                       - M (1 - x_{j1, o1, k1})
    //                                       - M (1 - x_{j2, o2, k2})
    // C_{j2, o2} >= C_{j1, o1} + p_{j2, o2} - M (1 - y_{j1, o1, j2, o2})
    //                                       - M (1 - x_{j1, o1, k1})
    //                                       - M (1 - x_{j2, o2, k2})
    // <=>
    // p_{j2, o1}     - 2M <= C_{j1, o1} - C_{j2, o2} + M y_{j1, o1, j2, o2}
    //                                                - M x_{j1, o1, k1}
    //                                                - M x_{j2, o2, k2}
    // p_{j2, o2} - M - 2M <= C_{j1, o2} - C_{j2, o1} - M y_{j1, o1, j2, o2})
    //                                                - M x_{j1, o1, k1}
    //                                                - M x_{j2, o2, k2}
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        for (JobId pos = 0; pos < machine.operations.size(); ++pos) {
            const MachineOperation& machine_operation = machine.operations[pos];
            const Job& job = instance.job(machine_operation.job_id);
            const Operation& operation = job.operations[machine_operation.operation_id];
            const Alternative& alternative = operation.alternatives[machine_operation.alternative_id];
            for (JobId pos_2 = 0; pos_2 < pos; ++pos_2) {
                const MachineOperation& machine_operation_2 = machine.operations[pos_2];
                const Job& job_2 = instance.job(machine_operation_2.job_id);
                const Operation& operation_2 = job_2.operations[machine_operation_2.operation_id];
                const Alternative& alternative_2 = operation_2.alternatives[machine_operation_2.alternative_id];

                {
                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.co[machine_operation.job_id][machine_operation.operation_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.co[machine_operation_2.job_id][machine_operation_2.operation_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.elements_variables.push_back(model.y[machine_id][pos][pos_2]);
                    model.model.elements_coefficients.push_back(m);
                    if (instance.blocking()) {
                        model.model.elements_variables.push_back(model.p[machine_operation.job_id][machine_operation.operation_id]);
                        model.model.elements_coefficients.push_back(-1);
                    }
                    if (instance.flexible()) {
                        model.model.elements_variables.push_back(model.x[machine_operation.job_id][machine_operation.operation_id][machine_operation.alternative_id]);
                        model.model.elements_coefficients.push_back(-m);
                        model.model.elements_variables.push_back(model.x[machine_operation_2.job_id][machine_operation_2.operation_id][machine_operation_2.alternative_id]);
                        model.model.elements_coefficients.push_back(-m);
                    }

                    double lower_bound = 0;
                    if (!instance.blocking())
                        lower_bound += alternative.processing_time;
                    if (instance.flexible())
                        lower_bound -= 2 * m;
                    model.model.constraints_lower_bounds.push_back(lower_bound);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }

                {
                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.co[machine_operation_2.job_id][machine_operation_2.operation_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.co[machine_operation.job_id][machine_operation.operation_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.elements_variables.push_back(model.y[machine_id][pos][pos_2]);
                    model.model.elements_coefficients.push_back(-m);
                    if (instance.blocking()) {
                        model.model.elements_variables.push_back(model.p[machine_operation_2.job_id][machine_operation_2.operation_id]);
                        model.model.elements_coefficients.push_back(-1);
                    }
                    if (instance.flexible()) {
                        model.model.elements_variables.push_back(model.x[machine_operation.job_id][machine_operation.operation_id][machine_operation.alternative_id]);
                        model.model.elements_coefficients.push_back(-m);
                        model.model.elements_variables.push_back(model.x[machine_operation_2.job_id][machine_operation_2.operation_id][machine_operation_2.alternative_id]);
                        model.model.elements_coefficients.push_back(-m);
                    }

                    double lower_bound = -m;
                    if (!instance.blocking())
                        lower_bound += alternative_2.processing_time;
                    if (instance.flexible())
                        lower_bound -= 2 * m;
                    model.model.constraints_lower_bounds.push_back(lower_bound);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        }
    }

    // Disjunction on jobs.
    if (instance.operations_arbitrary_order()) {
        // C_{j, o1} >= C_{j, o2} + p_{j, o1} - M z_{j, o1, o2}
        // C_{j, o2} >= C_{j, o1} + p_{j, o2} - M (1 - z_{j, o1, o2})
        // <=>
        // p_{j, o1} <= C_{j, o1} - C_{j, o2} + M z_{j, o1, o2}
        // p_{j, o2} - M <= C_{j, o2} - C_{j, o1} - M z_{j, o1, o2})
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                for (OperationId operation_2_id = 0;
                        operation_2_id < operation_id;
                        ++operation_2_id) {
                    const Operation& operation_2 = job.operations[operation_2_id];

                    {
                        model.model.constraints_starts.push_back(model.model.elements_variables.size());

                        model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                        model.model.elements_coefficients.push_back(1.0);
                        model.model.elements_variables.push_back(model.co[job_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(-1.0);
                        model.model.elements_variables.push_back(model.z[job_id][operation_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(m);
                        if (instance.flexible() || instance.blocking()) {
                            model.model.elements_variables.push_back(model.p[job_id][operation_id]);
                            model.model.elements_coefficients.push_back(-1);
                        }

                        if (instance.flexible() || instance.blocking()) {
                            model.model.constraints_lower_bounds.push_back(0);
                            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                        } else {
                            model.model.constraints_lower_bounds.push_back(operation.alternatives[0].processing_time);
                            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                        }
                    }

                    {
                        model.model.constraints_starts.push_back(model.model.elements_variables.size());

                        model.model.elements_variables.push_back(model.co[job_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(1.0);
                        model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                        model.model.elements_coefficients.push_back(-1.0);
                        model.model.elements_variables.push_back(model.z[job_id][operation_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(-m);
                        if (instance.flexible() || instance.blocking()) {
                            model.model.elements_variables.push_back(model.p[job_id][operation_2_id]);
                            model.model.elements_coefficients.push_back(-1);
                        }

                        if (instance.flexible() || instance.blocking()) {
                            model.model.constraints_lower_bounds.push_back(-m);
                            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                        } else {
                            model.model.constraints_lower_bounds.push_back(operation_2.alternatives[0].processing_time - m);
                            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                        }
                    }
                }
            }
        }
    } else {
        // C_{j, o2} >= C_{j, o1} + p_{j, o2}
        // <=>
        // p_{j, o2} <= C_{j, o2} - C_{j, o1}
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size() - 1;
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                const Operation& operation_2 = job.operations[operation_id + 1];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.co[job_id][operation_id + 1]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);
                if (instance.flexible() || instance.blocking()) {
                    model.model.elements_variables.push_back(model.p[job_id][operation_id + 1]);
                    model.model.elements_coefficients.push_back(-1.0);
                }

                if ((instance.flexible() && instance.no_wait()) || instance.blocking()) {
                    model.model.constraints_lower_bounds.push_back(0);
                    model.model.constraints_upper_bounds.push_back(0);
                } else if (instance.flexible()) {
                    model.model.constraints_lower_bounds.push_back(0);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                } else if (instance.no_wait()) {
                    model.model.constraints_lower_bounds.push_back(operation_2.alternatives[0].processing_time);
                    model.model.constraints_upper_bounds.push_back(operation_2.alternatives[0].processing_time);
                } else {
                    model.model.constraints_lower_bounds.push_back(operation_2.alternatives[0].processing_time);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        }
    }

    // Lower bound of C_{j, 0}.
    // These constraints are only needed when operations processing times are
    // variables (flexible). Otheriwse, this is handled by the lower bound of
    // C_{j, o} directly.
    //
    // C_{j, o} >= p_{j, O}
    // 0 <= C_{j, o} - p_{j, o}
    if (instance.flexible()) {
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.p[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);
                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // Definition of pmsum.
    if (instance.mixed_no_idle() && instance.flexible()) {
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            const Machine& machine = instance.machine(machine_id);
            if (!machine.no_idle)
                continue;

            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.elements_variables.push_back(model.pmsum[machine_id]);
            model.model.elements_coefficients.push_back(1.0);
            for (OperationId machine_operation_id = 0;
                    machine_operation_id < (OperationId)machine.operations.size();
                    ++machine_operation_id) {
                const MachineOperation& machine_operation = machine.operations[machine_operation_id];
                const Job& job = instance.job(machine_operation.job_id);
                const Operation& operation = job.operations[machine_operation.operation_id];
                const Alternative& alternative = operation.alternatives[machine_operation.alternative_id];
                model.model.elements_variables.push_back(model.x[machine_operation.job_id][machine_operation.operation_id][machine_operation.alternative_id]);
                model.model.elements_coefficients.push_back(-alternative.processing_time);
            }
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // No-idle constraints.
    if (instance.mixed_no_idle()) {
        if (!instance.flexible()) {
            // Sm_i <= C_{j, o} - p_{j, o}
            // <=>
            // p_{j, o} <= C_{j, o} - Sm_i
            //
            // Sm_i >= C_{j, o} - \sum_{o'} p_{j, o'}
            // <=>
            // C_{j, o} - Sm_i <= \sum_{o'} p_{j, o'}
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                const Machine& machine = instance.machine(machine_id);
                if (!machine.no_idle)
                    continue;

                Time processing_time_sum = 0;
                for (OperationId machine_operation_id = 0;
                        machine_operation_id < (OperationId)machine.operations.size();
                        ++machine_operation_id) {
                    const MachineOperation& machine_operation = machine.operations[machine_operation_id];
                    const Job& job = instance.job(machine_operation.job_id);
                    const Operation& operation = job.operations[machine_operation.operation_id];
                    const Alternative& alternative = operation.alternatives[machine_operation.alternative_id];
                    processing_time_sum += alternative.processing_time;
                }

                for (OperationId machine_operation_id = 0;
                        machine_operation_id < (OperationId)machine.operations.size();
                        ++machine_operation_id) {
                    const MachineOperation& machine_operation = machine.operations[machine_operation_id];
                    const Job& job = instance.job(machine_operation.job_id);
                    const Operation& operation = job.operations[machine_operation.operation_id];
                    const Alternative& alternative = operation.alternatives[machine_operation.alternative_id];

                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.co[machine_operation.job_id][machine_operation.operation_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.sm[machine_id]);
                    model.model.elements_coefficients.push_back(-1.0);

                    model.model.constraints_lower_bounds.push_back(alternative.processing_time);
                    model.model.constraints_upper_bounds.push_back(processing_time_sum);
                }
            }
        } else {
            // Sm_i <= Ck_{j, o, k} - p_{j, o, k} + M x_{j, o, k}
            // <=>
            // Ck_{j, o, k} + M x_{j, o, k} - Sm_i >=  p_{j, o, k}
            //
            // Sm_i >= C_{j, o, k} - \sum_{o'} p_{j, o'}
            // <=>
            // Sm_i - C_{j, o, k} - pmsum_i >= 0
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                const Machine& machine = instance.machine(machine_id);
                if (!machine.no_idle)
                    continue;

                for (OperationId machine_operation_id = 0;
                        machine_operation_id < (OperationId)machine.operations.size();
                        ++machine_operation_id) {
                    const MachineOperation& machine_operation = machine.operations[machine_operation_id];
                    const Job& job = instance.job(machine_operation.job_id);
                    const Operation& operation = job.operations[machine_operation.operation_id];
                    const Alternative& alternative = operation.alternatives[machine_operation.alternative_id];

                    model.model.constraints_starts.push_back(model.model.elements_variables.size());
                    model.model.elements_variables.push_back(model.ck[machine_operation.job_id][machine_operation.operation_id][machine_operation.alternative_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.x[machine_operation.job_id][machine_operation.operation_id][machine_operation.alternative_id]);
                    model.model.elements_coefficients.push_back(m);
                    model.model.elements_variables.push_back(model.sm[machine_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.constraints_lower_bounds.push_back(alternative.processing_time);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());

                    model.model.constraints_starts.push_back(model.model.elements_variables.size());
                    model.model.elements_variables.push_back(model.sm[machine_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.ck[machine_operation.job_id][machine_operation.operation_id][machine_operation.alternative_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.elements_variables.push_back(model.pmsum[machine_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.constraints_lower_bounds.push_back(0);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        }
    }

    // Definition of psum.
    if ((instance.blocking() || instance.flexible()) && instance.operations_arbitrary_order()) {
        // psum_j == \sum_{o} p_{j, o}
        // <=>
        // psum_j - \sum_{o} p_{j, o} == 0
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);

            Time psum = 0;
            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.psum[job_id]);
            model.model.elements_coefficients.push_back(1.0);
            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                model.model.elements_variables.push_back(model.p[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);
            }

            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // No-wait constraints / blocking constraints.
    if ((instance.no_wait() || instance.blocking())
            && instance.operations_arbitrary_order()) {
        // S_j <= C_{j, o} - p_{j, o}
        // <=>
        // p_{j, o} <= C_{j, o} - S_j
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);

            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                const Alternative& alternative = operation.alternatives[0];
                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.s[job_id]);
                model.model.elements_coefficients.push_back(-1.0);
                if (instance.flexible() || instance.blocking()) {
                    model.model.elements_variables.push_back(model.p[job_id][operation_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                }

                if (instance.flexible() || instance.blocking()) {
                    model.model.constraints_lower_bounds.push_back(0);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                } else if (instance.no_wait()) {
                    model.model.constraints_lower_bounds.push_back(alternative.processing_time);
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        }

        // S_j >= C_{j, o} - \sum_{o'} p_{j, o'}
        // <=>
        // C_{j, o} - S_j <= \sum_{o'} p_{j, o'}
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);

            Time processing_time_sum = 0;
            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                processing_time_sum += operation.alternatives[0].processing_time;
            }

            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.co[job_id][operation_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.s[job_id]);
                model.model.elements_coefficients.push_back(-1.0);
                if (instance.blocking() || instance.flexible()) {
                    model.model.elements_variables.push_back(model.psum[job_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                }

                if (instance.flexible() || instance.blocking()) {
                    model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
                    model.model.constraints_upper_bounds.push_back(0);
                } else if (instance.no_wait()) {
                    model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
                    model.model.constraints_upper_bounds.push_back(processing_time_sum);
                }
            }
        }
    }

    //std::cout << "MILP model" << std::endl;
    //std::cout << "----------" << std::endl;
    //model.model.format(std::cout, 4);

    //std::vector<double> solution = {0, 1, 3, 0, 3, 3, 3};
    //model.model.check_solution(solution);
    return model;
}

Solution retrieve_solution(
        const Instance& instance,
        const Model& model,
        const std::vector<double>& milp_solution)
{
    //std::cout << "MILP solution" << std::endl;
    //std::cout << "-------------" << std::endl;
    //model.model.format_solution(std::cout, milp_solution, 3);

    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        const Job& job = instance.job(job_id);
        for (OperationId operation_id = 0;
                operation_id < job.operations.size();
                ++operation_id) {
            const Operation& operation = job.operations[operation_id];

            AlternativeId selected_alternative_id
                = (instance.flexible())? -1: 0;
            if (instance.flexible()) {
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.alternatives.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.alternatives[alternative_id];
                    int x = std::round(milp_solution[model.x[job_id][operation_id][alternative_id]]);
                    if (x == 1) {
                        selected_alternative_id = alternative_id;
                        break;
                    }
                }
            }
            if (selected_alternative_id == -1)
                continue;

            const Alternative& alternative = operation.alternatives[selected_alternative_id];
            Time completion_time = std::round(milp_solution[model.co[job_id][operation_id]]);
            Time start = (instance.blocking())?
                completion_time - std::round(milp_solution[model.p[job_id][operation_id]]):
                completion_time - alternative.processing_time;
            solution_builder.append_operation(
                    job_id,
                    operation_id,
                    selected_alternative_id,
                    start);
        }
    }
    solution_builder.sort_machines();
    solution_builder.sort_jobs();
    Solution solution = solution_builder.build();

    //std::cout << "Solution" << std::endl;
    //std::cout << "--------" << std::endl;
    //solution.format(std::cout, 4);

    bool milp_feasible = model.model.check_solution(milp_solution, 0);
    if (milp_feasible) {
        if (!solution.feasible()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "wrong solution feasibility; "
                    "solution.feasible(): " + std::to_string(solution.feasible()) + "; "
                    "milp_feasible: " + std::to_string(milp_feasible) + ".");
        }
        double milp_objective_value = model.model.evaluate_objective(milp_solution);
        switch (instance.objective()) {
        case Objective::Makespan: {
            if (solution.makespan() > std::round(milp_objective_value)) {
                throw std::invalid_argument(
                        FUNC_SIGNATURE + ": "
                        "wrong solution makespan; "
                        "solution.makespan(): " + std::to_string(solution.makespan()) + "; "
                        "milp_objective_value: " + std::to_string(milp_objective_value) + ".");
            }
            break;
        } case Objective::TotalFlowTime: {
            if (solution.total_flow_time() > std::round(milp_objective_value)) {
                throw std::invalid_argument(
                        FUNC_SIGNATURE + ": "
                        "wrong solution total flow time; "
                        "solution.total_flow_time(): " + std::to_string(solution.total_flow_time()) + "; "
                        "milp_objective_value: " + std::to_string(milp_objective_value) + ".");
            }
            break;
        } case Objective::Throughput: {
            if (solution.throughput() > std::round(milp_objective_value)) {
                throw std::invalid_argument(
                        FUNC_SIGNATURE + ": "
                        "wrong solution throughput; "
                        "solution.throughput(): " + std::to_string(solution.throughput()) + "; "
                        "milp_objective_value: " + std::to_string(milp_objective_value) + ".");
            }
            break;
        } case Objective::TotalTardiness: {
            if (solution.total_tardiness() > std::round(milp_objective_value)) {
                throw std::invalid_argument(
                        FUNC_SIGNATURE + ": "
                        "wrong solution total tardiness; "
                        "solution.total_tardiness(): " + std::to_string(solution.total_tardiness()) + "; "
                        "milp_objective_value: " + std::to_string(milp_objective_value) + ".");
            }
            break;
        }
        }
    }
    return solution;
}

#ifdef CBC_FOUND

class EventHandler: public CbcEventHandler
{

public:

    virtual CbcAction event(CbcEvent which_event);

    EventHandler(
            const Instance& instance,
            const MilpDisjunctiveParameters& parameters,
            const Model& milp_model,
            Output& output,
            AlgorithmFormatter& algorithm_formatter):
        CbcEventHandler(),
        instance_(instance),
        parameters_(parameters),
        milp_model_(milp_model),
        output_(output),
        algorithm_formatter_(algorithm_formatter) { }

    virtual ~EventHandler() { }

    EventHandler(const EventHandler &rhs):
        CbcEventHandler(rhs),
        instance_(rhs.instance_),
        parameters_(rhs.parameters_),
        milp_model_(rhs.milp_model_),
        output_(rhs.output_),
        algorithm_formatter_(rhs.algorithm_formatter_) { }

    virtual CbcEventHandler* clone() const { return new EventHandler(*this); }

private:

    const Instance& instance_;
    const MilpDisjunctiveParameters& parameters_;
    const Model& milp_model_;
    Output& output_;
    AlgorithmFormatter& algorithm_formatter_;

};

CbcEventHandler::CbcAction EventHandler::event(CbcEvent which_event)
{
    // Not in subtree.
    if ((model_->specialOptions() & 2048) != 0)
        return noAction;
    const CbcModel& cbc_model = *model_;

    int number_of_nodes = mathoptsolverscmake::get_number_of_nodes(cbc_model);

    // Retrieve solution.
    double milp_objective_value = mathoptsolverscmake::get_solution_value(cbc_model);
    if (!output_.solution.feasible()
            || ((instance_.objective() == Objective::Makespan && output_.solution.makespan() > milp_objective_value)
                || (instance_.objective() == Objective::TotalFlowTime && output_.solution.total_flow_time() > milp_objective_value)
                || (instance_.objective() == Objective::TotalTardiness && output_.solution.total_tardiness() > milp_objective_value))) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution(instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(solution, "node " + std::to_string(number_of_nodes));
    }

    // Retrieve bound.
    Time bound = std::ceil(mathoptsolverscmake::get_bound(cbc_model) - 1e5);
    switch (instance_.objective()) {
    case Objective::Makespan: {
        algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));
        break;
    } case Objective::TotalFlowTime: {
        algorithm_formatter_.update_total_flow_time_bound(bound, "node " + std::to_string(number_of_nodes));
        break;
    } case Objective::Throughput: {
        algorithm_formatter_.update_throughput_bound(bound, "node " + std::to_string(number_of_nodes));
        break;
    } case Objective::TotalTardiness: {
        algorithm_formatter_.update_total_tardiness_bound(bound, "node " + std::to_string(number_of_nodes));
        break;
    }
    }

    // Check end.
    if (parameters_.timer.needs_to_end())
        return stop;

    return noAction;
}

#endif

#ifdef XPRESS_FOUND

struct XpressCallbackUser
{
    const Instance& instance;
    const MilpDisjunctiveParameters& parameters;
    Output& output;
    AlgorithmFormatter& algorithm_formatter;
};

void xpress_callback(
        XPRSprob xpress_model,
        void* user,
        int*)
{
    const XpressCallbackUser& d = *(const XpressCallbackUser*)(user);

    // Retrieve solution.
    double milp_objective_value = mathoptsolverscmake::get_solution_value(xpress_model);
    if (!d.output.solution.feasible()
            || ((d.instance.objective() == Objective::Makespan && d.output.solution.makespan() > milp_objective_value)
                || (d.instance.objective() == Objective::TotalFlowTime && d.output.solution.total_flow_time() > milp_objective_value)
                || (d.instance.objective() == Objective::TotalTardiness && d.output.solution.total_tardiness() > milp_objective_value))) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution(d.instance, milp_solution);
        d.algorithm_formatter.update_solution(solution, "");
    }

    // Retrieve bound.
    Time bound = std::ceil(mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
    if (d.instance.objective() == Objective::Makespan)
        d.algorithm_formatter.update_makespan_bound(bound, "");

    // Check end.
    if (d.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}

Output shopschedulingsolver::milp_disjunctive(
        const Instance& instance,
        const Solution* initial_solution,
        const MilpDisjunctiveParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Disjunctive MILP");

    algorithm_formatter.print_header();

    Model milp_model = create_milp_model(instance);

    std::vector<double> milp_solution;
    double milp_bound = 0;

    if (parameters.solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::reduce_printout(cbc_model);
        mathoptsolverscmake::set_time_limit(cbc_model, parameters.timer.remaining_time());
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        EventHandler cbc_event_handler(instance, parameters, milp_model, output, algorithm_formatter);
        cbc_model.passInEventHandler(&cbc_event_handler);
        mathoptsolverscmake::solve(cbc_model);
        milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        milp_bound = mathoptsolverscmake::get_bound(cbc_model);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else if (parameters.solver == mathoptsolverscmake::SolverName::Highs) {
#ifdef HIGHS_FOUND
        Highs highs;
        mathoptsolverscmake::reduce_printout(highs);
        mathoptsolverscmake::set_time_limit(highs, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(highs, "highs.log");
        mathoptsolverscmake::load(highs, milp_model.model);
        highs.setCallback([
                &instance,
                &parameters,
                &milp_model,
                &output,
                &algorithm_formatter](
                    const int,
                    const std::string& message,
                    const HighsCallbackOutput* highs_output,
                    HighsCallbackInput* highs_input,
                    void*)
                {
                    if (!highs_output->mip_solution.empty()) {
                        // Retrieve solution.
                        double milp_objective_value = highs_output->mip_primal_bound;
                        if (!output.solution.feasible()
                                || ((instance.objective() == Objective::Makespan && output.solution.makespan() > milp_objective_value)
                                    || (instance.objective() == Objective::TotalFlowTime && output.solution.total_flow_time() > milp_objective_value)
                                    || (instance.objective() == Objective::TotalTardiness && output.solution.total_tardiness() > milp_objective_value))) {
                            Solution solution = retrieve_solution(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        // Retrieve bound.
                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            if (instance.objective() == Objective::Makespan)
                                algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                    } else {
                        // Check end.
                        if (parameters.timer.needs_to_end())
                            highs_input->user_interrupt = 1;
                    }
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipInterrupt);
        mathoptsolverscmake::solve(highs);
        milp_solution = mathoptsolverscmake::get_solution(highs);
        milp_bound = mathoptsolverscmake::get_bound(highs);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else if (parameters.solver == mathoptsolverscmake::SolverName::Xpress) {
#ifdef XPRESS_FOUND
        XPRSprob xpress_model;
        XPRScreateprob(&xpress_model);
        mathoptsolverscmake::set_time_limit(xpress_model, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(xpress_model, "xpress.log");
        mathoptsolverscmake::load(xpress_model, milp_model);
        //mathoptsolverscmake::write_mps(xpress_model, "kpc.mps");
        XpressCallbackUser xpress_callback_user{instance, parameters, output, algorithm_formatter};
        XPRSaddcbprenode(xpress_model, xpress_callback, (void*)&xpress_callback_user, 0);
        mathoptsolverscmake::solve(xpress_model);
        milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        milp_bound = mathoptsolverscmake::get_bound(xpress_model);
        XPRSdestroyprob(xpress_model);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else {
        throw std::invalid_argument(FUNC_SIGNATURE);
    }

    // Retrieve solution.
    if (!milp_solution.empty()) {
        Solution solution = retrieve_solution(instance, milp_model, milp_solution);
        algorithm_formatter.update_solution(solution, "");
    }

    // Retrieve bound.
    if (instance.objective() == Objective::Makespan) {
        algorithm_formatter.update_makespan_bound(milp_bound, "");
    } else if (instance.objective() == Objective::TotalFlowTime) {
        algorithm_formatter.update_total_flow_time_bound(milp_bound, "");
    } else if (instance.objective() == Objective::TotalTardiness) {
        algorithm_formatter.update_total_tardiness_bound(milp_bound, "");
    }

    algorithm_formatter.end();
    return output;
}

void shopschedulingsolver::write_mps(
        const Instance& instance,
        mathoptsolverscmake::SolverName solver,
        const std::string& output_path)
{
    Model milp_model = create_milp_model(instance);

    if (solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        cbc_model.solver()->writeMps(output_path.c_str());
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else if (solver == mathoptsolverscmake::SolverName::Highs) {
#ifdef HIGHS_FOUND
        Highs highs;
        mathoptsolverscmake::load(highs, milp_model.model);
        highs.writeModel(output_path);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else if (solver == mathoptsolverscmake::SolverName::Xpress) {
#ifdef XPRESS_FOUND
        XPRSprob xpress_model;
        XPRScreateprob(&xpress_model);
        mathoptsolverscmake::load(xpress_model, milp_model);
        mathoptsolverscmake::write_mps(xpress_model, "kpc.mps");
        XPRSdestroyprob(xpress_model);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else {
        throw std::invalid_argument(FUNC_SIGNATURE);
    }

}
