/**
 * Permutation flow shop scheduling problem
 *
 * Tree search:
 * - Forward branching
 * - Guide:
 *   - 0: total completion time (TFT)
 *   - 1: idle time (TFT)
 *   - 2: total completion time and idle time (TFT, default for TotalFlowTime)
 *   - 3: total completion time and weighted idle time (TFT)
 *   - 4: total tardiness, total earliness and weighted idle time
 *        (default for TotalTardiness; Fernandez-Viagas et al., 2018)
 */

#include "shopschedulingsolver/algorithms/tree_search_pfss.hpp"

#include "shopschedulingsolver/solution_builder.hpp"

#include "treesearchsolver/iterative_beam_search_2.hpp"

#include <memory>
#include <sstream>

using namespace shopschedulingsolver;

namespace
{

using NodeId = int64_t;
using GuideId = int64_t;

class BranchingScheme
{

public:

    struct NodeMachine
    {
        Time time = 0;
        Time idle_time = 0;
    };

    struct Node
    {
        /** Parent node. */
        std::shared_ptr<Node> parent = nullptr;

        /** Array indicating for each job, if it still available. */
        std::vector<bool> available_jobs;

        /** Last job added to the partial solution. */
        JobId job_id = -1;

        /** Number of jobs in the partial solution. */
        JobId number_of_jobs = 0;

        /** Machines. */
        std::vector<NodeMachine> machines;

        /** Total completion time of the partial solution. */
        Time total_completion_time = 0;

        /** Total tardiness of the scheduled jobs. */
        Time total_tardiness_scheduled = 0;

        /** Lower bound on the tardiness of the unscheduled jobs. */
        Time total_tardiness_unscheduled = 0;

        /** Total tardiness (sum of the above two). */
        Time total_tardiness = 0;

        /** Index in sorted_jobs_ up to which due dates are < machines[last].time. */
        JobId due_date_pos = 0;

        /** Number of unscheduled jobs with due date < machines[last].time. */
        JobId number_of_late_unscheduled = 0;

        /** Sum of due dates of those jobs. */
        Time sum_late_unscheduled_due_dates = 0;

        /** Total earliness of the partial solution. */
        Time total_earliness = 0;

        /** Idle time. */
        Time idle_time = 0;

        /**
         * Weighted idle time for TFT guides.
         *
         * Sum over machines of idle_time / time.
         * Computed fresh for each child (not inherited from parent).
         */
        double weighted_idle_time = 0;

        /**
         * Weighted idle time for TT guide (Fernandez-Viagas et al., 2018).
         *
         * Accumulated over all scheduled jobs.
         */
        double weighted_idle_time_tt = 0;

        /** Bound. */
        Time bound = 0;

        /** Guide. */
        double guide = 0;

        /** Unique id of the node. */
        NodeId id = -1;
    };

    struct Parameters
    {
        GuideId guide_id = 2;
    };

