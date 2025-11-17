/**
 * Permutation flow shop scheduling problem, makespan
 *
 * Tree search:
 * - Bidirectional branching
 * - Guides:
 *   - 0: bound
 *   - 1: idle time
 *   - 2: weighted idle time
 *   - 3: bound and weighted idle time
 *   - 4: gap, bound and weighted idle time
 */

#include "shopschedulingsolver/algorithms/tree_search_pfss_makespan.hpp"

#include "shopschedulingsolver/solution_builder.hpp"

#include "treesearchsolver/iterative_beam_search.hpp"

#include <memory>
#include <sstream>

using namespace shopschedulingsolver;

namespace
{

using NodeId = int64_t;
using GuideId = int64_t;

class BranchingSchemeBidirectional
{

public:

    struct NodeMachine
    {
        Time time_forward = 0;
        Time time_backward = 0;
        Time remaining_processing_time;
        Time idle_time_forward = 0;
        Time idle_time_backward = 0;
    };

    struct Node
    {
        /** Parent node. */
        std::shared_ptr<Node> parent = nullptr;

        /** Array indicating for each job, if it still available. */
        std::vector<bool> available_jobs;

        /** Position of the last job added in the solution. */
        bool forward = true;

        /** Last job added to the partial solution. */
        JobId job_id = -1;

        /** Number of jobs in the partial solution. */
        JobId number_of_jobs = 0;

        /** Machines. */
        std::vector<NodeMachine> machines;

        /** Idle time. */
        Time idle_time = 0;

        /** Weighted idle time. */
        double weighted_idle_time = 0;

        /** Bound. */
        Time bound = 0;

        /** Guide. */
        double guide = 0;

        /** Next child to generate. */
        JobId next_child_pos = 0;

        /** Unique id of the node. */
        NodeId id = -1;
    };

    struct Parameters
    {
        /** Enable bidirectional branching (otherwise forward branching). */
        bool bidirectional = true;

        /** Guide. */
        GuideId guide_id = 3;
    };

