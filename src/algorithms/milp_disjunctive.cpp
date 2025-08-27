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
     * y_{j1, o1, j2, o2} = 1 iff o1 is scheduled before o2
     * Let o1 and o2 be two operation of a job j.
     */
    std::vector<std::vector<std::vector<int>>> y;

    /**
     * z_{j, o1, o2} = 1 iff o1 is scheduled before o2
     * These variables are only required for open shop; otherwise, the order
     * is already known.
     */
    std::vector<std::vector<std::vector<int>>> z;

    /** s_{j, o} the start time of operation (j, o). */
    std::vector<std::vector<int>> s;

    /** Makespan. */
    int cmax = -1;
};

Model create_milp_model(
        const Instance& instance)
{
    Model model;

    // Variable and objective.
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
                }
            }
        }
    }

    // Varibles s.
    model.s = std::vector<std::vector<int>>(instance.number_of_jobs());
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        const Job& job = instance.job(job_id);
        model.s[job_id] = std::vector<int>(job.operations.size());
        for (OperationId operation_id = 0;
                operation_id < job.operations.size();
                ++operation_id) {
            model.s[job_id][operation_id] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // cmax.
    model.cmax = model.model.variables_lower_bounds.size();
    model.model.variables_lower_bounds.push_back(0);
    model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
    model.model.objective_coefficients.push_back(1);

    // Constraints.

    // Makespan definition.
    if (instance.operations_arbitrary_order()) {
        // Cmax >= s_{j, o} + p_{j, o}
        // p_{j, o} <= Cmax - s_{j, o} <= inf
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
                model.model.elements_variables.push_back(model.s[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);

                model.model.constraints_lower_bounds.push_back(operation.machines[0].processing_time);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    } else {
        // Cmax >= s_{j, o_last} + p_{j, o_last}
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            OperationId operation_id = job.operations.size() - 1;
            const Operation& operation = job.operations[operation_id];
            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.cmax);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.s[job_id][operation_id]);
            model.model.elements_coefficients.push_back(-1.0);

            model.model.constraints_lower_bounds.push_back(operation.machines[0].processing_time);
            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        }
    }

    // Disjunction on machines.
    Time m = 0;
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        const Job& job = instance.job(job_id);
        for (OperationId operation_id = 0;
                operation_id < (OperationId)job.operations.size();
                ++operation_id) {
            const Operation& operation = job.operations[operation_id];
            m += operation.machines[0].processing_time;
        }
    }
    // If no-idle:
    // s_{j1, o1} = s{j2, o2} + p_{j2, o2} - M y_{o1, o2}
    // s_{j1, o2} = s{j2, o1} + p_{j2, o2} - M (1 - y_{o1, o2})
    // <=>
    // s_{j1, o1} - s{j2, o2} + M y_{o1, o2} = p_{j2, o2}
    // s_{j1, o2} - s{j2, o1} - M y_{o1, o2} = p_{j2, o2} - M
    // else
    // s_{j1, o1} >= s{j2, o2} + p_{j2, o2} - M y_{o1, o2}
    // s_{j1, o2} >= s{j2, o1} + p_{j2, o2} - M (1 - y_{o1, o2})
    // <=>
    // p_{j2, o2} <= s_{j1, o1} - s{j2, o2} + M y_{o1, o2}
    // p_{j2, o1} - M <= s_{j1, o2} - s{j2, o1} - M y_{o1, o2})
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        for (JobId pos = 0; pos < machine.operations.size(); ++pos) {
            const MachineOperation& machine_operation = machine.operations[pos];
            const Job& job = instance.job(machine_operation.job_id);
            const Operation& operation = job.operations[machine_operation.operation_id];
            for (JobId pos_2 = 0; pos_2 < pos; ++pos_2) {
                const MachineOperation& machine_operation_2 = machine.operations[pos_2];
                const Job& job_2 = instance.job(machine_operation_2.job_id);
                const Operation& operation_2 = job_2.operations[machine_operation_2.operation_id];

                {
                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.s[machine_operation.job_id][machine_operation.operation_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.s[machine_operation_2.job_id][machine_operation_2.operation_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.elements_variables.push_back(model.y[machine_id][pos][pos_2]);
                    model.model.elements_coefficients.push_back(m);

                    model.model.constraints_lower_bounds.push_back(operation_2.machines[0].processing_time);
                    if (instance.no_idle()) {
                        model.model.constraints_upper_bounds.push_back(operation_2.machines[0].processing_time);
                    } else {
                        model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                    }
                }

                {
                    model.model.constraints_starts.push_back(model.model.elements_variables.size());

                    model.model.elements_variables.push_back(model.s[machine_operation_2.job_id][machine_operation_2.operation_id]);
                    model.model.elements_coefficients.push_back(1.0);
                    model.model.elements_variables.push_back(model.s[machine_operation.job_id][machine_operation.operation_id]);
                    model.model.elements_coefficients.push_back(-1.0);
                    model.model.elements_variables.push_back(model.y[machine_id][pos][pos_2]);
                    model.model.elements_coefficients.push_back(-m);

                    model.model.constraints_lower_bounds.push_back(operation.machines[0].processing_time - m);
                    if (instance.no_idle()) {
                        model.model.constraints_upper_bounds.push_back(operation.machines[0].processing_time - m);
                    } else {
                        model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                    }
                }
            }
        }
    }

    // Disjunction on jobs.
    if (instance.operations_arbitrary_order()) {
        // s_{j, o1} >= s{j, o2} + p_{j, o2} - M z_{j, o1, o2}
        // s_{j, o2} >= s{j, o1} + p_{j, o1} - M (1 - z_{j, o1, o2})
        // <=>
        // p_{j, o2} <= s_{j, o1} - s{j, o2} + M z_{j, o1, o2}
        // p_{j, o1} - M <= s_{j, o2} - s{j, o1} - M z_{j, o1, o2})
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

                        model.model.elements_variables.push_back(model.s[job_id][operation_id]);
                        model.model.elements_coefficients.push_back(1.0);
                        model.model.elements_variables.push_back(model.s[job_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(-1.0);
                        model.model.elements_variables.push_back(model.z[job_id][operation_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(m);

                        model.model.constraints_lower_bounds.push_back(operation_2.machines[0].processing_time);
                        if (instance.no_wait()) {
                            model.model.constraints_upper_bounds.push_back(operation_2.machines[0].processing_time);
                        } else {
                            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                        }
                    }

                    {
                        model.model.constraints_starts.push_back(model.model.elements_variables.size());

                        model.model.elements_variables.push_back(model.s[job_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(1.0);
                        model.model.elements_variables.push_back(model.s[job_id][operation_id]);
                        model.model.elements_coefficients.push_back(-1.0);
                        model.model.elements_variables.push_back(model.z[job_id][operation_id][operation_2_id]);
                        model.model.elements_coefficients.push_back(-m);

                        model.model.constraints_lower_bounds.push_back(operation.machines[0].processing_time - m);
                        if (instance.no_wait()) {
                            model.model.constraints_upper_bounds.push_back(operation.machines[0].processing_time - m);
                        } else {
                            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                        }
                    }
                }
            }
        }
    } else {
        // s_{j, o2} >= s{j, o1} + p_{j, o1}
        // <=>
        // p_{j, o1} <= s_{j, o2} - s{j, o1}
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Job& job = instance.job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < job.operations.size() - 1;
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];

                model.model.constraints_starts.push_back(model.model.elements_variables.size());

                model.model.elements_variables.push_back(model.s[job_id][operation_id + 1]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.s[job_id][operation_id]);
                model.model.elements_coefficients.push_back(-1.0);

                model.model.constraints_lower_bounds.push_back(operation.machines[0].processing_time);
                if (instance.no_wait()) {
                    model.model.constraints_upper_bounds.push_back(operation.machines[0].processing_time);
                } else {
                    model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
                }
            }
        }
    }

    return model;
}

