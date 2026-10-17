import argparse
import sys
import os
import json


def run_command(command):
    print(command)
    status = os.system(command)
    if status != 0:
        sys.exit(1)
    print()


parser = argparse.ArgumentParser(description='')
parser.add_argument(
        "--algorithm",
        type=str,
        required=True,
        help='')
parser.add_argument(
        "--instances",
        type=str,
        nargs='+',
        required=True,
        help='')
parser.add_argument(
        "--output",
        type=str,
        required=True,
        help='')

args = parser.parse_args()


main = os.path.join(
        "install",
        "bin",
        "shopschedulingsolver")

with open(args.output, 'w') as output_file:
    for instances_path in args.instances:
        with open(instances_path, 'r') as instances_file:
            for line in instances_file:
                instance_base_path = line[:-1]
                instance_full_path = os.path.join("data", instance_base_path)
                json_output_path = "output.json"
                command = (
                        main
                        + f"  --verbosity-level 1"
                        + f"  --input \"{instance_full_path}\""
                        " --format json"
                        + f"  --algorithm {args.algorithm}"
                        + f"  --output \"{json_output_path}\""
                        + f"  --time-limit 1")
                run_command(command)

                # If solved to optimality, add the instance and its optimal
                # objective value to the output file.
                with open(json_output_path, "r") as json_output_file:
                    json_data = json.load(json_output_file)
                if not bool(json_data["Output"]["Optimal"]):
                    continue
                bound = int(json_data["Output"]["Bound"])
                value = int(json_data["Output"]["Value"])
                if bound == value:
                    output_file.write(f"{instance_base_path} {value}\n")
