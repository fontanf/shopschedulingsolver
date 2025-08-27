import os
import sys


def run_command(command):
    print(command)
    status = os.system(command)
    if status != 0:
        sys.exit(1)
    print()


main = os.path.join(
        "install",
        "bin",
        "shopschedulingsolver_mps_writer")


if __name__ == "__main__":

    for i in range(1, 81):

        instance_path = os.path.join(
                "data",
                "taillard1993_jss",
                f"ta{i:02}")

        output_path_1 = os.path.join(
                "mps",
                "taillard1993_jss",
                f"ta{i:02}_jss.mps")
        output_path_2 = os.path.join(
                "mps",
                "taillard1993_jss",
                f"ta{i:02}_oss.mps")
        if not os.path.exists(os.path.dirname(output_path_1)):
            os.makedirs(os.path.dirname(output_path_1))

        command = (
                f"{main}"
                f"  --input \"{instance_path}\""
                f" --format job-shop"
                f" --objective makespan"
                f"  --solver highs"
                f"  --output \"{output_path_1}\"")
        run_command(command)
        command = (
                f"{main}"
                f"  --input \"{instance_path}\""
                f" --format job-shop"
                f" --objective makespan"
                " --operations-arbitrary-order 1"
                f"  --solver highs"
                f"  --output \"{output_path_2}\"")
        run_command(command)