Solution retrieve_solution(
        const Instance& instance,
        const Model& model,
        const std::vector<double>& milp_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        const Job& job = instance.job(job_id);
        for (OperationId operation_id = 0;
                operation_id < job.operations.size();
                ++operation_id) {
            Time start = milp_solution[model.s[job_id][operation_id]];
            solution_builder.append_operation(
                    job_id,
                    operation_id,
                    0,  // operation_machine_id
                    start);
        }
    }
    solution_builder.sort_machines();
    Solution solution = solution_builder.build();
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
            || output_.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution(instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(solution, "node " + std::to_string(number_of_nodes));
    }

    // Retrieve bound.
    Time bound = std::ceil(mathoptsolverscmake::get_bound(cbc_model) - 1e5);
    algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));

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
            || d.output.solution.maekspan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution(d.instance, milp_solution);
        d.algorithm_formatter.update_solution(solution, "");
    }

    // Retrieve bound.
    Time bound = std::ceil(mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
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
        throw std::invalid_argument("");
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
                                || output.solution.makespan() > milp_objective_value) {
                            Solution solution = retrieve_solution(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        // Retrieve bound.
                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                    }

                    // Check end.
                    if (parameters.timer.needs_to_end())
                        highs_input->user_interrupt = 1;
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipInterrupt);
        mathoptsolverscmake::solve(highs);
        milp_solution = mathoptsolverscmake::get_solution(highs);
        milp_bound = mathoptsolverscmake::get_bound(highs);
#else
        throw std::invalid_argument("");
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
        throw std::invalid_argument("");
#endif

    } else {
        throw std::invalid_argument("");
    }

    // Retrieve solution.
    Solution solution = retrieve_solution(instance, milp_model, milp_solution);
    algorithm_formatter.update_solution(solution, "");

    // Retrieve bound.
    algorithm_formatter.update_makespan_bound(milp_bound, "");

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
        throw std::invalid_argument("");
#endif

    } else if (solver == mathoptsolverscmake::SolverName::Highs) {
#ifdef HIGHS_FOUND
        Highs highs;
        mathoptsolverscmake::load(highs, milp_model.model);
        highs.writeModel(output_path);
#else
        throw std::invalid_argument("");
#endif

    } else if (solver == mathoptsolverscmake::SolverName::Xpress) {
#ifdef XPRESS_FOUND
        XPRSprob xpress_model;
        XPRScreateprob(&xpress_model);
        mathoptsolverscmake::load(xpress_model, milp_model);
        mathoptsolverscmake::write_mps(xpress_model, "kpc.mps");
        XPRSdestroyprob(xpress_model);
#else
        throw std::invalid_argument("");
#endif

    } else {
        throw std::invalid_argument("");
    }

}
