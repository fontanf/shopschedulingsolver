import streamlit as st
import argparse
import os
import csv
import json
import datetime
import pandas as pd
import pathlib


st.set_page_config(layout='wide')
st.title("ShopSchedulingSolver benchmarks analyzer")

def show_datafram(df):
    st.dataframe(
            df,
            use_container_width=True,
            height=(len(df.index) + 1) * 35 + 3)

benchmarks = [
    f
    for f in os.listdir("benchmark_results")
    if os.path.isdir(os.path.join("benchmark_results", f))]
benchmarks.sort()

benchmark = st.selectbox(
    "Benchmark",
    benchmarks)

benchmark_directory = os.path.join(
        "benchmark_results",
        benchmark)

output_directories = [
    f
    for f in os.listdir(benchmark_directory)
    if os.path.isdir(os.path.join(benchmark_directory, f))]
output_directories.sort()

bksv_field = "Best known solution value"


if benchmark in ["pfss_makespan_vallada2015_small",
                 "pfss_makespan_vallada2015_large"]:

    datacsv_path = os.path.join(
            "data",
            "data_pfss_makespan.csv")

    data_dir = os.path.dirname(os.path.realpath(datacsv_path))
    with open(datacsv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)

        # Get fieldnames of CSV output file.
        out_fieldnames = reader.fieldnames
        for output_directory in output_directories:
            out_fieldnames.append(output_directory + " / Solution value")

        result_columns = [fieldname for fieldname in reader.fieldnames
                          if "Solution value" in fieldname]

        # Add gap columns.
        out_fieldnames_tmp = []
        for fieldname in out_fieldnames:
            out_fieldnames_tmp.append(fieldname)
            if "Solution value" in fieldname:
                out_fieldnames_tmp.append(
                        fieldname.replace("Solution value", "Gap"))
        out_fieldnames = out_fieldnames_tmp

        out_rows = []

        # Initialize extra rows.
        jobs = []
        machines = []
        if benchmark == "pfss_makespan_vallada2015_small":
            jobs = [10, 20, 30, 40, 50, 60]
            machines = [5, 10, 15, 20]
        elif benchmark == "pfss_makespan_vallada2015_large":
            jobs = [100, 200, 300, 400, 500, 600, 700, 800]
            machines = [20, 40, 60]
        extra_rows = [
                {
                    "Path": (str(n) + "_" + str(m)),
                    bksv_field: 0,
                }
                for n in jobs
                for m in machines
                ] + [{
                        "Path": "Total",
                        bksv_field: 0}]
        for fieldname in [bksv_field] + result_columns:
            for row in extra_rows:
                row[fieldname] = 0
                row[fieldname.replace("Solution value", "Gap")] = 0

        for row in reader:

            if benchmark == "pfss_makespan_vallada2015_small":
                if row["Dataset"] != "vallada2015_small":
                    continue
            if benchmark == "pfss_makespan_vallada2015_large":
                if row["Dataset"] != "vallada2015_large":
                    continue

            row[bksv_field] = int(row[bksv_field])

            # Fill current row.
            for output_directory in output_directories:
                json_output_path = os.path.join(
                        benchmark_directory,
                        output_directory,
                        row["Path"] + "_output.json")
                try:
                    json_output_file = open(json_output_path, "r")
                    json_data = json.load(json_output_file)
                    row[output_directory + " / Solution value"] = (
                            json_data["Output"]["Solution"]["Makespan"])
                except:
                    row[output_directory + " / Solution value"] = 9999999

            # Get extra rows to update.
            row_id = 0
            # Get extra rows to update.
            n = int(row["Number of jobs"])
            m = int(row["Number of machines"])
            extra_rows_to_update = [
                    (jobs.index(n) * len(machines) + machines.index(m)),
                    -1]

            # Update "Best known solution value" column of extra rows.
            for row_id in extra_rows_to_update:
                extra_rows[row_id][bksv_field] += row[bksv_field]

            # Update result columns of extra rows.
            for result_column in result_columns:
                try:
                    solution_value = int(row[result_column])
                except:
                    solution_value = 9999999
                row[result_column] = solution_value
                for row_id in extra_rows_to_update:
                    extra_rows[row_id][result_column] += solution_value

                # Compute gap.
                gap = (solution_value - row[bksv_field]) / row[bksv_field] * 100
                gap_column = result_column.replace("Solution value", "Gap")
                row[gap_column] = gap
                for row_id in extra_rows_to_update:
                    extra_rows[row_id][gap_column] += gap

            # Add current row.
            out_rows.append(row)

        # Add extra rows.
        for row in extra_rows:
            out_rows.append(row)

        df = pd.DataFrame.from_records(out_rows, columns=out_fieldnames)

        def highlight(s):
            return [('background-color: lightgreen'
                     if s[fieldname] == s[bksv_field]
                     else ('background-color: pink'
                           if s[fieldname] > s[bksv_field]
                           else 'background-color: yellow'))
                    if fieldname in result_columns
                    else ''
                    for fieldname in out_fieldnames]
        df = df.style.apply(highlight, axis = 1)
        show_datafram(df)


