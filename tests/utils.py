import json
from proto import persistent_state_pb2
from google.protobuf import text_format


class Command:
    def __init__(self, wait_time, command):
        self.wait_time = wait_time
        
        if command[-1] != '\n':
            command = command + '\n'
        
        self.command = command
    
    def serialize(self):
        return {
            'wait_time': self.wait_time,
            'command': self.command,
        }
    
    def __repr__(self):
        return "Command('wait_time' : {}, 'command' : {})".format(self.wait_time, self.command)

class Scenario:
    def __init__(self, nb_servers, nb_clients, command_list, time_before_kill, test_type):
        self.nb_servers = nb_servers
        self.nb_clients = nb_clients
        self.command_list = command_list
        self.time_before_kill = time_before_kill
        self.test_type = test_type

        for i in range(len(self.command_list)):
            if type(self.command_list[i]) is dict:
                self.command_list[i] = Command(**self.command_list[i])
    
    def expand_command_list(self, command):
        self.command_list.append(command)
    
    def serialize(self):
        obj = {
            'nb_servers' : self.nb_servers,
            'nb_clients' : self.nb_clients,
            'time_before_kill' : self.time_before_kill,
            'test_type' : self.test_type,
        }

        commands = []
        for command in self.command_list:
            commands.append(command.serialize())
        obj['command_list'] = commands

        return obj
    
    def __repr__(self):
        return "Scenario('nb_servers' : {},\n'nb_clients' : {},\n'time_before_kill' : {},\n'test_type' : {},\n'command_list' : {})".format(
                self.nb_servers,
                self.nb_clients,
                self.time_before_kill,
                self.test_type,
                self.command_list
            )

def scenario_to_json(scenario):
    return json.dumps(scenario.serialize())

def json_to_scenario(string):
    return Scenario(**json.loads(string))

def extract_number(string):
    res = []

    for s in string.split():
        if s.isdigit():
            res.append(int(s))
    
    return res

def is_word_in_string(word, string):
    for elem in string.split():
        if elem == word:
            return True
    return False

def read_log(filename):
    with open(filename, 'rb') as f:
        logs = persistent_state_pb2.PersistentState()
        logs.ParseFromString(f.read())
        f.close()
        return logs.log_entries
    
def are_logs_identical(log_a, log_b):
    if len(log_a) != len(log_b):
        return False
    
    for i in range(len(log_a)):
        if log_a[i].command != log_b[i].command or log_a[i].command != log_b[i].command:
            return False
    
    return True

def are_log_files_identical(filenames):
    if len(filenames) == 1:
        return True
    
    base_log = read_log(filenames[0])

    for i in range(1, len(filenames)):
        new_log = read_log(filenames[i])

        if not are_logs_identical(base_log, new_log):
            return False
    
    return True