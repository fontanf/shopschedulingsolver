/**
 * Permutation flow shop scheduling problem, total completion time
 *
 * Tree search:
 * - Forward branching
 * - Guide:
 *   - 0: total completion time
 *   - 1: idle time
 *   - 2: weighted idle time
 *   - 3: total completion time and weighted idle time
 */

#include "shopschedulingsolver/algorithms/tree_search_pfss_tft.hpp"

#include "shopschedulingsolver/solution_builder.hpp"

#include "treesearchsolver/iterative_beam_search.hpp"

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

        /** For each machine, the current time. */
        std::vector<Time> times;

        /** Total completion time of the partial solution. */
        Time total_completion_time = 0;

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
        r->times.resize(instance_.number_of_machines(), 0);
        r->bound = 0;
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            MachineId machine_id = instance_.number_of_machines() - 1;
            r->bound += job.operations[machine_id].machines[0].processing_time;
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
        node->times = parent->times;
        Time p0 = job.operations[0].machines[0].processing_time;
        node->times[0] = parent->times[0] + p0;
        for (MachineId machine_id = 1;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            Time p = job.operations[machine_id].machines[0].processing_time;
            if (node->times[machine_id - 1] > parent->times[machine_id]) {
                node->times[machine_id] = node->times[machine_id - 1] + p;
            } else {
                node->times[machine_id] = parent->times[machine_id] + p;
            }
        }
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& parent) const
    {
        // Compute parent's structures.
        if (parent->times.empty())
            compute_structures(parent);

        //if (parent->next_child_pos == 0)
        //    std::cout << "parent"
        //        << " j " << parent->job_id
        //        << " n " << parent->number_of_jobs
        //        << " ct " << parent->total_completion_time
        //        << " it " << parent->idle_time
        //        << " wit " << parent->weighted_idle_time
        //        << " guide " << parent->guide
        //        << std::endl;

        // Get the next job to process.
        JobId job_next_id = parent->next_child_pos;

        // Update parent
        parent->next_child_pos++;

        // Check job availibility.
        if (!parent->available_jobs[job_next_id])
            return nullptr;

        // Compute new child.
        const Job& job_next = instance_.job(job_next_id);
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->id = node_id_;
        node_id_++;
        child->parent = parent;
        child->job_id = job_next_id;
        child->number_of_jobs = parent->number_of_jobs + 1;
        child->idle_time = parent->idle_time;
        child->weighted_idle_time = parent->weighted_idle_time;
        Time p0 = job_next.operations[0].machines[0].processing_time;
        Time t = parent->times[0] + p0;
        Time t_prec = t;
        for (MachineId machine_id = 1;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            Time p = job_next.operations[machine_id].machines[0].processing_time;
            if (t_prec > parent->times[machine_id]) {
                Time idle_time = t_prec - parent->times[machine_id];
                t = t_prec + p;
                child->idle_time += idle_time;
                child->weighted_idle_time += ((double)parent->number_of_jobs / instance_.number_of_jobs() + 1) * (instance_.number_of_machines() - machine_id) * idle_time;
            } else {
                t = parent->times[machine_id] + p;
            }
            t_prec = t;
        }
        child->total_completion_time = parent->total_completion_time + t;
        // Compute bound.
        MachineId machine_id = instance_.number_of_machines() - 1;
        child->bound = parent->bound
            + (instance_.number_of_jobs() - parent->number_of_jobs) * (t - parent->times[instance_.number_of_machines() - 1])
            - job_next.operations[machine_id].machines[0].processing_time;
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
            //child->guide = alpha * child->total_completion_time
            //    + (1.0 - alpha) * (child->weighted_idle_time + m * child->idle_time) / 2;
            child->guide = alpha * child->total_completion_time
                + (1.0 - alpha) * (child->weighted_idle_time / instance_.number_of_machines() + child->idle_time) / 2 * child->number_of_jobs / instance_.number_of_machines();
            break;
        } default: {
        }
        }
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
                if (node_1->times[machine_id] > node_2->times[machine_id]) {
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

    treesearchsolver::IterativeBeamSearchParameters<BranchingScheme> ibs_parameters;
    ibs_parameters.verbosity_level = 0;
    ibs_parameters.timer = parameters.timer;
    ibs_parameters.new_solution_callback
        = [&instance, &algorithm_formatter](
                const treesearchsolver::Output<BranchingScheme>& ts_output)
        {
            const auto& ibs_output = static_cast<const treesearchsolver::IterativeBeamSearchOutput<BranchingScheme>&>(ts_output);
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
            std::vector<Time> machines_current_times(instance.number_of_machines(), 0);
            for (JobId job_id: jobs) {
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
            std::stringstream ss;
            ss << "queue " << ibs_output.maximum_size_of_the_queue;
            algorithm_formatter.update_solution(solution, ss.str());
        };
    auto ts_output = treesearchsolver::iterative_beam_search(branching_scheme, ibs_parameters);

    if (ts_output.optimal) {
        algorithm_formatter.update_total_flow_time_bound(
                output.solution.total_flow_time(), "tree search completed");
    }

    algorithm_formatter.end();
    return output;
}