    BranchingSchemeBidirectional(
            const Instance& instance,
            const Parameters& parameters):
        instance_(instance),
        parameters_(parameters)
    {
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingSchemeBidirectional::Node());
        r->id = node_id_;
        node_id_++;
        r->available_jobs.resize(instance_.number_of_jobs(), true);
        r->machines.resize(instance_.number_of_machines());
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            for (MachineId machine_id = 0;
                    machine_id < instance_.number_of_machines();
                    ++machine_id) {
                Time p = job.operations[machine_id].machines[0].processing_time;
                r->machines[machine_id].remaining_processing_time += p;
            }
        }
        r->bound = 0;
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            MachineId machine_id = instance_.number_of_machines() - 1;
            Time p = job.operations[machine_id].machines[0].processing_time;
            r->bound += p;
        }
        if (best_node_ == nullptr)
            best_node_ = r;
        return r;
    }

    inline void compute_structures(
            const std::shared_ptr<Node>& node) const
    {
        const Job& job = instance_.job(node->job_id);
        auto parent = node->parent;
        node->available_jobs = parent->available_jobs;
        node->available_jobs[node->job_id] = false;
        node->machines = parent->machines;
        if (parent->forward) {
            Time p0 = job.operations[0].machines[0].processing_time;
            node->machines[0].time_forward += p0;
            node->machines[0].remaining_processing_time -= p0;
            for (MachineId machine_id = 1;
                    machine_id < instance_.number_of_machines();
                    ++machine_id) {
                Time p = job.operations[machine_id].machines[0].processing_time;
                if (node->machines[machine_id - 1].time_forward
                        > parent->machines[machine_id].time_forward) {
                    Time idle_time = node->machines[machine_id - 1].time_forward
                        - parent->machines[machine_id].time_forward;
                    node->machines[machine_id].time_forward
                        = node->machines[machine_id - 1].time_forward + p;
                    node->machines[machine_id].idle_time_forward += idle_time;
                } else {
                    node->machines[machine_id].time_forward += p;
                }
                node->machines[machine_id].remaining_processing_time -= p;
            }
        } else {
            MachineId machine_id = instance_.number_of_machines() - 1;
            Time p = job.operations[machine_id].machines[0].processing_time;
            node->machines[machine_id].time_backward += p;
            node->machines[machine_id].remaining_processing_time -= p;
            for (MachineId machine_id = instance_.number_of_machines() - 2;
                    machine_id >= 0;
                    --machine_id) {
                Time p = job.operations[machine_id].machines[0].processing_time;
                if (node->machines[machine_id + 1].time_backward
                        > parent->machines[machine_id].time_backward) {
                    Time idle_time = node->machines[machine_id + 1].time_backward
                        - parent->machines[machine_id].time_backward;
                    node->machines[machine_id].time_backward
                        = node->machines[machine_id + 1].time_backward + p;
                    node->machines[machine_id].idle_time_backward += idle_time;
                } else {
                    node->machines[machine_id].time_backward += p;
                }
                node->machines[machine_id].remaining_processing_time -= p;
            }
        }
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& parent) const
    {
        // Compute parent's structures.
        if (parent->next_child_pos == 0 && parent->parent != nullptr)
            compute_structures(parent);

        //if (parent->next_child_pos == 0)
        //    std::cout << "parent"
        //        << " j " << parent->j
        //        << " n " << parent->number_of_jobs
        //        << " ct " << parent->total_completion_time
        //        << " it " << parent->idle_time
        //        << " wit " << parent->weighted_idle_time
        //        << " guide " << parent->guide
        //        << std::endl;

        // Determine wether to use forward or backward.
        if (parent->next_child_pos != 0) {
        } else if (!parameters_.bidirectional) {
            parent->forward = true;
        } else if (parent->parent == nullptr) {
            parent->forward = true;
        } else if (parent->parent->parent == nullptr) {
            parent->forward = false;
        } else {
            JobId n_forward = 0;
            JobId n_backward = 0;
            Time bound_forward = 0;
            Time bound_backward = 0;
            for (JobId job_next_id = 0;
                    job_next_id < instance_.number_of_jobs();
                    ++job_next_id) {
                if (!parent->available_jobs[job_next_id])
                    continue;
                const Job& job_next = instance_.job(job_next_id);
                Time p0 = job_next.operations[0].machines[0].processing_time;
                // Forward.
                Time bf = 0;
                Time t_prec = parent->machines[0].time_forward + p0;
                Time t = 0;
                bf = std::max(bf,
                        t_prec
                        + parent->machines[0].remaining_processing_time
                        - p0
                        + parent->machines[0].time_backward);
                for (MachineId machine_id = 1;
                        machine_id < instance_.number_of_machines();
                        ++machine_id) {
                    Time p = job_next.operations[machine_id].machines[0].processing_time;
                    if (t_prec > parent->machines[machine_id].time_forward) {
                        t = t_prec + p;
                    } else {
                        t = parent->machines[machine_id].time_forward + p;
                    }
                    bf = std::max(
                            bf,
                            t + parent->machines[machine_id].remaining_processing_time
                            - p
                            + parent->machines[machine_id].time_backward);
                    t_prec = t;
                }
                if (best_node_->number_of_jobs != instance_.number_of_jobs()
                        || bf < best_node_->bound) {
                    n_forward++;
                    bound_forward += bf;
                }
                // Backward.
                MachineId machine_id = instance_.number_of_machines() - 1;
                Time pm1 = job_next.operations[machine_id].machines[0].processing_time;
                Time bb = 0;
                t_prec = parent->machines[machine_id].time_backward + pm1;
                bb = std::max(bb,
                        parent->machines[machine_id].time_forward
                        + parent->machines[machine_id].remaining_processing_time
                        - pm1
                        + t_prec);
                for (MachineId machine_id = instance_.number_of_machines() - 2;
                        machine_id >= 0;
                        --machine_id) {
                    Time p = job_next.operations[machine_id].machines[0].processing_time;
                    if (t_prec > parent->machines[machine_id].time_backward) {
                        t = t_prec + p;
                    } else {
                        t = parent->machines[machine_id].time_backward + p;
                    }
                    bb = std::max(
                            bb,
                            parent->machines[machine_id].time_forward
                            + parent->machines[machine_id].remaining_processing_time
                            - p
                            + t);
                    t_prec = t;
                }
                if (best_node_->number_of_jobs != instance_.number_of_jobs()
                        || bb < best_node_->bound) {
                    n_backward++;
                    bound_backward += bb;
                }
            }
            if (n_forward < n_backward) {
                parent->forward = true;
            } else if (n_forward > n_backward) {
                parent->forward = false;
            } else if (bound_forward > bound_backward) {
                parent->forward = true;
            } else if (bound_forward < bound_backward) {
                parent->forward = false;
            } else {
                parent->forward = !parent->parent->forward;
            }
        }

        // Get the next job to process.
        JobId job_next_id = parent->next_child_pos;

        // Update parent
        parent->next_child_pos++;

        // Check job availibility.
        if (!parent->available_jobs[job_next_id])
            return nullptr;
        const Job& job_next = instance_.job(job_next_id);

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingSchemeBidirectional::Node());
        child->id = node_id_;
        node_id_++;
        child->parent = parent;
        child->job_id = job_next_id;
        child->number_of_jobs = parent->number_of_jobs + 1;
        // Update machines and idle_time.
        child->idle_time = parent->idle_time;
        Time t = 0;
        Time t_prec = 0;
        if (parent->forward) {
            Time p = job_next.operations[0].machines[0].processing_time;
            t_prec = parent->machines[0].time_forward + p;
            Time remaining_processing_time
                = parent->machines[0].remaining_processing_time - p;
            child->weighted_idle_time += (parent->machines[0].time_backward == 0)? 1:
                (double)parent->machines[0].idle_time_backward / parent->machines[0].time_backward;
            child->bound = std::max(child->bound,
                    t_prec
                    + remaining_processing_time
                    + parent->machines[0].time_backward);
            for (MachineId machine_id = 1;
                    machine_id < instance_.number_of_machines();
                    ++machine_id) {
                Time p = job_next.operations[machine_id].machines[0].processing_time;
                Time machine_idle_time = parent->machines[machine_id].idle_time_forward;
                if (t_prec > parent->machines[machine_id].time_forward) {
                    Time idle_time = t_prec - parent->machines[machine_id].time_forward;
                    t = t_prec + p;
                    machine_idle_time += idle_time;
                    child->idle_time += idle_time;
                } else {
                    t = parent->machines[machine_id].time_forward + p;
                }
                Time remaining_processing_time
                    = parent->machines[machine_id].remaining_processing_time - p;
                child->weighted_idle_time += (t == 0)? 1:
                    (double)machine_idle_time / t;
                child->weighted_idle_time += (parent->machines[machine_id].time_backward == 0)? 1:
                    (double)parent->machines[machine_id].idle_time_backward
                    / parent->machines[machine_id].time_backward;
                child->bound = std::max(
                        child->bound,
                        t + remaining_processing_time
                        + parent->machines[machine_id].time_backward);
                t_prec = t;
            }
        } else {
            MachineId machine_id = instance_.number_of_machines() - 1;
            Time p = job_next.operations[machine_id].machines[0].processing_time;
            t_prec = parent->machines[machine_id].time_backward + p;
            Time remaining_processing_time
                = parent->machines[machine_id].remaining_processing_time - p;
            child->weighted_idle_time += (parent->machines[machine_id].time_forward == 0)? 1:
                (double)parent->machines[machine_id].idle_time_forward / parent->machines[machine_id].time_forward;
            child->bound = std::max(child->bound,
                    parent->machines[machine_id].time_forward
                    + remaining_processing_time
                    + t_prec);
            for (MachineId machine_id = instance_.number_of_machines() - 2;
                    machine_id >= 0;
                    --machine_id) {
                Time p = job_next.operations[machine_id].machines[0].processing_time;
                Time machine_idle_time = parent->machines[machine_id].idle_time_backward;
                if (t_prec > parent->machines[machine_id].time_backward) {
                    Time idle_time = t_prec - parent->machines[machine_id].time_backward;
                    t = t_prec + p;
                    machine_idle_time += idle_time;
                    child->idle_time += idle_time;
                } else {
                    t = parent->machines[machine_id].time_backward + p;
                }
                Time remaining_processing_time
                    = parent->machines[machine_id].remaining_processing_time - p;
                child->weighted_idle_time += (parent->machines[machine_id].time_forward == 0)? 1:
                    (double)parent->machines[machine_id].idle_time_forward
                    / parent->machines[machine_id].time_forward;
                child->weighted_idle_time += (t == 0)? 1:
                    (double)machine_idle_time / t;
                child->bound = std::max(
                        child->bound,
                        parent->machines[machine_id].time_forward
                        + remaining_processing_time + t);
                t_prec = t;
            }
        }
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
            child->guide = alpha * child->bound
                + (1.0 - alpha) * child->idle_time * child->number_of_jobs / instance_.number_of_machines();
            break;
        } case 3: {
            child->guide = alpha * child->bound
                + (1.0 - alpha) * child->weighted_idle_time * child->bound;
            break;
        } case 4: {
            double a1 = (best_node_->number_of_jobs == instance_.number_of_jobs())?
                (double)(best_node_->bound) / (best_node_->bound - child->bound):
                1 - alpha;
            double a2 = (best_node_->number_of_jobs == instance_.number_of_jobs())?
                (double)(best_node_->bound - child->bound) / best_node_->bound:
                alpha;
            child->guide = a1 * child->bound
                + a2 * child->weighted_idle_time;
            break;
        } default: {
        }
        }
        if (better(child, best_node_))
            best_node_ = child;
        return child;
    }

    inline bool infertile(
            const std::shared_ptr<Node>& node) const
    {
        return (node->next_child_pos == instance_.number_of_jobs());
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
        if (node_1->bound >= node_2->bound)
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
        return node_1->bound < node_2->bound;
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
        const BranchingSchemeBidirectional& branching_scheme_;
        std::hash<std::vector<bool>> hasher;

        NodeHasher(const BranchingSchemeBidirectional& branching_scheme):
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
        (void)node_1;
        (void)node_2;
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
        ss << node->bound;
        return ss.str();
    }

