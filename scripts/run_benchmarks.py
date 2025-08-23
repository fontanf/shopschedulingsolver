import os
import csv
import argparse
import sys
import datetime


def run_command(command):
    print(command)
    status = os.system(command)
    if status != 0:
        sys.exit(1)
    print()


main = os.path.join(
        "install",
        "bin",
        "shopschedulingsolver")


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
            prog='run_benchmarks',
            usage='%(prog)s [options]')
    parser.add_argument('benchmark', help='benchmark to run')
    parser.add_argument('algorithm', help='algorithm')
    parser.add_argument(
            '--directory',
            help='benchmark directory',
            default="benchmark_results")
    parser.add_argument('--name', help='output directory', default=None)
    parser.add_argument('--options', help='options', default="")
    args = parser.parse_args()

    benchmark = args.benchmark
    options = args.options
    output_directory = os.path.join(args.directory, benchmark)
    if args.name:
        output_directory = os.path.join(output_directory, args.name)
    else:
        date = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        output_directory = os.path.join(
                output_directory,
                date + "_" + benchmark + "_" + args.algorithm)


    if benchmark == "pfss_makespan":

        datacsv_path = os.path.join(
                "data",
                "data_pfss_makespan.csv")

        data_dir = os.path.dirname(os.path.realpath(datacsv_path))
        with open(datacsv_path, newline='') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:

                instance_path = os.path.join(
                        data_dir,
                        row["Path"])

                objective = row["Objective"]

                time_limit = float(row["Time limit"])

                json_output_path = os.path.join(
                        output_directory,
                        row["Path"] + "_output.json")
                if not os.path.exists(os.path.dirname(json_output_path)):
                    os.makedirs(os.path.dirname(json_output_path))

                certificate_path = os.path.join(
                        output_directory,
                        row["Path"] + "_solution.csv")
                if not os.path.exists(os.path.dirname(certificate_path)):
                    os.makedirs(os.path.dirname(certificate_path))

                instance_format = row["Format"]
                command = (
                        f"{main}"
                        f"  --verbosity-level 1"
                        f"  --input \"{instance_path}\""
                        f" --objective \"{objective}\""
                        f" --format \"{instance_format}\""
                        f"  --algorithm {args.algorithm}"
                        f" --time-limit {time_limit}"
                        f"  --output \"{json_output_path}\""
                        f" --certificate \"{certificate_path}\"")
                run_command(command)


    if benchmark == "pfss_tct":

        datacsv_path = os.path.join(
                "data",
                "data_pfss_tct.csv")

        data_dir = os.path.dirname(os.path.realpath(datacsv_path))
        with open(datacsv_path, newline='') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:

                instance_path = os.path.join(
                        data_dir,
                        row["Path"])

                objective = row["Objective"]

                time_limit = float(row["Time limit"])

                json_output_path = os.path.join(
                        output_directory,
                        row["Path"] + "_output.json")
                if not os.path.exists(os.path.dirname(json_output_path)):
                    os.makedirs(os.path.dirname(json_output_path))

                certificate_path = os.path.join(
                        output_directory,
                        row["Path"] + "_solution.csv")
                if not os.path.exists(os.path.dirname(certificate_path)):
                    os.makedirs(os.path.dirname(certificate_path))

                instance_format = row["Format"]
                command = (
                        f"{main}"
                        f"  --verbosity-level 1"
                        f"  --input \"{instance_path}\""
                        f" --objective \"{objective}\""
                        f" --format \"{instance_format}\""
                        f"  --algorithm {args.algorithm}"
                        f" --time-limit {time_limit}"
                        f"  --output \"{json_output_path}\""
                        f" --certificate \"{certificate_path}\"")
                run_command(command)