    BranchingScheme(
            const Instance& instance,
            Parameters parameters):
        instance_(instance),
        parameters_(parameters)
    {
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            if (job.due_date >= 0)
                sorted_jobs_.push_back({job.due_date, job_id});
        }
        std::sort(sorted_jobs_.begin(), sorted_jobs_.end());
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->id = node_id_;
        node_id_++;
        r->available_jobs.resize(instance_.number_of_jobs(), true);
        r->machines.resize(instance_.number_of_machines());
        r->bound = 0;
        if (instance_.objective() == Objective::TotalFlowTime) {
            for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
                const Job& job = instance_.job(job_id);
                MachineId machine_id = instance_.number_of_machines() - 1;
                r->bound += job.operations[machine_id].alternatives[0].processing_time;
            }
        }
        return r;
    }

    inline void compute_structures(
            const std::shared_ptr<Node>& node) const
    {
        const Job& job = instance_.job(node->job_id);
        auto parent = node->parent;
        node->available_jobs = parent->available_jobs;
        node->available_jobs[node->job_id] = false;
        node->machines.resize(instance_.number_of_machines());
        if (!instance_.blocking()) {
            Time t_prec = 0;
            for (MachineId machine_id = 0;
                    machine_id < instance_.number_of_machines();
                    ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                Time start_time = std::max(t_prec, parent->machines[machine_id].time);
                if (start_time > parent->machines[machine_id].time) {
                    node->machines[machine_id].idle_time =
                        parent->machines[machine_id].idle_time
                        + (start_time - parent->machines[machine_id].time);
                } else {
                    node->machines[machine_id].idle_time =
                        parent->machines[machine_id].idle_time;
                }
                t_prec = start_time + p;
                node->machines[machine_id].time = t_prec;
            }
        } else {
            MachineId last_machine_id = instance_.number_of_machines() - 1;
            Time p0 = job.operations[0].alternatives[0].processing_time;
            if (last_machine_id == 0) {
                node->machines[0].time = parent->machines[0].time + p0;
                node->machines[0].idle_time = parent->machines[0].idle_time;
            } else if (parent->machines[0].time + p0 > parent->machines[1].time) {
                node->machines[0].time = parent->machines[0].time + p0;
                node->machines[0].idle_time = parent->machines[0].idle_time;
            } else {
                Time idle_time = parent->machines[1].time
                    - parent->machines[0].time - p0;
                node->machines[0].time = parent->machines[1].time;
                node->machines[0].idle_time =
                    parent->machines[0].idle_time + idle_time;
            }
            for (MachineId machine_id = 1;
                    machine_id < last_machine_id;
                    ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                if (node->machines[machine_id - 1].time + p
                        > parent->machines[machine_id + 1].time) {
                    Time idle_time = node->machines[machine_id - 1].time
                        - parent->machines[machine_id].time;
                    node->machines[machine_id].time =
                        node->machines[machine_id - 1].time + p;
                    node->machines[machine_id].idle_time =
                        parent->machines[machine_id].idle_time + idle_time;
                } else {
                    Time idle_time = parent->machines[machine_id + 1].time
                        - parent->machines[machine_id].time - p;
                    node->machines[machine_id].time =
                        parent->machines[machine_id + 1].time;
                    node->machines[machine_id].idle_time =
                        parent->machines[machine_id].idle_time + idle_time;
                }
            }
            if (last_machine_id > 0) {
                Time pm = job.operations[last_machine_id].alternatives[0].processing_time;
                Time idle_time = node->machines[last_machine_id - 1].time
                    - parent->machines[last_machine_id].time;
                node->machines[last_machine_id].time =
                    node->machines[last_machine_id - 1].time + pm;
                node->machines[last_machine_id].idle_time =
                    parent->machines[last_machine_id].idle_time + idle_time;
            }
        }
    }

    inline std::vector<std::shared_ptr<Node>> children(
            const std::shared_ptr<Node>& parent) const
    {
        // Compute parent's structures if needed.
        if (parent->machines.empty())
            compute_structures(parent);

        const double n = instance_.number_of_jobs();
        const double m = instance_.number_of_machines();

        std::vector<std::shared_ptr<Node>> result;
        for (JobId job_next_id = 0;
                job_next_id < instance_.number_of_jobs();
                ++job_next_id) {
            if (!parent->available_jobs[job_next_id])
                continue;

            const Job& job_next = instance_.job(job_next_id);
            auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
            child->id = node_id_;
            node_id_++;
            child->parent = parent;
            child->job_id = job_next_id;
            child->number_of_jobs = parent->number_of_jobs + 1;
            child->idle_time = parent->idle_time;

            Time t_prec = 0;
            double ti_job = 0.0;

            if (!instance_.blocking()) {
                for (MachineId machine_id = 0;
                        machine_id < instance_.number_of_machines();
                        ++machine_id) {
                    Time p = job_next.operations[machine_id].alternatives[0].processing_time;
                    Time start_time = std::max(t_prec, parent->machines[machine_id].time);
                    Time machine_idle_time = parent->machines[machine_id].idle_time;
                    if (start_time > parent->machines[machine_id].time) {
                        Time idle_time = start_time - parent->machines[machine_id].time;
                        child->idle_time += idle_time;
                        machine_idle_time += idle_time;
                        // TI contribution (paper's weighted idle time, machines 1..m-1)
                        if (machine_id >= 1 && n > 2.0) {
                            double denom = machine_id
                                + (double)(child->number_of_jobs - 1)
                                * (m - machine_id) / (n - 2.0);
                            ti_job += m * idle_time / denom;
                        }
                    }
                    t_prec = start_time + p;
                    child->weighted_idle_time += (t_prec == 0) ? 1.0 :
                        (double)machine_idle_time / t_prec;
                }
            } else {
                MachineId last_machine_id = instance_.number_of_machines() - 1;
                Time p0 = job_next.operations[0].alternatives[0].processing_time;
                Time machine_idle_time_0 = parent->machines[0].idle_time;
                if (last_machine_id == 0) {
                    t_prec = parent->machines[0].time + p0;
                } else if (parent->machines[0].time + p0
                        > parent->machines[1].time) {
                    t_prec = parent->machines[0].time + p0;
                } else {
                    Time idle_time = parent->machines[1].time
                        - parent->machines[0].time - p0;
                    machine_idle_time_0 += idle_time;
                    child->idle_time += idle_time;
                    t_prec = parent->machines[1].time;
                }
                child->weighted_idle_time += (t_prec == 0) ? 1.0 :
                    (double)machine_idle_time_0 / t_prec;
                for (MachineId machine_id = 1;
                        machine_id < last_machine_id;
                        ++machine_id) {
                    Time p = job_next.operations[machine_id].alternatives[0].processing_time;
                    Time machine_idle_time = parent->machines[machine_id].idle_time;
                    Time idle_time;
                    if (t_prec + p > parent->machines[machine_id + 1].time) {
                        idle_time = t_prec - parent->machines[machine_id].time;
                        t_prec += p;
                    } else {
                        idle_time = parent->machines[machine_id + 1].time
                            - parent->machines[machine_id].time - p;
                        t_prec = parent->machines[machine_id + 1].time;
                    }
                    machine_idle_time += idle_time;
                    child->idle_time += idle_time;
                    child->weighted_idle_time += (t_prec == 0) ? 1.0 :
                        (double)machine_idle_time / t_prec;
                }
                if (last_machine_id > 0) {
                    Time pm = job_next.operations[last_machine_id].alternatives[0].processing_time;
                    Time machine_idle_time_last = parent->machines[last_machine_id].idle_time;
                    Time idle_time = t_prec - parent->machines[last_machine_id].time;
                    machine_idle_time_last += idle_time;
                    child->idle_time += idle_time;
                    t_prec += pm;
                    child->weighted_idle_time += (t_prec == 0) ? 1.0 :
                        (double)machine_idle_time_last / t_prec;
                }
            }

            child->total_completion_time = parent->total_completion_time
                + (instance_.number_of_jobs() - parent->number_of_jobs)
                * (t_prec - parent->machines.back().time);
            child->weighted_idle_time_tt = parent->weighted_idle_time_tt + ti_job;

            // TT / TE
            Time due_date = job_next.due_date;
            if (due_date >= 0) {
                child->total_tardiness_scheduled = parent->total_tardiness_scheduled
                    + std::max((Time)0, t_prec - due_date);
                child->total_earliness = parent->total_earliness
                    + std::max((Time)0, due_date - t_prec);
            } else {
                child->total_tardiness_scheduled = parent->total_tardiness_scheduled;
                child->total_earliness = parent->total_earliness;
            }

            // Bound
            MachineId last_machine_id = instance_.number_of_machines() - 1;

            // LB on unscheduled tardiness
            if (instance_.objective() == Objective::TotalTardiness) {
                Time t_parent = parent->machines[last_machine_id].time;
                child->due_date_pos = parent->due_date_pos;
                child->number_of_late_unscheduled = parent->number_of_late_unscheduled;
                child->sum_late_unscheduled_due_dates = parent->sum_late_unscheduled_due_dates;
                if (due_date >= 0 && due_date < t_parent) {
                    child->number_of_late_unscheduled--;
                    child->sum_late_unscheduled_due_dates -= due_date;
                }
                while (child->due_date_pos < (JobId)sorted_jobs_.size()
                       && sorted_jobs_[child->due_date_pos].first < t_prec) {
                    JobId other_id = sorted_jobs_[child->due_date_pos].second;
                    if (other_id != job_next_id && parent->available_jobs[other_id]) {
                        child->number_of_late_unscheduled++;
                        child->sum_late_unscheduled_due_dates += sorted_jobs_[child->due_date_pos].first;
                    }
                    child->due_date_pos++;
                }
                child->total_tardiness_unscheduled = child->number_of_late_unscheduled * t_prec
                    - child->sum_late_unscheduled_due_dates;
            }
            child->total_tardiness = child->total_tardiness_scheduled + child->total_tardiness_unscheduled;

            switch (instance_.objective()) {
            case Objective::TotalFlowTime: {
                child->bound = parent->bound
                    + (instance_.number_of_jobs() - parent->number_of_jobs)
                    * (t_prec - parent->machines[last_machine_id].time)
                    - job_next.operations[last_machine_id].alternatives[0].processing_time;
                break;
            }
            case Objective::TotalTardiness: {
                child->bound = child->total_tardiness;
                break;
            }
            default:
                child->bound = 0;
            }

            // Guide
            double alpha = (double)child->number_of_jobs / n;
            double k = child->number_of_jobs;
            switch (parameters_.guide_id) {
            case 0: {
                child->guide = child->bound;
                break;
            } case 1: {
                child->guide = child->idle_time;
                break;
            } case 2: {
                child->guide = alpha * child->total_completion_time
                    + (1.0 - alpha) * child->idle_time * child->number_of_jobs / m;
                break;
            } case 3: {
                child->guide = alpha * child->total_completion_time
                    + (1.0 - alpha) * child->weighted_idle_time * child->total_completion_time;
                break;
            } case 4: {
                double w_tt = (k + n - 1.0) / (2.0 * n);
                double w_te = (2.0 * n - k - 1.0) / (2.0 * n);
                double w_ti = (n > 2.0) ? (n - k - 1.0) / n : 0.0;
                child->guide = w_ti * child->weighted_idle_time_tt
                    + w_te * child->total_earliness
                    + w_tt * child->total_tardiness;
                break;
            } default: {
            }
            }
            result.push_back(child);
        }
        return result;
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_jobs != node_2->number_of_jobs)
            return node_1->number_of_jobs < node_2->number_of_jobs;
        if (node_1->guide != node_2->guide)
            return node_1->guide < node_2->guide;
        return node_1->id < node_2->id;
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_jobs == instance_.number_of_jobs();
    }

    bool bound(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_2->number_of_jobs != instance_.number_of_jobs())
            return false;
        switch (instance_.objective()) {
        case Objective::TotalFlowTime:
            return node_1->bound >= node_2->total_completion_time;
        case Objective::TotalTardiness:
            return node_1->bound >= node_2->total_tardiness;
        default:
            return false;
        }
    }

    /*
     * Solution pool.
     */

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_jobs != instance_.number_of_jobs())
            return false;
        if (node_2->number_of_jobs != instance_.number_of_jobs())
            return true;
        switch (instance_.objective()) {
        case Objective::TotalFlowTime:
            return node_1->total_completion_time < node_2->total_completion_time;
        case Objective::TotalTardiness:
            return node_1->total_tardiness < node_2->total_tardiness;
        default:
            return false;
        }
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        (void)node_1;
        (void)node_2;
        return false;
    }

    /*
     * Dominances.
     */

    inline bool comparable(
            const std::shared_ptr<Node>& node) const
    {
        (void)node;
        return false;
    }

    const Instance& instance() const { return instance_; }

    struct NodeHasher
    {
        const BranchingScheme& branching_scheme_;
        std::hash<std::vector<bool>> hasher;

        NodeHasher(const BranchingScheme& branching_scheme):
            branching_scheme_(branching_scheme) { }

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            if (node_1->available_jobs != node_2->available_jobs)
                return false;
            return true;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher(node->available_jobs);
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(*this); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        Time obj_1, obj_2;
        switch (instance_.objective()) {
        case Objective::TotalFlowTime:
            obj_1 = node_1->total_completion_time;
            obj_2 = node_2->total_completion_time;
            break;
        case Objective::TotalTardiness:
            obj_1 = node_1->total_tardiness_scheduled;
            obj_2 = node_2->total_tardiness_scheduled;
            break;
        default:
            return false;
        }
        if (obj_1 > obj_2)
            return false;
        for (MachineId machine_id = 0;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            if (node_1->machines[machine_id].time
                    > node_2->machines[machine_id].time) {
                return false;
            }
        }
        return true;
    }

    /*
     * Outputs
     */

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (node->number_of_jobs != instance_.number_of_jobs())
            return "";
        std::stringstream ss;
        switch (instance_.objective()) {
        case Objective::TotalFlowTime:
            ss << node->total_completion_time;
            break;
        case Objective::TotalTardiness:
            ss << node->total_tardiness;
            break;
        default:
            break;
        }
        return ss.str();
    }

