#include "shopschedulingsolver/algorithms/milp_positional.hpp"

#include "mathoptsolverscmake/milp.hpp"

using namespace shopschedulingsolver;

namespace
{

struct Model
{
    /** Model. */
    mathoptsolverscmake::MilpModel model;

    /** x_{j, k} = 1 iff job j is in position k in the sequence. */
    std::vector<std::vector<int>> x;

    /**
     * p_{i, k} the processing time of the operation in position k on machine i.
     */
    std::vector<std::vector<int>> p;

    /** d_k the due date of the operation in position k on machine i. */
    std::vector<int> d;

    /**
     * C_{i, k} the completion time of the operation in position k on machine i.
     */
    std::vector<std::vector<int>> c;

    /** Makespan. */
    int cmax = -1;

    /**
     * t_j the tardiness of job j.
     * These variables are only required with total weighted tardiness
     * objective.
     */
    std::vector<int> t;
};

Model create_milp_model(
        const Instance& instance)
{
    Model model;

    /////////////////////////////
    // Variables and objective //
    /////////////////////////////

    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    // Variables x.
    model.x = std::vector<std::vector<int>>(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_jobs()));
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.x[job_id][pos] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(1);
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Varibles p.
    model.p = std::vector<std::vector<int>>(
            instance.number_of_machines(),
            std::vector<int>(instance.number_of_jobs()));
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.p[machine_id][pos] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Varibles p.
    model.d = std::vector<int>(std::vector<int>(instance.number_of_jobs()));
    for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
        model.d[pos] = model.model.variables_lower_bounds.size();
        model.model.variables_lower_bounds.push_back(0);
        model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
        model.model.objective_coefficients.push_back(0);
    }

    // Varibles c.
    model.c = std::vector<std::vector<int>>(
            instance.number_of_machines(),
            std::vector<int>(instance.number_of_jobs()));
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.c[machine_id][pos] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            if (instance.objective() == Objective::TotalFlowTime
                    && machine_id == instance.number_of_machines() - 1) {
                model.model.objective_coefficients.push_back(1);
            } else {
                model.model.objective_coefficients.push_back(0);
            }
        }
    }

    // cmax.
    if (instance.objective() == Objective::Makespan) {
        model.cmax = model.model.variables_lower_bounds.size();
        model.model.variables_lower_bounds.push_back(0);
        model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
        model.model.objective_coefficients.push_back(1);
    }

    // Varibles t.
    if (instance.objective() == Objective::TotalTardiness) {
        model.t = std::vector<int>(instance.number_of_jobs());
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.t[pos] = model.model.variables_lower_bounds.size();
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(1);
        }
    }

    /////////////////
    // Constraints //
    /////////////////

    // Constraints: definition of p_{i, k}.
    // p_{i, k} = sum p_{j, i} x_{j, k}
    // <=>
    // p_{i, k} - sum p_{j, i} x_{j, k} = 0
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.p[machine_id][pos]);
            model.model.elements_coefficients.push_back(1.0);

            for (JobId job_id = 0;
                    job_id < instance.number_of_jobs();
                    ++job_id) {
                const Job& job = instance.job(job_id);
                const Operation& operation = job.operations[machine_id];
                Time p = operation.alternatives[0].processing_time;

                model.model.elements_variables.push_back(model.x[job_id][pos]);
                model.model.elements_coefficients.push_back(-p);
            }

            model.model.constraints_lower_bounds.push_back(0);
            if (!instance.blocking()) {
                model.model.constraints_upper_bounds.push_back(0);
            } else {
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // Constraints: definition of d_k.
    // d_k = sum d_j x_{j, k}
    // <=>
    // d_k - sum d_j x_{j, k} = 0
    if (instance.objective() == Objective::TotalTardiness) {
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.d[pos]);
            model.model.elements_coefficients.push_back(1.0);

            for (JobId job_id = 0;
                    job_id < instance.number_of_jobs();
                    ++job_id) {
                const Job& job = instance.job(job_id);

                model.model.elements_variables.push_back(model.x[job_id][pos]);
                model.model.elements_coefficients.push_back(-job.due_date);
            }

            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // Constraints: makespan definition.
    if (instance.objective() == Objective::Makespan) {
        // Cmax >= C_{j, o_last}
        // <=>
        // 0 <= Cmax - C_{j, o_last} <= inf
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            JobId pos = instance.number_of_jobs() - 1;

            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.cmax);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.c[machine_id][pos]);
            model.model.elements_coefficients.push_back(-1.0);

            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        }
    }

    // Constraints: definition of tardiness.
    if (instance.objective() == Objective::TotalTardiness) {
        // T_k >= C_{i_last, k} - d_k
        // <=>
        // 0 <= T_k + d_k - C_{i_last, k} <= inf
        MachineId machine_id = instance.number_of_machines() - 1;
        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {

            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.t[pos]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.d[pos]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.c[machine_id][pos]);
            model.model.elements_coefficients.push_back(-1.0);

            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        }
    }

    // Constraints: each job must have a position.
    // sum x_{j, k} = 1 for all job j.
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());

        for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
            model.model.elements_variables.push_back(model.x[job_id][pos]);
            model.model.elements_coefficients.push_back(1.0);
        }

        model.model.constraints_lower_bounds.push_back(1);
        model.model.constraints_upper_bounds.push_back(1);
    }

    // Constraints: each position must be used by a job.
    for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());

        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            model.model.elements_variables.push_back(model.x[job_id][pos]);
            model.model.elements_coefficients.push_back(1.0);
        }

        model.model.constraints_lower_bounds.push_back(1);
        model.model.constraints_upper_bounds.push_back(1);
    }

    // Constraints: an operation must start after the end of the previous
    // operation of the same job.
    // C_{i, k} >= C_{i - 1, k} + p_{i, k}
    // 0 <= C_{i, k} - C_{i - 1, k} - p_{i, k}
    {
        MachineId machine_id = 0;
        JobId pos = 0;

        model.model.constraints_starts.push_back(model.model.elements_variables.size());

        model.model.elements_variables.push_back(model.c[machine_id][pos]);
        model.model.elements_coefficients.push_back(1.0);
        model.model.elements_variables.push_back(model.p[machine_id][pos]);
        model.model.elements_coefficients.push_back(-1.0);

        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(0);
    }
    for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
        for (MachineId machine_id = 1;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.c[machine_id][pos]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.c[machine_id - 1][pos]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.elements_variables.push_back(model.p[machine_id][pos]);
            model.model.elements_coefficients.push_back(-1.0);

            model.model.constraints_lower_bounds.push_back(0);
            if (!instance.no_wait() && !instance.blocking()) {
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            } else {
                model.model.constraints_upper_bounds.push_back(0);
            }
        }
    }

    // Constraints: an operation must start after the end of the previous
    // operation scheduled on the machine.
    // C_{i, k} >= C_{i, k - 1} + p_{i, k}
    // <=>
    // 0 <= C_{i, k} - C_{i, k - 1} - p_{i, k}
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        for (JobId pos = 1; pos < instance.number_of_jobs(); ++pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());

            model.model.elements_variables.push_back(model.c[machine_id][pos]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.c[machine_id][pos - 1]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.elements_variables.push_back(model.p[machine_id][pos]);
            model.model.elements_coefficients.push_back(-1.0);

            model.model.constraints_lower_bounds.push_back(0);
            if (!machine.no_idle) {
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            } else {
                model.model.constraints_upper_bounds.push_back(0);
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
    std::vector<JobId> job_ids(instance.number_of_jobs());
    for (JobId pos = 0; pos < instance.number_of_jobs(); ++pos) {
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            double x = milp_solution[model.x[job_id][pos]];
            if (x < 0.5)
                continue;
            const Job& job = instance.job(job_id);
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                const Operation& operation = job.operations[machine_id];
                Time completion_time = std::round(milp_solution[model.c[machine_id][pos]]);
                Time processing_time = std::round(milp_solution[model.p[machine_id][pos]]);
                Time start = completion_time - processing_time;
                solution_builder.append_operation(
                        job_id,
                        machine_id,  // operation_id
                        0,  // operation_machine_id
                        start);
            }
            break;
        }
    }
    Solution solution = solution_builder.build();
    //solution.format(std::cout, 3);

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
            const MilpPositionalParameters& parameters,
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
    const MilpPositionalParameters& parameters_;
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
    if (instance_.objective() == Objective::Makespan) {
        algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));
    } else if (instance_.objective() == Objective::TotalFlowTime) {
        algorithm_formatter_.update_total_flow_time_bound(bound, "node " + std::to_string(number_of_nodes));
    } else if (instance_.objective() == Objective::TotalTardiness) {
        algorithm_formatter_.update_total_tardiness_bound(bound, "node " + std::to_string(number_of_nodes));
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
    const MilpPositionalParameters& parameters;
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
    if (d.instance.objective() == Objective::Makespan) {
        d.algorithm_formatter.update_makespan_bound(bound, "");
    } else if (d.instance.objective() == Objective::TotalFlowTime) {
        d.algorithm_formatter.update_total_flow_time_bound(bound, "");
    } else if (d.instance.objective() == Objective::TotalTardiness) {
        d.algorithm_formatter.update_total_tardiness_bound(bound, "");
    }

    // Check end.
    if (d.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}

Output shopschedulingsolver::milp_positional(
        const Instance& instance,
        const Solution* initial_solution,
        const MilpPositionalParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Positional MILP");

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
                                || ((instance.objective() == Objective::Makespan && output.solution.makespan() > milp_objective_value)
                                    || (instance.objective() == Objective::TotalFlowTime && output.solution.total_flow_time() > milp_objective_value)
                                    || (instance.objective() == Objective::TotalTardiness && output.solution.total_tardiness() > milp_objective_value))) {
                            Solution solution = retrieve_solution(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        // Retrieve bound.
                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity()) {
                            if (instance.objective() == Objective::Makespan) {
                                algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                            } else if (instance.objective() == Objective::TotalFlowTime) {
                                algorithm_formatter.update_total_flow_time_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                            } else if (instance.objective() == Objective::TotalTardiness) {
                                algorithm_formatter.update_total_tardiness_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                            }
                        }
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
