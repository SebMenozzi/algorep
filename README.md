# ALGOREP

EPITA project that uses the RAFT algorithms for logs replication.

## Requirements

- CMake 3.1

## Dependencies

Install OpenMPI, Boost and Protobuf:

- Linux: **sudo apt install libopenmpi-dev libboost-all-dev protobuf-compiler libprotobuf-dev -y**
- MacOS: **brew install open-mpi boost protobuf**

## Build

- Run **make debug** or **make release**. Default **make** is in debug.

## Usage

- To run the raft network, run **make run**

## Tests

- To test the network after building it, run **make tests**
- Some tests may fail (the test on speed) but such a behavior is to be expected and explained in the report

# REPL (for the controller)

- **CRASH [NODE_ID]** will simulate a crash to the node and will become unresponsive.
- **SPEED [NODE_ID] [TYPE]** will impact the speed of the node. There are 3 types: LOW, MEDIUM, HIGH).
- **SEND_COMMAND [NODE_ID] [STRING]** sends string as a command to add into the logs from the client node.
- **START [NODE_ID]** will start the node process.
- **RECOVER [NODE_ID]** will recover the node process.
- **START_SERVERS** will start all server processes.
- **SET_ELECTION_TIMEOUT [SERVER_ID] [TIMEOUT]** allows to hardcode the election timeout before starting the server.
- **EXIT** stops the whole system and exits the process.
