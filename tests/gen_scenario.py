import os
from utils import *

def create_scenario_leader_check(nb_servers, speed):
    commands = []

    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} {}".format(i + 1, speed)))

    commands.append(Command(0, "START_SERVERS"))

    return Scenario(nb_servers, 0, commands, 2, "LEADER_TEST")

def create_scenario_leader_non_uniform_speed(nb_servers, most_of_wich_speed=None):
    commands = []
    if most_of_wich_speed is None:
        speeds = ["HIGH", "MEDIUM", "LOW"]
    elif most_of_wich_speed == "HIGH":
        speeds = ["HIGH", "MEDIUM", "HIGH", "LOW"]
    elif most_of_wich_speed == "MEDIUM":
        speeds = ["HIGH", "MEDIUM", "LOW", "MEDIUM"]
    else:
        speeds = ["HIGH", "LOW", "MEDIUM", "LOW"]

    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} {}".format(i + 1, speeds[i % len(speeds)])))

    commands.append(Command(0, "START_SERVERS"))

    return Scenario(nb_servers, 0, commands, 2, "LEADER_TEST")

def create_scenario_log_check(nb_servers, nb_clients, time_between_messages):
    commands = []

    # Here every server is fast for faster test and less leader problem
    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} HIGH".format(i + 1)))

    # Sets the server 1 as the leader by having the lowest election timeout
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 1 140"))
    commands.append(Command(0, "START_SERVERS"))

    # Starts all clients back to back, waits 2 seconds for the servers to elect a leader
    for i in range(nb_clients):
        commands.append(Command(2 if i == 0 else 0, "START {}".format(i + 1)))

    # Sends 10 messages after waiting 0.1s for clients to launch
    for i in range(10):
        commands.append(Command(0.1 if i == 0 else time_between_messages, "SEND_COMMAND 1 {}".format("command_1".format(i + 1))))

    return Scenario(nb_servers, 0, commands, 2, "LOG_TEST")

def create_scenario_crash_leader(nb_servers):
    commands = []

    # Here every server is fast for faster test and less leader problem
    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} HIGH".format(i + 1)))

    # Sets the server 1 as the leader by having the lowest election timeout
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 1 140"))
    commands.append(Command(0, "START_SERVERS"))

    commands.append(Command(2 , "CRASH 1"))

    return Scenario(nb_servers, 0, commands, 2, "CRASH_LEADER_TEST")

def create_scenario_crash_follower(nb_servers):
    commands = []

    # Here every server is fast for faster test and less leader problem
    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} HIGH".format(i + 1)))

    # Sets the server 2 as the leader by having the lowest election timeout
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 2 140"))
    commands.append(Command(0, "START_SERVERS"))

    commands.append(Command(2 , "CRASH 1"))

    return Scenario(nb_servers, 0, commands, 2, "CRASH_LEADER_TEST")

def create_scenario_crash_log_continuity(nb_servers):
    commands = []

    # Here every server is fast for faster test and less leader problem
    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} HIGH".format(i + 1)))

    # Sets the server 1 as the leader by having the lowest election timeout
    # After the crash, server 2 should take the lead
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 1 100"))
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 2 145"))
    for i in range(2, nb_servers):
        commands.append(Command(0, "SET_ELECTION_TIMEOUT {} 300".format(i + 1)))
    commands.append(Command(0, "START_SERVERS"))

    # Sends 5 messages after waiting for end of election
    for i in range(5):
        commands.append(Command(2 if i == 0 else 0, "SEND_COMMAND 1 {}".format("command_1".format(i + 1))))

    commands.append(Command(2 , "CRASH 1"))

    # Sends 5 messages after waiting for end of election
    for i in range(5):
        commands.append(Command(2 if i == 0 else 0, "SEND_COMMAND 2 {}".format("command_1".format(i + 1))))

    return Scenario(nb_servers, 0, commands, 2, "CRASH_LOG_TEST")

