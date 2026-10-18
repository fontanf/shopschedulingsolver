#include "shopschedulingsolver/algorithms/constraint_programming_optalcp.hpp"

#include "shopschedulingsolver/solution_builder.hpp"

#include <boost/process.hpp>
#include <boost/process/extend.hpp>

#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace shopschedulingsolver;

namespace bp = boost::process;

namespace
{

/**
 * Get the path of the script which builds and solves the OptalCP model.
 *
 * It can be set through the environment variable
 * SHOPSCHEDULINGSOLVER_OPTALCP_SCRIPT; otherwise, the script compiled with
 * the library is used.
 */
std::string optalcp_script_path()
{
    const char* path = std::getenv("SHOPSCHEDULINGSOLVER_OPTALCP_SCRIPT");
    if (path != nullptr)
        return path;
    return OPTALCP_SCRIPT_PATH;
}

/**
 * Ignore SIGPIPE while an instance of this class exists.
 *
 * If the script exits early (e.g. if it fails to load), writing to its
 * standard input would otherwise kill the process without any error message.
 * The write fails instead, and the exit code of the script is reported.
 */
class IgnoreSigpipe
{

public:

#ifdef SIGPIPE
    IgnoreSigpipe(): previous_handler_(std::signal(SIGPIPE, SIG_IGN)) { }

    ~IgnoreSigpipe() { std::signal(SIGPIPE, previous_handler_); }

private:

    void (*previous_handler_)(int);
#endif

};

/**
 * Thread which sends "stop" to the script once the timer ends.
 *
 * The timer is checked periodically, so that the search is stopped even if
 * the script doesn't send any message (e.g. after a SIGINT).
 */
class StopThread
{

public:

    StopThread(
            const Parameters& parameters,
            bp::opstream& child_input):
        thread_([this, &parameters, &child_input]()
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    while (!finished_) {
                        if (parameters.timer.needs_to_end()) {
                            child_input << "stop" << std::endl;
                            return;
                        }
                        condition_variable_.wait_for(lock, std::chrono::milliseconds(100));
                    }
                }) { }

    /** Stop the thread; the standard input of the script can then be closed. */
    void finish()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
        }
        condition_variable_.notify_one();
        if (thread_.joinable())
            thread_.join();
    }

    ~StopThread() { finish(); }

private:

    std::mutex mutex_;

    std::condition_variable condition_variable_;

    bool finished_ = false;

    std::thread thread_;

};

void update_bound(
        const Instance& instance,
        AlgorithmFormatter& algorithm_formatter,
        Time bound)
{
    switch (instance.objective()) {
    case Objective::Makespan: {
        algorithm_formatter.update_makespan_bound(bound, "");
        break;
    } case Objective::TotalFlowTime: {
        algorithm_formatter.update_total_flow_time_bound(bound, "");
        break;
    } case Objective::Throughput: {
        algorithm_formatter.update_throughput_bound(bound, "");
        break;
    } case Objective::TotalTardiness: {
        algorithm_formatter.update_total_tardiness_bound(bound, "");
        break;
    }
    }
}

}

Output shopschedulingsolver::constraint_programming_optalcp(
        const Instance& instance,
        const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Constraint programming (OptalCP)");

    algorithm_formatter.print_header();

    // Run the script. It reads the instance and the parameters on its
    // standard input and writes one JSON message per line on its standard
    // output. Its standard error, which contains its error messages, is
    // inherited.
    // On POSIX systems, it is run in its own process group, so that a SIGINT
    // sent from the terminal (Ctrl-C) doesn't kill it; the search is stopped
    // through the timer instead.
    std::string script_path = optalcp_script_path();
    IgnoreSigpipe ignore_sigpipe;
    bp::opstream child_input;
    bp::ipstream child_output;
    bp::child child(
            bp::search_path("node"),
            script_path,
            bp::std_in < child_input,
            bp::std_out > child_output
#ifndef _WIN32
            , bp::extend::on_exec_setup([](auto&) { setpgid(0, 0); })
#endif
            );

    nlohmann::json input_json;
    input_json["instance"] = instance.to_json();
    input_json["parameters"] = parameters.to_json();
    input_json["solver_log_path"] = parameters.solver_log_path;
    child_input << input_json.dump() << std::endl;

    // From now on, only the stop thread writes to the standard input of the
    // script.
    StopThread stop_thread(parameters, child_input);

    // Process the messages of the script.
    bool end_received = false;
    std::string line;
    while (std::getline(child_output, line)) {
        nlohmann::json message;
        try {
            message = nlohmann::json::parse(line);
        } catch (const nlohmann::json::parse_error&) {
            // Ignore anything which is not a message of the script.
            continue;
        }

        const std::string type = message.value("type", "");
        if (type == "solution") {
            SolutionBuilder solution_builder;
            solution_builder.set_instance(instance);
            for (const auto& json_operation: message["operations"]) {
                solution_builder.append_operation(
                        json_operation["job_id"],
                        json_operation["operation_id"],
                        json_operation["alternative_id"],
                        json_operation["start"]);
            }
            solution_builder.sort_machines();
            solution_builder.sort_jobs();
            algorithm_formatter.update_solution(solution_builder.build(), "");
        } else if (type == "bound") {
            update_bound(instance, algorithm_formatter, message["value"]);
        } else if (type == "end") {
            end_received = true;
            break;
        }
    }

    // Closing the standard input of the script lets it exit.
    stop_thread.finish();
    child_input.pipe().close();
    child.wait();
    if (!end_received || child.exit_code() != 0) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": "
                "the OptalCP script \"" + script_path + "\" failed; "
                "exit code: " + std::to_string(child.exit_code()) + ".");
    }

    algorithm_formatter.end();
    return output;
}
