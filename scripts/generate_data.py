import os
import contextlib
import sys

generator_main = os.path.join(
        "install",
        "bin",
        "shopschedulingsolver_generator")

shop_types = [
        ("permutation_flow_shop", "pfss"),
        ("flow_shop", "fss"),
        ("flexible_flow_shop", "ffss"),
        ("job_shop", "jss"),
        ("flexible_job_shop", "fjss"),
        ("open_shop", "oss"),
        ("flexible_open_shop", "foss")]
objectives = [
        ("makespan", "makespan"),
        ("total_flow_time", "tft"),
        ("total_weighted_flow_time", "twft"),
        ("total_tardiness", "tt"),
        ("total_weighted_tardiness", "twt"),
        ("throughput", "throughput")]
properties = ["", "no_wait", "no_idle", "blocking"]

instances_paths = {}
for shop_type, shop_type_short in shop_types:
    for objective, objective_short in objectives:
        for prop in properties:
            instances_path = os.path.join("data", "test_" + shop_type_short)
            instances_path += "_" + objective_short
            if prop != "":
                instances_path += "_" + prop
            instances_path += ".txt"
            instances_paths[(shop_type, objective, prop)] = instances_path

with contextlib.ExitStack() as stack:
    files = {
            instances_path:
            stack.enter_context(open(instances_path, 'w'))
            for instances_path in instances_paths.values()}

    for shop_type, shop_type_short in shop_types:
        for objective, objective_short in objectives:
            for prop in properties:
                for number_of_jobs in [1, 2, 3, 4, 5, 6, 7, 8]:
                    for number_of_machine_groups in [1, 2, 3]:
                        for number_of_machines_per_groups in [1, 2]:
                            if number_of_machines_per_groups != 1:
                                if shop_type_short not in ["ffss", "fjss", "foss"]:
                                    continue
                            for seed in [0, 1]:
                                instances_path = instances_paths[(shop_type, objective, prop)]

                                shop_type_str = shop_type
                                if number_of_machines_per_groups != 1:
                                    shop_type_str = f"flexible_{shop_type_str}"

                                prop_str = ""
                                if prop != "":
                                    prop_str = "_" + prop

                                instance_base_path = os.path.join(
                                        "tests",
                                        shop_type,
                                        objective + prop_str,
                                        shop_type_short
                                        + "_" + objective_short + prop_str
                                        + f"_n{number_of_jobs}"
                                        f"_m{number_of_machine_groups}"
                                        f"x{number_of_machines_per_groups}"
                                        f"_s{seed}"
                                        f".json")
                                instance_full_path = os.path.join("data", instance_base_path)
                                if not os.path.exists(os.path.dirname(instance_full_path)):
                                    os.makedirs(os.path.dirname(instance_full_path))

                                weight_range = 1
                                if objective_short in ["twft", "twt", "throughput"]:
                                    weight_range = 100

                                command = generator_main
                                if shop_type_short in ["oss", "foss"]:
                                    command += "  --operations-arbitrary-order 1"
                                command += "  --objective " + objective.replace("_weighted", "").replace("_", "-")
                                if shop_type_short == "pfss":
                                    command += "  --permutation 1"
                                if prop == "no_wait":
                                    command += "  --no-wait 1"
                                if prop == "no_idle":
                                    command += "  --no-idle 1"
                                if prop == "blocking":
                                    command += "  --blocking 1"
                                command += f"  --number-of-jobs \"{number_of_jobs}\""
                                command += f"  --number-of-machine-groups \"{number_of_machine_groups}\""
                                command += f"  --number-of-machines-per-group \"{number_of_machines_per_groups}\""
                                command += f"  --number-of-operations-per-job \"{number_of_machine_groups}\""
                                command += f"  --processing-times-range 100"
                                command += f"  --weights-range {weight_range}"
                                command += f"  --due-date-tightness-factor 3"
                                command += f"  --seed {seed}"
                                command += f"  --output \"{instance_full_path}\""
                                print(command)
                                status = os.system(command)
                                if status != 0:
                                    sys.exit(1)

                                files[instances_path].write(f"{instance_base_path}\n")