def create_scenario_crash_log_recovery(nb_servers):
    commands = []

    # Here every server is fast for faster test and less leader problem
    for i in range(nb_servers):
        commands.append(Command(0, "SPEED {} HIGH".format(i + 1)))

    # Sets the server 1 as the leader by having the lowest election timeout
    # After the crash, server 2 should take the lead
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 1 100"))
    commands.append(Command(0, "SET_ELECTION_TIMEOUT 2 145"))
    for i in range(2, nb_servers):
        commands.append(Command(0, "SET_ELECTION_TIMEOUT {} 300".format(i + 1)))
    commands.append(Command(0, "START_SERVERS"))

    # Sends 5 messages after waiting for end of election
    for i in range(5):
        commands.append(Command(2 if i == 0 else 0, "SEND_COMMAND 1 {}".format("command_1".format(i + 1))))

    commands.append(Command(2 , "CRASH 1"))

    # Sends 5 messages after waiting for end of election
    for i in range(5):
        commands.append(Command(2 if i == 0 else 0, "SEND_COMMAND 2 {}".format("command_1".format(i + 1))))

    commands.append(Command(2 , "RECOVER 1"))

    return Scenario(nb_servers, 0, commands, 2, "LOG_TEST")

def write_scenario_to_file(filename, scenario):
    text_file = open("scenarios/gen/" + filename, "w")
    n = text_file.write(scenario_to_json(scenario))
    text_file.close()

def gen_leader_scenarios():
    # Can elect a leader within 500ms
    # An election needs at least 2 servers
    # Goes up to 20 servers in our tests
    for i in range(2, 21):
        for speed in ["LOW", "MEDIUM", "HIGH"]:
            scenario = create_scenario_leader_check(i, speed)
            write_scenario_to_file("leader_test_{}_uniform_speed_{}.json".format(i, speed), scenario)

            scenario = create_scenario_leader_non_uniform_speed(i, speed)
            write_scenario_to_file("leader_test_{}_speed_most_{}.json".format(i, speed), scenario)

        scenario = create_scenario_leader_non_uniform_speed(i)
        write_scenario_to_file("leader_test_{}_mixed_speed.json".format(i), scenario)

def gen_log_scenarios(var_server=4, var_client=4, subdivisions = 10):
    # We generate scenarios with serveral nb. of servers, clients and different times between messages
    for i in range(4, 4 + var_server):
        for j in range(1, 1 + var_client):
            for time in range(subdivisions):
                scenario = create_scenario_log_check(i, j, float(time) / subdivisions)
                write_scenario_to_file("log_test_{}_servers_{}_clients_{}s_between_messages.json".format(i, j, float(time) / subdivisions), scenario)

def gen_crash_scenarios(nb_servers=15):
    # if we crash we need at least 2 servers to reach a majority
    for i in range(3, nb_servers + 1):
        scenario = create_scenario_crash_leader(i)
        write_scenario_to_file("crash_test_{}_servers_new_leader_elected.json".format(i), scenario)
        scenario = create_scenario_crash_follower(i)
        write_scenario_to_file("crash_test_{}_servers_crash_follower.json".format(i), scenario)
        scenario = create_scenario_crash_log_continuity(i)
        write_scenario_to_file("crash_test_{}_servers_crash_log_continuity.json".format(i), scenario)

def gen_recovery_scenarios(nb_servers=15):
    # if we crash we need at least 2 servers to reach a majority
    for i in range(3, nb_servers + 1):
        scenario = create_scenario_crash_log_recovery(i)
        write_scenario_to_file("crash_test_{}_servers_crash_log_recovery.json".format(i), scenario)

if __name__ == "__main__":
    os.system("rm -rf scenarios/gen/")
    os.system("mkdir -p scenarios/gen/")

    gen_leader_scenarios()
    gen_log_scenarios(2, 2, 3)
    gen_crash_scenarios()
    gen_recovery_scenarios()