private:

    /** Instance. */
    const Instance& instance_;

    /** Parameters. */
    Parameters parameters_;

    /** Best node. */
    mutable std::shared_ptr<Node> best_node_;

    mutable NodeId node_id_ = 0;

};

}

Output shopschedulingsolver::tree_search_pfss_makespan(
        const Instance& instance,
        const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Tree search");
    algorithm_formatter.print_header();

    // Create LocalScheme.
    BranchingSchemeBidirectional::Parameters branching_scheme_parameters;
    BranchingSchemeBidirectional branching_scheme(instance, branching_scheme_parameters);

    treesearchsolver::IterativeBeamSearchParameters<BranchingSchemeBidirectional> ibs_parameters;
    ibs_parameters.verbosity_level = 0;
    ibs_parameters.timer = parameters.timer;
    ibs_parameters.new_solution_callback
        = [&instance, &algorithm_formatter](
                const treesearchsolver::Output<BranchingSchemeBidirectional>& ts_output)
        {
            const auto& ibs_output = static_cast<const treesearchsolver::IterativeBeamSearchOutput<BranchingSchemeBidirectional>&>(ts_output);
            auto node = ts_output.solution_pool.best();
            std::vector<JobId> jobs_forward;
            std::vector<JobId> jobs_backward;
            for (auto node_tmp = node;
                    node_tmp->parent != nullptr;
                    node_tmp = node_tmp->parent) {
                if (node_tmp->parent->forward) {
                    jobs_forward.push_back(node_tmp->job_id);
                } else {
                    jobs_backward.push_back(node_tmp->job_id);
                }
            }
            std::reverse(jobs_forward.begin(), jobs_forward.end());
            jobs_forward.insert(jobs_forward.end(), jobs_backward.begin(), jobs_backward.end());

            SolutionBuilder solution_builder;
            solution_builder.set_instance(instance);
            std::vector<Time> machines_current_times(instance.number_of_machines(), 0);
            for (JobId job_id: jobs_forward) {
                const Job& job = instance.job(job_id);
                Time start0 = machines_current_times[0];
                solution_builder.append_operation(
                        job_id,
                        0,  // operation_id
                        0,  // operation_machine_id
                        start0);
                Time p0 = job.operations[0].machines[0].processing_time;
                machines_current_times[0] = start0 + p0;
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time start = -1;
                    if (machines_current_times[machine_id - 1]
                            > machines_current_times[machine_id]) {
                        start = machines_current_times[machine_id - 1];
                    } else {
                        start = machines_current_times[machine_id];
                    }
                    solution_builder.append_operation(
                            job_id,
                            machine_id,  // operation_id
                            0,  // operation_machine_id
                            start);
                    Time p = job.operations[machine_id].machines[0].processing_time;
                    machines_current_times[machine_id] = start + p;
                }
            }
            Solution solution = solution_builder.build();
            //solution.format(std::cout, 4);
            std::stringstream ss;
            ss << "queue " << ibs_output.maximum_size_of_the_queue;
            algorithm_formatter.update_solution(solution, ss.str());
        };
    auto ts_output = treesearchsolver::iterative_beam_search(branching_scheme, ibs_parameters);

    if (ts_output.optimal) {
        algorithm_formatter.update_makespan_bound(
                output.solution.makespan(), "tree search completed");
    }

    algorithm_formatter.end();
    return output;
}