private:

    /** Instance. */
    const Instance& instance_;

    /** Parameters. */
    Parameters parameters_;

    /** Jobs sorted by due date (only those with due_date >= 0). */
    std::vector<std::pair<Time, JobId>> sorted_jobs_;

    mutable NodeId node_id_ = 0;

};

}

Output shopschedulingsolver::tree_search_pfss(
        const Instance& instance,
        const TreeSearchPfssParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Tree search");
    algorithm_formatter.print_header();

    BranchingScheme::Parameters branching_scheme_parameters;
    if (instance.objective() == Objective::TotalTardiness)
        branching_scheme_parameters.guide_id = 4;
    BranchingScheme branching_scheme(instance, branching_scheme_parameters);

    treesearchsolver::IterativeBeamSearch2Parameters<BranchingScheme> ibs_parameters;
    ibs_parameters.verbosity_level = 0;
    ibs_parameters.timer = parameters.timer;
    ibs_parameters.minimum_size_of_the_queue = parameters.minimum_size_of_the_queue;
    ibs_parameters.maximum_size_of_the_queue = parameters.maximum_size_of_the_queue;
    ibs_parameters.new_solution_callback
        = [&instance, &algorithm_formatter](
                const treesearchsolver::Output<BranchingScheme>& ts_output)
        {
            const auto& ibs_output = static_cast<const treesearchsolver::IterativeBeamSearch2Output<BranchingScheme>&>(ts_output);
            auto node = ts_output.solution_pool.best();
            std::vector<JobId> jobs;
            for (auto node_tmp = node;
                    node_tmp->parent != nullptr;
                    node_tmp = node_tmp->parent) {
                jobs.push_back(node_tmp->job_id);
            }
            std::reverse(jobs.begin(), jobs.end());

            SolutionBuilder solution_builder;
            solution_builder.set_instance(instance);
            std::vector<Time> machines_current_departure_times(instance.number_of_machines(), 0);
            for (JobId job_id: jobs) {
                const Job& job = instance.job(job_id);
                std::vector<Time> next_departure_times(instance.number_of_machines(), 0);
                Time t_prec = 0;

                if (!instance.blocking()) {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        Time start = std::max(t_prec, machines_current_departure_times[machine_id]);
                        solution_builder.append_operation(
                                job_id,
                                machine_id,
                                0,
                                start);
                        t_prec = start + p;
                        next_departure_times[machine_id] = t_prec;
                    }
                } else {
                    MachineId last_machine_id = instance.number_of_machines() - 1;
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    Time start0 = std::max(t_prec, machines_current_departure_times[0]);
                    solution_builder.append_operation(job_id, 0, 0, start0);
                    if (last_machine_id == 0) {
                        t_prec = start0 + p0;
                    } else if (start0 + p0 > machines_current_departure_times[1]) {
                        t_prec = start0 + p0;
                    } else {
                        t_prec = machines_current_departure_times[1];
                    }
                    next_departure_times[0] = t_prec;
                    for (MachineId machine_id = 1;
                            machine_id < last_machine_id;
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        solution_builder.append_operation(job_id, machine_id, 0, t_prec);
                        if (t_prec + p > machines_current_departure_times[machine_id + 1]) {
                            t_prec += p;
                        } else {
                            t_prec = machines_current_departure_times[machine_id + 1];
                        }
                        next_departure_times[machine_id] = t_prec;
                    }
                    if (last_machine_id > 0) {
                        Time pm = job.operations[last_machine_id].alternatives[0].processing_time;
                        solution_builder.append_operation(job_id, last_machine_id, 0, t_prec);
                        t_prec += pm;
                        next_departure_times[last_machine_id] = t_prec;
                    }
                }
                machines_current_departure_times = next_departure_times;
            }
            Solution solution = solution_builder.build();
            std::stringstream ss;
            ss << "queue " << ibs_output.maximum_size_of_the_queue;
            algorithm_formatter.update_solution(solution, ss.str());
        };
    auto ts_output = treesearchsolver::iterative_beam_search_2(branching_scheme, ibs_parameters);

    if (ts_output.optimal) {
        switch (instance.objective()) {
        case Objective::TotalFlowTime:
            algorithm_formatter.update_total_flow_time_bound(
                    output.solution.total_flow_time(), "tree search completed");
            break;
        case Objective::TotalTardiness:
            algorithm_formatter.update_total_tardiness_bound(
                    output.solution.total_tardiness(), "tree search completed");
            break;
        default:
            break;
        }
    }

    algorithm_formatter.end();
    return output;
}