if benchmark == "pfss_tct":

    datacsv_path = os.path.join(
            "data",
            "data_pfss_tct.csv")

    data_dir = os.path.dirname(os.path.realpath(datacsv_path))
    with open(datacsv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)

        # Get fieldnames of CSV output file.
        out_fieldnames = reader.fieldnames
        for output_directory in output_directories:
            out_fieldnames.append(output_directory + " / Solution value")

        result_columns = [fieldname for fieldname in reader.fieldnames
                          if "Solution value" in fieldname]

        # Add gap columns.
        out_fieldnames_tmp = []
        for fieldname in out_fieldnames:
            out_fieldnames_tmp.append(fieldname)
            if "Solution value" in fieldname:
                out_fieldnames_tmp.append(
                        fieldname.replace("Solution value", "Gap"))
        out_fieldnames = out_fieldnames_tmp

        out_rows = []

        # Initialize extra rows.
        extra_rows = [{"Path": "Total", bksv_field: 0}]
        for fieldname in [bksv_field] + result_columns:
            for row in extra_rows:
                row[fieldname] = 0
                row[fieldname.replace("Solution value", "Gap")] = 0

        for row in reader:

            row[bksv_field] = int(row[bksv_field])

            # Fill current row.
            for output_directory in output_directories:
                json_output_path = os.path.join(
                        benchmark_directory,
                        output_directory,
                        row["Path"] + "_output.json")
                try:
                    json_output_file = open(json_output_path, "r")
                    json_data = json.load(json_output_file)
                    row[output_directory + " / Solution value"] = (
                            json_data["Output"]["Solution"]["TotalFlowTime"])
                except:
                    row[output_directory + " / Solution value"] = 9999999

            # Get extra rows to update.
            row_id = 0

            # Update "Best known solution value" column of extra rows.
            extra_rows[row_id][bksv_field] += row[bksv_field]

            # Update result columns of extra rows.
            for result_column in result_columns:
                solution_value = int(row[result_column])
                row[result_column] = solution_value
                extra_rows[row_id][result_column] += solution_value

                # Compute gap.
                gap = (solution_value - row[bksv_field]) / row[bksv_field] * 100
                gap_column = result_column.replace("Solution value", "Gap")
                row[gap_column] = gap
                extra_rows[row_id][gap_column] += gap

            # Add current row.
            out_rows.append(row)

        # Add extra rows.
        for row in extra_rows:
            out_rows.append(row)

        df = pd.DataFrame.from_records(out_rows, columns=out_fieldnames)

        def highlight(s):
            return [('background-color: lightgreen'
                     if s[fieldname] == s[bksv_field]
                     else ('background-color: pink'
                           if s[fieldname] > s[bksv_field]
                           else 'background-color: yellow'))
                    if fieldname in result_columns
                    else ''
                    for fieldname in out_fieldnames]
        df = df.style.apply(highlight, axis = 1)
        show_datafram(df)


if benchmark == "pfss_tt":

    datacsv_path = os.path.join(
            "data",
            "data_pfss_tt.csv")

    data_dir = os.path.dirname(os.path.realpath(datacsv_path))
    with open(datacsv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)

        # Get fieldnames of CSV output file.
        out_fieldnames = reader.fieldnames
        for output_directory in output_directories:
            out_fieldnames.append(output_directory + " / Solution value")

        result_columns = [fieldname for fieldname in reader.fieldnames
                          if "Solution value" in fieldname]

        # Add gap columns.
        out_fieldnames_tmp = []
        for fieldname in out_fieldnames:
            out_fieldnames_tmp.append(fieldname)
            if "Solution value" in fieldname:
                out_fieldnames_tmp.append(
                        fieldname.replace("Solution value", "Gap"))
        out_fieldnames = out_fieldnames_tmp

        out_rows = []

        # Initialize extra rows.
        extra_rows = [{"Path": "Total", bksv_field: 0}]
        for fieldname in [bksv_field] + result_columns:
            for row in extra_rows:
                row[fieldname] = 0
                row[fieldname.replace("Solution value", "Gap")] = 0

        for row in reader:

            row[bksv_field] = int(row[bksv_field])

            # Fill current row.
            for output_directory in output_directories:
                json_output_path = os.path.join(
                        benchmark_directory,
                        output_directory,
                        row["Path"] + "_output.json")
                try:
                    json_output_file = open(json_output_path, "r")
                    json_data = json.load(json_output_file)
                    row[output_directory + " / Solution value"] = (
                            json_data["Output"]["Solution"]["TotalTardiness"])
                except:
                    row[output_directory + " / Solution value"] = 9999999

            # Get extra rows to update.
            row_id = 0

            # Update "Best known solution value" column of extra rows.
            extra_rows[row_id][bksv_field] += row[bksv_field]

            # Update result columns of extra rows.
            for result_column in result_columns:
                solution_value = int(row[result_column])
                row[result_column] = solution_value
                extra_rows[row_id][result_column] += solution_value

                # Compute gap.
                gap = (solution_value - row[bksv_field]) / row[bksv_field] * 100
                gap_column = result_column.replace("Solution value", "Gap")
                row[gap_column] = gap
                extra_rows[row_id][gap_column] += gap

            # Add current row.
            out_rows.append(row)

        # Add extra rows.
        for row in extra_rows:
            out_rows.append(row)

        df = pd.DataFrame.from_records(out_rows, columns=out_fieldnames)

        def highlight(s):
            return [('background-color: lightgreen'
                     if s[fieldname] == s[bksv_field]
                     else ('background-color: pink'
                           if s[fieldname] > s[bksv_field]
                           else 'background-color: yellow'))
                    if fieldname in result_columns
                    else ''
                    for fieldname in out_fieldnames]
        df = df.style.apply(highlight, axis = 1)
        show_datafram(df)
