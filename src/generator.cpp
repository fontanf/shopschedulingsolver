#include "shopschedulingsolver/generator.hpp"

#include "shopschedulingsolver/instance_builder.hpp"

#include "optimizationtools/utils/utils.hpp"

using namespace shopschedulingsolver;

Instance shopschedulingsolver::generate(
        const GenerateInput& input,
        std::mt19937_64& generator)
{
    if (input.permutation && !input.flow_shop) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "permutation requires a flow shop environment.");
    }

    InstanceBuilder instance_builder;
    instance_builder.set_objective(input.objective);
    instance_builder.set_operations_arbitrary_order(input.operations_arbitrary_order);
    instance_builder.set_blocking(input.blocking);
    instance_builder.set_no_wait(input.no_wait);
    instance_builder.set_all_machines_no_idle(input.no_idle);
    instance_builder.set_permutation(input.permutation);
    MachineId number_of_machines = input.number_of_machine_groups * input.number_of_machines_per_group;
    instance_builder.set_number_of_machines(number_of_machines);
    instance_builder.add_jobs(input.number_of_jobs);

    // Add jobs.
    std::uniform_int_distribution<Time> distribution_p(1, input.processing_times_range);
    std::uniform_int_distribution<Time> distribution_w(1, input.weights_range);
    std::uniform_int_distribution<MachineId> distribution_m(0, input.number_of_machine_groups - 1);
    for (JobId job_id = 0; job_id < input.number_of_jobs; ++job_id) {
        Time processing_time_sum = 0;
        Time weight = distribution_w(generator);
        instance_builder.set_job_weight(job_id, weight);
        std::vector<MachineId> machine_groups_ids = optimizationtools::bob_floyd(
                input.number_of_operations_per_job,
                input.number_of_machine_groups,
                generator);
        if (!input.flow_shop)
            std::shuffle(machine_groups_ids.begin(), machine_groups_ids.end(), generator);
        for (OperationId operation_id = 0;
                operation_id < input.number_of_operations_per_job;
                ++operation_id) {
            instance_builder.add_operation(job_id);
            Time psum = 0;
            for (AlternativeId alternative_id = 0;
                    alternative_id < input.number_of_machines_per_group;
                    ++alternative_id) {
                MachineId machine_id = machine_groups_ids[operation_id] * input.number_of_machines_per_group + alternative_id;
                Time processing_time = distribution_p(generator);
                instance_builder.add_alternative(
                        job_id,
                        operation_id,
                        machine_id,
                        processing_time);
                psum += processing_time;
            }
            processing_time_sum += psum / input.number_of_machines_per_group;
        }
        // The due date of a job is set according to the rule of Eilon and
        // Hodgson (1967): d_j = floor(f * sum_i p_ij), where f is the due date
        // tightness factor. For flexible operations, the mean processing time
        // of the operation over its alternatives is used.
        // In "A neighborhood for complex job shop scheduling problems with
        // regular objectives" (Bürgy, 2017), f is set to 1.3 for the job shop,
        // 1.6 for the job shop with sequence-dependent setup times and 1.8
        // for the blocking job shop.
        // In "The no-wait job shop with regular objective: a method based on
        // optimal job insertion" (Bürgy and Gröflin, 2017), f grows with the
        // number of jobs: 3.0 for 20x15 and 20x20 instances, 3.5 for 20x5 and
        // 20x10, 5.0 for 30x10 and 7.0 for 50x10.
        // The test instances have at most 8 jobs; a factor of 1 is used so
        // that most of them have a positive optimal tardiness.
        Time due_date = input.due_date_tightness_factor * processing_time_sum;
        instance_builder.set_job_due_date(job_id, due_date);
    }

    return instance_builder.build();
}
