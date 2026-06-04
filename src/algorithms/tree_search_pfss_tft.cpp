/**
 * Permutation flow shop scheduling problem, total completion time
 *
 * Tree search:
 * - Forward branching
 * - Guide:
 *   - 0: total completion time
 *   - 1: idle time
 *   - 2: total completion time and idle time
 *   - 3: total completion time and weighted idle time
 */

#include "shopschedulingsolver/algorithms/tree_search_pfss_tft.hpp"

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

        /** Idle time. */
        Time idle_time = 0;

        /**
         * Weighted idle time.
         *
         * Sum over machines of idle_time / time.
         * Computed fresh for each child (not inherited from parent).
         */
        double weighted_idle_time = 0;

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
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->id = node_id_;
        node_id_++;
        r->available_jobs.resize(instance_.number_of_jobs(), true);
        r->machines.resize(instance_.number_of_machines());
        r->bound = 0;
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            MachineId machine_id = instance_.number_of_machines() - 1;
            r->bound += job.operations[machine_id].alternatives[0].processing_time;
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
                    }
                    t_prec = start_time + p;
                    child->weighted_idle_time += (t_prec == 0)? 1.0:
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
                child->weighted_idle_time += (t_prec == 0)? 1.0:
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
                    child->weighted_idle_time += (t_prec == 0)? 1.0:
                        (double)machine_idle_time / t_prec;
                }
                if (last_machine_id > 0) {
                    Time pm = job_next.operations[last_machine_id].alternatives[0].processing_time;
                    Time machine_idle_time_last = parent->machines[last_machine_id].idle_time;
                    Time idle_time = t_prec - parent->machines[last_machine_id].time;
                    machine_idle_time_last += idle_time;
                    child->idle_time += idle_time;
                    t_prec += pm;
                    child->weighted_idle_time += (t_prec == 0)? 1.0:
                        (double)machine_idle_time_last / t_prec;
                }
            }
            child->total_completion_time = parent->total_completion_time + t_prec;
            // Compute bound.
            MachineId machine_id = instance_.number_of_machines() - 1;
            child->bound = parent->bound
                + (instance_.number_of_jobs() - parent->number_of_jobs)
                * (t_prec - parent->machines[machine_id].time)
                - job_next.operations[machine_id].alternatives[0].processing_time;
            // Compute guide.
            double alpha = (double)child->number_of_jobs / instance_.number_of_jobs();
            switch (parameters_.guide_id) {
            case 0: {
                child->guide = child->bound;
                break;
            } case 1: {
                child->guide = child->idle_time;
                break;
            } case 2: {
                child->guide = alpha * child->total_completion_time
                    + (1.0 - alpha) * child->idle_time * child->number_of_jobs / instance_.number_of_machines();
                break;
            } case 3: {
                child->guide = alpha * child->total_completion_time
                    + (1.0 - alpha) * child->weighted_idle_time * child->total_completion_time;
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
        if (node_1->bound >= node_2->total_completion_time)
            return true;
        return false;
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
        return node_1->total_completion_time < node_2->total_completion_time;
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
        if (node_1->total_completion_time <= node_2->total_completion_time) {
            bool dominates = true;
            for (MachineId machine_id = 0;
                    machine_id < instance_.number_of_machines();
                    ++machine_id) {
                if (node_1->machines[machine_id].time
                        > node_2->machines[machine_id].time) {
                    dominates = false;
                    break;
                }
            }
            if (dominates)
                return true;
        }

        return false;
    }

    /*
     * Outputs
     */

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (node->number_of_jobs != instance_.number_of_jobs())
            return "";
        std::stringstream ss;
        ss << node->total_completion_time;
        return ss.str();
    }

private:

    /** Instance. */
    const Instance& instance_;

    /** Parameters. */
    Parameters parameters_;

    mutable NodeId node_id_ = 0;

};

}

Output shopschedulingsolver::tree_search_pfss_tft(
        const Instance& instance,
        const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Tree search");
    algorithm_formatter.print_header();

    // Create LocalScheme.
    BranchingScheme::Parameters branching_scheme_parameters;
    BranchingScheme branching_scheme(instance, branching_scheme_parameters);

    treesearchsolver::IterativeBeamSearch2Parameters<BranchingScheme> ibs_parameters;
    ibs_parameters.verbosity_level = 0;
    ibs_parameters.timer = parameters.timer;
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
                                machine_id,  // operation_id
                                0,  // operation_machine_id
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
        algorithm_formatter.update_total_flow_time_bound(
                output.solution.total_flow_time(), "tree search completed");
    }

    algorithm_formatter.end();
    return output;
}
