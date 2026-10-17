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

#include "treesearchsolver/iterative_beam_search_2.hpp"

#include <limits>
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
        Time remaining_processing_time = 0;
        Time idle_time_forward = 0;
        Time idle_time_backward = 0;
    };

    struct Node
    {
        /** Parent node. */
        std::shared_ptr<Node> parent = nullptr;

        /** Array indicating for each job, if it still available. */
        std::vector<bool> available_jobs;

        /**
         * True iff the structures of the node ('available_jobs', 'machines'
         * and 'forward') have been computed.
         */
        bool structures_computed = false;

        /** True iff the children of the node are added at the front. */
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
        auto root = std::shared_ptr<Node>(new BranchingSchemeBidirectional::Node());
        root->id = node_id_;
        node_id_++;
        root->structures_computed = true;
        root->available_jobs.resize(instance_.number_of_jobs(), true);
        root->machines.resize(instance_.number_of_machines());
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            for (MachineId machine_id = 0;
                    machine_id < instance_.number_of_machines();
                    ++machine_id) {
                Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                root->machines[machine_id].remaining_processing_time += processing_time;
            }
        }
        root->bound = 0;
        for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
            const Job& job = instance_.job(job_id);
            MachineId machine_id = instance_.number_of_machines() - 1;
            Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
            root->bound += processing_time;
        }
        if (best_node_ == nullptr)
            best_node_ = root;
        return root;
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
            if (!instance_.blocking()) {
                Time processing_time_0 = job.operations[0].alternatives[0].processing_time;
                node->machines[0].time_forward = parent->machines[0].time_forward + processing_time_0;
                node->machines[0].remaining_processing_time -= processing_time_0;
                for (MachineId machine_id = 1;
                        machine_id < instance_.number_of_machines();
                        ++machine_id) {
                    Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                    if (node->machines[machine_id - 1].time_forward
                            > parent->machines[machine_id].time_forward) {
                        Time idle_time = node->machines[machine_id - 1].time_forward
                            - parent->machines[machine_id].time_forward;
                        node->machines[machine_id].time_forward
                            = node->machines[machine_id - 1].time_forward + processing_time;
                        node->machines[machine_id].idle_time_forward += idle_time;
                    } else {
                        node->machines[machine_id].time_forward += processing_time;
                    }
                    node->machines[machine_id].remaining_processing_time -= processing_time;
                }
            } else {
                MachineId last_machine_id = instance_.number_of_machines() - 1;

                Time processing_time_0 = job.operations[0].alternatives[0].processing_time;
                if (parent->machines[0].time_forward + processing_time_0
                        > parent->machines[1].time_forward) {
                    node->machines[0].time_forward
                        = parent->machines[0].time_forward + processing_time_0;
                } else {
                    Time idle_time = parent->machines[1].time_forward
                        - node->machines[0].time_forward - processing_time_0;
                    node->machines[0].time_forward
                        = parent->machines[1].time_forward;
                    node->machines[0].idle_time_forward += idle_time;
                }
                node->machines[0].remaining_processing_time -= processing_time_0;

                for (MachineId machine_id = 1;
                        machine_id < last_machine_id;
                        ++machine_id) {
                    Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                    if (node->machines[machine_id - 1].time_forward + processing_time
                            > parent->machines[machine_id + 1].time_forward) {
                        Time idle_time = node->machines[machine_id - 1].time_forward
                            - node->machines[machine_id].time_forward;
                        node->machines[machine_id].time_forward
                            = node->machines[machine_id - 1].time_forward + processing_time;
                        node->machines[machine_id].idle_time_forward += idle_time;
                    } else {
                        Time idle_time = node->machines[machine_id + 1].time_forward
                            - node->machines[machine_id].time_forward - processing_time;
                        node->machines[machine_id].time_forward
                            = node->machines[machine_id + 1].time_forward;
                        node->machines[machine_id].idle_time_forward += idle_time;
                    }
                    node->machines[machine_id].remaining_processing_time -= processing_time;
                }

                Time processing_time_last = job.operations[last_machine_id].alternatives[0].processing_time;
                Time idle_time = node->machines[last_machine_id - 1].time_forward
                    - node->machines[last_machine_id].time_forward;
                node->machines[last_machine_id].time_forward
                    = node->machines[last_machine_id - 1].time_forward + processing_time_last;
                node->machines[last_machine_id].idle_time_forward += idle_time;
                node->machines[last_machine_id].remaining_processing_time -= processing_time_last;
            }
        } else {
            MachineId last_machine_id = instance_.number_of_machines() - 1;

            if (!instance_.blocking()) {
                Time processing_time = job.operations[last_machine_id].alternatives[0].processing_time;
                node->machines[last_machine_id].time_backward
                    = parent->machines[last_machine_id].time_backward + processing_time;
                node->machines[last_machine_id].remaining_processing_time -= processing_time;
                for (MachineId machine_id = last_machine_id - 1;
                        machine_id >= 0;
                        --machine_id) {
                    Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                    if (node->machines[machine_id + 1].time_backward
                            > parent->machines[machine_id].time_backward) {
                        Time idle_time = node->machines[machine_id + 1].time_backward
                            - parent->machines[machine_id].time_backward;
                        node->machines[machine_id].time_backward
                            = node->machines[machine_id + 1].time_backward + processing_time;
                        node->machines[machine_id].idle_time_backward += idle_time;
                    } else {
                        node->machines[machine_id].time_backward += processing_time;
                    }
                    node->machines[machine_id].remaining_processing_time -= processing_time;
                }
            } else {
                Time processing_time_last = job.operations[last_machine_id].alternatives[0].processing_time;
                if (parent->machines[last_machine_id].time_backward + processing_time_last
                        > parent->machines[last_machine_id - 1].time_backward) {
                    node->machines[last_machine_id].time_backward
                        = parent->machines[last_machine_id].time_backward + processing_time_last;
                } else {
                    Time idle_time = parent->machines[last_machine_id - 1].time_backward
                        - node->machines[last_machine_id].time_backward - processing_time_last;
                    node->machines[last_machine_id].time_backward
                        = parent->machines[last_machine_id - 1].time_backward;
                    node->machines[last_machine_id].idle_time_backward += idle_time;
                }
                node->machines[last_machine_id].remaining_processing_time -= processing_time_last;

                for (MachineId machine_id = last_machine_id - 1;
                        machine_id > 0;
                        --machine_id) {
                    Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                    if (node->machines[machine_id + 1].time_backward + processing_time
                            > parent->machines[machine_id - 1].time_backward) {
                        Time idle_time = node->machines[machine_id + 1].time_backward
                            - node->machines[machine_id].time_backward;
                        node->machines[machine_id].time_backward
                            = node->machines[machine_id + 1].time_backward + processing_time;
                        node->machines[machine_id].idle_time_backward += idle_time;
                    } else {
                        Time idle_time = node->machines[machine_id - 1].time_backward
                            - node->machines[machine_id].time_backward - processing_time;
                        node->machines[machine_id].time_backward
                            = node->machines[machine_id - 1].time_backward;
                        node->machines[machine_id].idle_time_backward += idle_time;
                    }
                    node->machines[machine_id].remaining_processing_time -= processing_time;
                }

                Time processing_time_0 = job.operations[0].alternatives[0].processing_time;
                Time idle_time = node->machines[1].time_backward
                    - node->machines[0].time_backward;
                node->machines[0].time_backward
                    = node->machines[1].time_backward + processing_time_0;
                node->machines[0].idle_time_backward += idle_time;
                node->machines[0].remaining_processing_time -= processing_time_0;
            }
        }
    }

    /**
     * Determine whether the children of a node are added at the front or at
     * the back of the partial solution.
     *
     * The direction depends on 'best_node_' which changes during the search.
     * Therefore, it must be computed only once per node, since the
     * structures of its children rely on it.
     */
    inline bool compute_direction(
            const std::shared_ptr<Node>& node) const
    {
        if (!parameters_.bidirectional) {
            return true;
        } else if (node->parent == nullptr) {
            return true;
        } else if (node->parent->parent == nullptr) {
            return false;
        } else {
            JobId number_of_children_forward = 0;
            JobId number_of_children_backward = 0;
            Time bound_sum_forward = 0;
            Time bound_sum_backward = 0;
            for (JobId job_next_id = 0;
                    job_next_id < instance_.number_of_jobs();
                    ++job_next_id) {
                if (!node->available_jobs[job_next_id])
                    continue;
                const Job& job_next = instance_.job(job_next_id);
                // Forward.
                Time bound_forward = 0;
                Time previous_completion_time = 0;
                Time completion_time = 0;
                if (!instance_.blocking()) {
                    Time processing_time_0 = job_next.operations[0].alternatives[0].processing_time;
                    previous_completion_time = node->machines[0].time_forward + processing_time_0;
                    bound_forward = std::max(
                        bound_forward,
                        previous_completion_time
                        + node->machines[0].remaining_processing_time
                        - processing_time_0
                        + node->machines[0].time_backward);
                    for (MachineId machine_id = 1;
                            machine_id < instance_.number_of_machines();
                            ++machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        if (previous_completion_time > node->machines[machine_id].time_forward) {
                            completion_time = previous_completion_time + processing_time;
                        } else {
                            completion_time = node->machines[machine_id].time_forward + processing_time;
                        }
                        bound_forward = std::max(
                                bound_forward,
                                completion_time
                                + node->machines[machine_id].remaining_processing_time
                                - processing_time
                                + node->machines[machine_id].time_backward);
                        previous_completion_time = completion_time;
                    }
                } else {
                    Time processing_time_0 = job_next.operations[0].alternatives[0].processing_time;
                    if (node->machines[0].time_forward + processing_time_0 > node->machines[1].time_forward) {
                        previous_completion_time = node->machines[0].time_forward + processing_time_0;
                    } else {
                        previous_completion_time = node->machines[1].time_forward;
                    }
                    bound_forward = std::max(
                        bound_forward,
                        previous_completion_time
                        + node->machines[0].remaining_processing_time - processing_time_0
                        + node->machines[0].time_backward);
                    MachineId last_machine_id = instance_.number_of_machines() - 1;
                    for (MachineId machine_id = 1;
                            machine_id < last_machine_id;
                            ++machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        Time t_departure;
                        if (previous_completion_time + processing_time > node->machines[machine_id + 1].time_forward) {
                            t_departure = previous_completion_time + processing_time;
                        } else {
                            t_departure = node->machines[machine_id + 1].time_forward;
                        }
                        bound_forward = std::max(
                                bound_forward,
                                t_departure
                                + node->machines[machine_id].remaining_processing_time - processing_time
                                + node->machines[machine_id].time_backward);
                        previous_completion_time = t_departure;
                    }
                    bound_forward = std::max(
                            bound_forward,
                            previous_completion_time
                            + node->machines[last_machine_id].remaining_processing_time
                            + node->machines[last_machine_id].time_backward);
                }

                if (best_node_->number_of_jobs != instance_.number_of_jobs()
                        || bound_forward < best_node_->bound) {
                    number_of_children_forward++;
                    bound_sum_forward += bound_forward;
                }
                // Backward.
                MachineId machine_id_last = instance_.number_of_machines() - 1;
                Time bound_backward = 0;
                previous_completion_time = 0;
                if (!instance_.blocking()) {
                    Time processing_time_last = job_next.operations[machine_id_last].alternatives[0].processing_time;
                    previous_completion_time = node->machines[machine_id_last].time_backward + processing_time_last;
                    bound_backward = std::max(bound_backward,
                            node->machines[machine_id_last].time_forward
                            + node->machines[machine_id_last].remaining_processing_time
                            - processing_time_last
                            + previous_completion_time);
                    for (MachineId machine_id = machine_id_last - 1;
                            machine_id >= 0;
                            --machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        if (previous_completion_time > node->machines[machine_id].time_backward) {
                            completion_time = previous_completion_time + processing_time;
                        } else {
                            completion_time = node->machines[machine_id].time_backward + processing_time;
                        }
                        bound_backward = std::max(
                                bound_backward,
                                node->machines[machine_id].time_forward
                                + node->machines[machine_id].remaining_processing_time
                                - processing_time
                                + completion_time);
                        previous_completion_time = completion_time;
                    }
                } else {
                    Time processing_time_last = job_next.operations[machine_id_last].alternatives[0].processing_time;
                    if (node->machines[machine_id_last].time_backward + processing_time_last > node->machines[machine_id_last - 1].time_backward) {
                        previous_completion_time = node->machines[machine_id_last].time_backward + processing_time_last;
                    } else {
                        previous_completion_time = node->machines[machine_id_last - 1].time_backward;
                    }
                    bound_backward = std::max(bound_backward,
                            node->machines[machine_id_last].time_forward
                            + node->machines[machine_id_last].remaining_processing_time - processing_time_last
                            + previous_completion_time);
                    for (MachineId machine_id = machine_id_last - 1;
                            machine_id > 0;
                            --machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        Time t_departure;
                        if (previous_completion_time + processing_time > node->machines[machine_id - 1].time_backward) {
                            t_departure = previous_completion_time + processing_time;
                        } else {
                            t_departure = node->machines[machine_id - 1].time_backward;
                        }
                        bound_backward = std::max(
                                bound_backward,
                                node->machines[machine_id].time_forward
                                + node->machines[machine_id].remaining_processing_time - processing_time
                                + t_departure);
                        previous_completion_time = t_departure;
                    }
                    bound_backward = std::max(
                            bound_backward,
                            node->machines[0].time_forward
                            + node->machines[0].remaining_processing_time
                            + previous_completion_time);
                }

                if (best_node_->number_of_jobs != instance_.number_of_jobs()
                        || bound_backward < best_node_->bound) {
                    number_of_children_backward++;
                    bound_sum_backward += bound_backward;
                }
            }
            if (number_of_children_forward < number_of_children_backward) {
                return true;
            } else if (number_of_children_forward > number_of_children_backward) {
                return false;
            } else if (bound_sum_forward > bound_sum_backward) {
                return true;
            } else if (bound_sum_forward < bound_sum_backward) {
                return false;
            } else {
                return !node->parent->forward;
            }
        }
    }

    inline std::vector<std::shared_ptr<Node>> children(
            const std::shared_ptr<Node>& parent) const
    {
        // Compute parent's structures if needed.
        if (!parent->structures_computed) {
            compute_structures(parent);
            parent->forward = compute_direction(parent);
            parent->structures_computed = true;
        }

        // Generate children.
        std::vector<std::shared_ptr<Node>> result;
        result.reserve(instance_.number_of_jobs() - parent->number_of_jobs);
        for (JobId job_next_id = 0;
                job_next_id < instance_.number_of_jobs();
                ++job_next_id) {
            if (!parent->available_jobs[job_next_id])
                continue;
            const Job& job_next = instance_.job(job_next_id);

            auto child = std::shared_ptr<Node>(new BranchingSchemeBidirectional::Node());
            child->id = node_id_;
            node_id_++;
            child->parent = parent;
            child->job_id = job_next_id;
            child->number_of_jobs = parent->number_of_jobs + 1;
            child->idle_time = parent->idle_time;
            Time completion_time = 0;
            Time previous_completion_time = 0;
            if (parent->forward) {
                if (!instance_.blocking()) {
                    Time processing_time_0 = job_next.operations[0].alternatives[0].processing_time;
                    previous_completion_time = parent->machines[0].time_forward + processing_time_0;
                    Time remaining_processing_time =
                        parent->machines[0].remaining_processing_time - processing_time_0;
                    child->weighted_idle_time += (parent->machines[0].time_backward == 0)? 1:
                        (double)parent->machines[0].idle_time_backward / parent->machines[0].time_backward;
                    child->bound = std::max(child->bound,
                            parent->machines[0].time_forward
                            + parent->machines[0].remaining_processing_time
                            + parent->machines[0].time_backward);
                    for (MachineId machine_id = 1;
                            machine_id < instance_.number_of_machines();
                            ++machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        Time machine_idle_time = parent->machines[machine_id].idle_time_forward;
                        if (previous_completion_time > parent->machines[machine_id].time_forward) {
                            Time idle_time = previous_completion_time - parent->machines[machine_id].time_forward;
                            completion_time = previous_completion_time + processing_time;
                            machine_idle_time += idle_time;
                            child->idle_time += idle_time;
                        } else {
                            completion_time = parent->machines[machine_id].time_forward + processing_time;
                        }
                        remaining_processing_time =
                            parent->machines[machine_id].remaining_processing_time - processing_time;
                        child->weighted_idle_time += (completion_time == 0)? 1:
                            (double)machine_idle_time / completion_time;
                        child->weighted_idle_time += (parent->machines[machine_id].time_backward == 0)? 1:
                            (double)parent->machines[machine_id].idle_time_backward
                            / parent->machines[machine_id].time_backward;
                        child->bound = std::max(
                                child->bound,
                                completion_time + remaining_processing_time
                                + parent->machines[machine_id].time_backward);
                        previous_completion_time = completion_time;
                    }
                } else {
                    Time processing_time_0 = job_next.operations[0].alternatives[0].processing_time;
                    if (parent->machines[0].time_forward + processing_time_0 > parent->machines[1].time_forward) {
                        previous_completion_time = parent->machines[0].time_forward + processing_time_0;
                    } else {
                        previous_completion_time = parent->machines[1].time_forward;
                    }
                    Time remaining_processing_time =
                        parent->machines[0].remaining_processing_time - processing_time_0;
                    child->weighted_idle_time += (parent->machines[0].time_backward == 0)? 1:
                        (double)parent->machines[0].idle_time_backward / parent->machines[0].time_backward;
                    child->bound = std::max(child->bound,
                            previous_completion_time
                            + parent->machines[0].remaining_processing_time - processing_time_0
                            + parent->machines[0].time_backward);
                    MachineId last_machine_id = instance_.number_of_machines() - 1;
                    for (MachineId machine_id = 1;
                            machine_id < last_machine_id;
                            ++machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        Time machine_idle_time = parent->machines[machine_id].idle_time_forward;
                        Time idle_time;
                        Time t_departure;
                        if (previous_completion_time + processing_time > parent->machines[machine_id + 1].time_forward) {
                            idle_time = previous_completion_time - parent->machines[machine_id].time_forward;
                            t_departure = previous_completion_time + processing_time;
                        } else {
                            idle_time = parent->machines[machine_id + 1].time_forward
                                - parent->machines[machine_id].time_forward - processing_time;
                            t_departure = parent->machines[machine_id + 1].time_forward;
                        }
                        child->bound = std::max(
                                child->bound,
                                t_departure
                                + parent->machines[machine_id].remaining_processing_time - processing_time
                                + parent->machines[machine_id].time_backward);
                        machine_idle_time += idle_time;
                        child->idle_time += idle_time;
                        previous_completion_time = t_departure;
                        remaining_processing_time =
                            parent->machines[machine_id].remaining_processing_time - processing_time;
                        child->weighted_idle_time += (previous_completion_time == 0)? 1:
                            (double)machine_idle_time / previous_completion_time;
                        child->weighted_idle_time += (parent->machines[machine_id].time_backward == 0)? 1:
                            (double)parent->machines[machine_id].idle_time_backward
                            / parent->machines[machine_id].time_backward;
                    }
                    Time processing_time_last = job_next.operations[last_machine_id].alternatives[0].processing_time;
                    Time machine_idle_time = parent->machines[last_machine_id].idle_time_forward;
                    Time idle_time = previous_completion_time - parent->machines[last_machine_id].time_forward;
                    machine_idle_time += idle_time;
                    child->idle_time += idle_time;
                    remaining_processing_time =
                        parent->machines[last_machine_id].remaining_processing_time - processing_time_last;
                    child->weighted_idle_time += (previous_completion_time + processing_time_last == 0)? 1:
                        (double)machine_idle_time / (previous_completion_time + processing_time_last);
                    child->weighted_idle_time += (parent->machines[last_machine_id].time_backward == 0)? 1:
                        (double)parent->machines[last_machine_id].idle_time_backward
                        / parent->machines[last_machine_id].time_backward;
                    child->bound = std::max(
                            child->bound,
                            previous_completion_time
                            + parent->machines[last_machine_id].remaining_processing_time
                            + parent->machines[last_machine_id].time_backward);
                }
            } else {
                MachineId last_machine_id = instance_.number_of_machines() - 1;
                if (!instance_.blocking()) {
                    Time processing_time_last = job_next.operations[last_machine_id].alternatives[0].processing_time;
                    previous_completion_time = parent->machines[last_machine_id].time_backward + processing_time_last;
                    Time remaining_processing_time =
                        parent->machines[last_machine_id].remaining_processing_time - processing_time_last;
                    child->weighted_idle_time += (parent->machines[last_machine_id].time_forward == 0)? 1:
                        (double)parent->machines[last_machine_id].idle_time_forward / parent->machines[last_machine_id].time_forward;
                    child->bound = std::max(child->bound,
                            parent->machines[last_machine_id].time_forward
                            + parent->machines[last_machine_id].remaining_processing_time
                            + parent->machines[last_machine_id].time_backward);
                    for (MachineId machine_id = last_machine_id - 1;
                            machine_id >= 0;
                            --machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        Time machine_idle_time = parent->machines[machine_id].idle_time_backward;
                        if (previous_completion_time > parent->machines[machine_id].time_backward) {
                            Time idle_time = previous_completion_time - parent->machines[machine_id].time_backward;
                            completion_time = previous_completion_time + processing_time;
                            machine_idle_time += idle_time;
                            child->idle_time += idle_time;
                        } else {
                            completion_time = parent->machines[machine_id].time_backward + processing_time;
                        }
                        remaining_processing_time =
                            parent->machines[machine_id].remaining_processing_time - processing_time;
                        child->weighted_idle_time += (parent->machines[machine_id].time_forward == 0)? 1:
                            (double)parent->machines[machine_id].idle_time_forward
                            / parent->machines[machine_id].time_forward;
                        child->weighted_idle_time += (completion_time == 0)? 1:
                            (double)machine_idle_time / completion_time;
                        child->bound = std::max(
                                child->bound,
                                parent->machines[machine_id].time_forward
                                + remaining_processing_time + completion_time);
                        previous_completion_time = completion_time;
                    }
                } else {
                    Time processing_time_last = job_next.operations[last_machine_id].alternatives[0].processing_time;
                    if (parent->machines[last_machine_id].time_backward + processing_time_last > parent->machines[last_machine_id - 1].time_backward) {
                        previous_completion_time = parent->machines[last_machine_id].time_backward + processing_time_last;
                    } else {
                        previous_completion_time = parent->machines[last_machine_id - 1].time_backward;
                    }
                    Time remaining_processing_time =
                        parent->machines[last_machine_id].remaining_processing_time - processing_time_last;
                    child->weighted_idle_time += (parent->machines[last_machine_id].time_forward == 0)? 1:
                        (double)parent->machines[last_machine_id].idle_time_forward / parent->machines[last_machine_id].time_forward;
                    child->bound = std::max(child->bound,
                            parent->machines[last_machine_id].time_forward
                            + parent->machines[last_machine_id].remaining_processing_time - processing_time_last
                            + previous_completion_time);
                    for (MachineId machine_id = last_machine_id - 1;
                            machine_id > 0;
                            --machine_id) {
                        Time processing_time = job_next.operations[machine_id].alternatives[0].processing_time;
                        Time machine_idle_time = parent->machines[machine_id].idle_time_backward;
                        Time idle_time;
                        Time t_departure;
                        if (previous_completion_time + processing_time > parent->machines[machine_id - 1].time_backward) {
                            idle_time = previous_completion_time - parent->machines[machine_id].time_backward;
                            t_departure = previous_completion_time + processing_time;
                        } else {
                            idle_time = parent->machines[machine_id - 1].time_backward
                                - parent->machines[machine_id].time_backward - processing_time;
                            t_departure = parent->machines[machine_id - 1].time_backward;
                        }
                        child->bound = std::max(
                                child->bound,
                                parent->machines[machine_id].time_forward
                                + parent->machines[machine_id].remaining_processing_time - processing_time
                                + t_departure);
                        machine_idle_time += idle_time;
                        child->idle_time += idle_time;
                        previous_completion_time = t_departure;
                        remaining_processing_time =
                            parent->machines[machine_id].remaining_processing_time - processing_time;
                        child->weighted_idle_time += (parent->machines[machine_id].time_forward == 0)? 1:
                            (double)parent->machines[machine_id].idle_time_forward
                            / parent->machines[machine_id].time_forward;
                        child->weighted_idle_time += (previous_completion_time == 0)? 1:
                            (double)machine_idle_time / previous_completion_time;
                    }
                    Time processing_time_0 = job_next.operations[0].alternatives[0].processing_time;
                    Time machine_idle_time = parent->machines[0].idle_time_backward;
                    Time idle_time = previous_completion_time - parent->machines[0].time_backward;
                    machine_idle_time += idle_time;
                    child->idle_time += idle_time;
                    remaining_processing_time =
                        parent->machines[0].remaining_processing_time - processing_time_0;
                    child->weighted_idle_time += (parent->machines[0].time_forward == 0)? 1:
                        (double)parent->machines[0].idle_time_forward
                        / parent->machines[0].time_forward;
                    child->weighted_idle_time += (previous_completion_time + processing_time_0 == 0)? 1:
                        (double)machine_idle_time / (previous_completion_time + processing_time_0);
                    child->bound = std::max(
                            child->bound,
                            parent->machines[0].time_forward
                            + parent->machines[0].remaining_processing_time
                            + previous_completion_time);
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
                if (best_node_->number_of_jobs != instance_.number_of_jobs()) {
                    child->guide = (1 - alpha) * child->bound
                        + alpha * child->weighted_idle_time;
                } else if (child->bound >= best_node_->bound) {
                    // The child will be pruned. Avoid dividing by zero.
                    child->guide = std::numeric_limits<double>::infinity();
                } else {
                    Time gap = best_node_->bound - child->bound;
                    double bound_weight = (double)best_node_->bound / gap;
                    double idle_time_weight = (double)gap / best_node_->bound;
                    child->guide = bound_weight * child->bound
                        + idle_time_weight * child->weighted_idle_time;
                }
                break;
            } default: {
            }
            }
            if (better(child, best_node_))
                best_node_ = child;
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

    treesearchsolver::IterativeBeamSearch2Parameters<BranchingSchemeBidirectional> beam_search_parameters;
    beam_search_parameters.verbosity_level = 0;
    beam_search_parameters.timer = parameters.timer;
    beam_search_parameters.new_solution_callback
        = [&instance, &algorithm_formatter](
                const treesearchsolver::Output<BranchingSchemeBidirectional>& tree_search_output)
        {
            const auto& beam_search_output = static_cast<const treesearchsolver::IterativeBeamSearch2Output<BranchingSchemeBidirectional>&>(tree_search_output);
            auto node = tree_search_output.solution_pool.best();
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
            std::vector<Time> machines_current_departure_times(instance.number_of_machines(), 0);
            for (JobId job_id: jobs_forward) {
                const Job& job = instance.job(job_id);
                std::vector<Time> next_departure_times(instance.number_of_machines(), 0);
                Time previous_completion_time = 0;

                if (!instance.blocking()) {
                    for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                        Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                        Time machine_free_time = machines_current_departure_times[machine_id];
                        Time start = std::max(previous_completion_time, machine_free_time);
                        solution_builder.append_operation(
                                job_id,
                                machine_id,  // operation_id
                                0,  // operation_machine_id
                                start);
                        Time finish_processing_time = start + processing_time;
                        previous_completion_time = finish_processing_time;
                        next_departure_times[machine_id] = previous_completion_time;
                    }
                } else {
                    for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                        Time processing_time = job.operations[machine_id].alternatives[0].processing_time;
                        Time machine_free_time = machines_current_departure_times[machine_id];
                        Time start = std::max(previous_completion_time, machine_free_time);
                        solution_builder.append_operation(
                                job_id,
                                machine_id,  // operation_id
                                0,  // operation_machine_id
                                start);
                        Time finish_processing_time = start + processing_time;
                        if (machine_id < instance.number_of_machines() - 1) {
                            previous_completion_time = std::max(
                                    finish_processing_time,
                                    machines_current_departure_times[machine_id + 1]);
                        } else {
                            previous_completion_time = finish_processing_time;
                        }
                        next_departure_times[machine_id] = previous_completion_time;
                    }

                }
                machines_current_departure_times = next_departure_times;
            }
            Solution solution = solution_builder.build();
            std::stringstream ss;
            ss << "queue " << beam_search_output.maximum_size_of_the_queue;
            algorithm_formatter.update_solution(solution, ss.str());
        };
    auto tree_search_output = treesearchsolver::iterative_beam_search_2(branching_scheme, beam_search_parameters);

    if (tree_search_output.optimal) {
        algorithm_formatter.update_makespan_bound(
                output.solution.makespan(), "tree search completed");
    }

    algorithm_formatter.end();
    return output;
}
