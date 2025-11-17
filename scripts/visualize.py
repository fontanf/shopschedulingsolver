import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import plotly
import json


if __name__ == "__main__":

    # Parse command line arguments.
    import argparse
    parser = argparse.ArgumentParser(description='')
    parser.add_argument(
            "certificate",
            type=str,
            help='Certificate file')
    args = parser.parse_args()

    color_list = px.colors.qualitative.Plotly
    d = []
    with open(args.certificate, 'r') as f:
        j = json.load(f)
        for operation_id, operation in enumerate(j["operations"]):
            label = ("(" + str(operation["job_id"])
                     + "," + str(operation["operation_id"])
                     + "," + str(operation["alternative_id"]) + ")")
            d.append(dict(
                machine_id=operation["machine_id"],
                job_id=operation["job_id"],
                operation_id=operation["operation_id"],
                alternative_id=operation["alternative_id"],
                start=operation["start"],
                processing_time=operation["processing_time"],
                end=operation["end"],
                label=label))

    df = pd.DataFrame(d)
    df["job_id"] = df["job_id"].astype(str)

    fig = px.timeline(
            df,
            x_start="start",
            x_end="end",
            y="machine_id",
            color="job_id",
            text="label",
            color_discrete_sequence=color_list,
            height=60*j["number_of_machines"],
            hover_data=[
                "job_id",
                "operation_id",
                "alternative_id",
                "start",
                "processing_time",
                "end"])
    fig.update_traces(textposition="inside")

    fig.update_layout(
        yaxis = dict(
            dtick = 1
            )
        )

    fig.update_yaxes(autorange="reversed")
    fig.layout.xaxis.type = 'linear'

    fig.update_yaxes(
        categoryorder="array",
        categoryarray=list(range(j["number_of_machines"]))
    )
    fig.update_layout(
        xaxis_title="Time",
        yaxis_title="Machines",
        legend_title="Jobs",
    )

    # Compute bar widths manually (important for numeric schedules)
    df['delta'] = df['end'] - df['start']
    for trace in fig.data:
        filt = df['job_id'] == trace.name
        trace.x = df[filt]['delta'].tolist()
        trace.base = df[filt]['start'].tolist()   # anchor bars at start times

    fig.show()
