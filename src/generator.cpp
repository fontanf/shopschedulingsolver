#include "shopschedulingsolver/generator.hpp"

#include "shopschedulingsolver/instance_builder.hpp"

#include "optimizationtools/utils/utils.hpp"

using namespace shopschedulingsolver;

Instance shopschedulingsolver::generate(
        const GenerateInput& input,
        std::mt19937_64& generator)
{
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
                psum += processing_time_sum;
            }
            processing_time_sum += psum / input.number_of_machine_groups;
        }
        Time due_date = input.due_date_tightness_factor * processing_time_sum;
        instance_builder.set_job_due_date(job_id, due_date);
    }

    return instance_builder.build();
}
