import subprocess as sbp
import time
import os
import select

from utils import *

# Works with Python 3.9

# Save logs
def save_logs(scenario_filename, dir, out):
    tmp = scenario_filename.split('.')
    tmp[-1] = ".txt"
    log_name = ''.join(tmp)

    text = ""
    for line in out[:-1]:
        text += line + '\n'

    filename = dir + '/' + log_name
    if not os.path.exists(os.path.dirname(filename)):
        os.makedirs(os.path.dirname(filename))

    with open(filename, "w") as f:
        f.write(text)
        f.close()

# Shared routine
def launch_subprocess(nb_servers, nb_clients):
    run_cmd = "mpirun --oversubscribe -np {} ./../build/algorep --servers {} --clients {}".format(nb_servers + nb_clients + 1, nb_servers, nb_clients)

    os.system("rm -rf logs")

    return sbp.Popen(run_cmd.split(), stdin=sbp.PIPE, stdout=sbp.PIPE, stderr=sbp.PIPE, text=True)

def launch_scenario(scenario):
    process = launch_subprocess(scenario.nb_servers, scenario.nb_clients)

    for command in scenario.command_list:
        time.sleep(command.wait_time)
        process.stdin.write(command.command)
        process.stdin.flush()

    time.sleep(scenario.time_before_kill)
    process.stdin.write("EXIT")
    process.stdin.close()
    process.wait()

    out, err = (process.stdout.read(), process.stderr.read())

    process.stdout.close()
    process.stderr.close()

    return (out, err)

# Specific scenario subfunction
def is_leader_test_sucessful(out, scenario, ignore_nodes=[]):
    res = ["" for i in range(scenario.nb_servers)]

    for line in out:
        if is_word_in_string("leader", line):
            res[extract_number(line)[0] - 1] = "leader"
        if is_word_in_string("candidate", line):
            res[extract_number(line)[0] - 1] = "candidate"
        if is_word_in_string("follower", line):
            res[extract_number(line)[0] - 1] = "follower"

    for i in ignore_nodes:
        res[i - 1] = "follower"

    return res.count("leader") == 1 and res.count("follower") == scenario.nb_servers - 1

def is_log_test_sucessful(out, scenario, ignore_logs=[]):
    filenames = []

    for f in os.listdir("logs"):
        if f not in ignore_logs:
            filenames.append("logs/" + f)

    return len(filenames) == scenario.nb_servers - len(ignore_logs) and len(read_log(filenames[0])) == 10 and are_log_files_identical(filenames)

# Specific scenario handling
def handle_leader_scenario(scenario):
    out, err = launch_scenario(scenario)

    out = out.split('\n')
    err = err.split('\n')

    if (err != [""]):
        print("Error occured")
        print(err)
    else:
        if is_leader_test_sucessful(out, scenario):
            return True
        else:
            return out

def handle_log_scenario(scenario):
    out, err = launch_scenario(scenario)

    out = out.split('\n')
    err = err.split('\n')

    if (err != [""]):
        print("Error occured")
        print(err)
    else:
        if is_log_test_sucessful(out, scenario):
            return True
        else:
            return out

def handle_leader_crash_scenario(scenario):
    out, err = launch_scenario(scenario)

    out = out.split('\n')
    err = err.split('\n')

    if (err != [""]):
        print("Error occured")
        print(err)
    else:
        if is_leader_test_sucessful(out, scenario, ignore_nodes=[1]):
            return True
        else:
            return out

def handle_leader_crash_log(scenario):
    out, err = launch_scenario(scenario)

    out = out.split('\n')
    err = err.split('\n')

    if (err != [""]):
        print("Error occured")
        print(err)
    else:
        if is_log_test_sucessful(out, scenario, ["server_1.data"]):
            return True
        else:
            return out

# Handle and dispatch scenarios to specific functions
def handle_scenario(scenario):
    test_type = scenario.test_type

    if test_type == "LEADER_TEST":
        res = handle_leader_scenario(scenario)
    elif test_type == "LOG_TEST":
        res = handle_log_scenario(scenario)
    elif test_type == "CRASH_LEADER_TEST":
        res = handle_leader_crash_scenario(scenario)
    elif test_type == "CRASH_LOG_TEST":
        res = handle_leader_crash_log(scenario)
    else:
        raise ValueError("Unknown scenario type : {}".format(test_type))

    if res == True:
        return True
    else:
        return res

if __name__ == "__main__":
    dir = "scenarios/"
    onlyfiles = []

    for f in os.listdir(dir):
        if os.path.isfile(dir + f):
            onlyfiles.append(dir + f)
        elif os.path.isdir(dir + f):
            for g in os.listdir(dir + f):
                if os.path.isfile(dir + f + '/' + g):
                    onlyfiles.append(dir + f + '/' + g)


    errors = {}
    length = len(onlyfiles)

    for i in range(length):
        print("Scenario {}/{}".format(i + 1, length), end='\r')
        filename = onlyfiles[i]

        with open(filename) as f:
            scenario = json_to_scenario(f.read())
            f.close()

            res = handle_scenario(scenario)

            if res != True:
                errors[filename] = res

    os.system("rm -rf error_logs")
    os.system("mkdir error_logs")

    if len(errors) != 0:
        print("We had several test fails during runtime, check their logs")
        for key in errors.keys():
            print("- Scenario : {}".format(key))
            save_logs(key, "error_logs", errors[key])
    else:
        print("All {} scenarios sucessful".format(length))

    os.system("rm -rf logs")
