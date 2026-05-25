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


if benchmark == "pfss_blocking_makespan":

    datacsv_path = os.path.join("data", "data_pfss_blocking_makespan.csv")
    data_dir = os.path.dirname(os.path.realpath(datacsv_path))

    with open(datacsv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)

        # Columns from the CSV that hold reference algorithm solution values.
        ref_value_columns = [
            f for f in reader.fieldnames
            if " / Best" in f or " / Avg" in f]

        # Build output fieldnames: keep all CSV columns, add RPD for each
        # result column (reference + new runs), then append new run columns.
        out_fieldnames = list(reader.fieldnames)
        for col in ref_value_columns:
            out_fieldnames.append(col.replace(" / ", " / RPD "))
        for output_directory_run in output_directories:
            out_fieldnames.append(output_directory_run + " / Solution value")
            out_fieldnames.append(output_directory_run + " / RPD")

        # All columns that hold a solution value (for highlighting).
        solution_value_columns = (
            ref_value_columns
            + [od + " / Solution value" for od in output_directories])

        # Group-level summary rows: one per (n × m) group, plus Total.
        group_keys = [
            (20, 5), (20, 10), (20, 20),
            (50, 5), (50, 10), (50, 20),
            (100, 5), (100, 10), (100, 20),
            (200, 10), (200, 20),
            (500, 20),
        ]
        def make_extra_row(label):
            r = {f: "" for f in out_fieldnames}
            r["Path"] = label
            r[bksv_field] = 0
            for c in solution_value_columns:
                r[c] = 0
                rpd_col = c.replace(" / Best", " / RPD Best").replace(
                    " / Avg", " / RPD Avg").replace(
                    " / Solution value", " / RPD")
                if rpd_col in out_fieldnames:
                    r[rpd_col] = 0.0
            return r
        group_rows = {gk: make_extra_row(f"{gk[0]}×{gk[1]}") for gk in group_keys}
        total_row = make_extra_row("Total")

        out_rows = []

        for row in reader:
            bks = int(row[bksv_field])
            row[bksv_field] = bks

            # Parse n and m from path: taillard1993/tai{n}_{m}_{idx}.txt
            import re as _re
            m_path = _re.search(r'tai(\d+)_(\d+)_', row["Path"])
            n_jobs = int(m_path.group(1)) if m_path else 0
            n_mach = int(m_path.group(2)) if m_path else 0
            group_key = (n_jobs, n_mach)

            # Reference algorithm columns: convert to int, compute RPD.
            for col in ref_value_columns:
                try:
                    val = int(float(row[col]))
                except:
                    val = None
                row[col] = val if val is not None else ""
                rpd_col = col.replace(" / ", " / RPD ")
                if val is not None and bks > 0:
                    rpd = (val - bks) / bks * 100
                    row[rpd_col] = round(rpd, 3)
                    if group_key in group_rows:
                        group_rows[group_key][col] = (
                            group_rows[group_key].get(col) or 0) + val
                        group_rows[group_key][rpd_col] = round(
                            (group_rows[group_key].get(rpd_col) or 0.0) + rpd, 3)
                    total_row[col] = (total_row.get(col) or 0) + val
                    total_row[rpd_col] = round(
                        (total_row.get(rpd_col) or 0.0) + rpd, 3)
                else:
                    row[rpd_col] = ""

            # Update group and total BKS sums.
            if group_key in group_rows:
                group_rows[group_key][bksv_field] += bks
            total_row[bksv_field] += bks

            # New run output directories.
            for output_directory_run in output_directories:
                val_col = output_directory_run + " / Solution value"
                rpd_col = output_directory_run + " / RPD"
                json_path = os.path.join(
                    benchmark_directory,
                    output_directory_run,
                    row["Path"] + "_output.json")
                try:
                    with open(json_path) as jf:
                        jd = json.load(jf)
                    val = jd["Output"]["Solution"]["Makespan"]
                except:
                    val = None
                row[val_col] = val if val is not None else ""
                if val is not None and bks > 0:
                    rpd = (val - bks) / bks * 100
                    row[rpd_col] = round(rpd, 3)
                    if group_key in group_rows:
                        group_rows[group_key][val_col] = (
                            group_rows[group_key].get(val_col) or 0) + val
                        group_rows[group_key][rpd_col] = round(
                            (group_rows[group_key].get(rpd_col) or 0.0) + rpd, 3)
                    total_row[val_col] = (total_row.get(val_col) or 0) + val
                    total_row[rpd_col] = round(
                        (total_row.get(rpd_col) or 0.0) + rpd, 3)
                else:
                    row[rpd_col] = ""

            out_rows.append(row)

        # Append group and total summary rows.
        for gk in group_keys:
            out_rows.append(group_rows[gk])
        out_rows.append(total_row)

        df = pd.DataFrame.from_records(out_rows, columns=out_fieldnames)

        def highlight(s):
            styles = []
            for col in out_fieldnames:
                if col in solution_value_columns:
                    try:
                        val = int(s[col])
                        bks_val = int(s[bksv_field])
                        if val == bks_val:
                            styles.append('background-color: lightgreen')
                        elif val > bks_val:
                            styles.append('background-color: pink')
                        else:
                            styles.append('background-color: yellow')
                    except:
                        styles.append('')
                else:
                    styles.append('')
            return styles

        df = df.style.apply(highlight, axis=1)
        show_datafram(df)


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
