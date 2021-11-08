cmake_minimum_required(VERSION 3.1)
project(algorep)

# Find MPI library
find_package(MPI REQUIRED)

# Find Boost library
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)

# Find Google protobuf library
find_package(Protobuf REQUIRED)

# Build options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Use the mpic ++ compiler
set(CMAKE_CXX_COMPILER mpicxx)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# Put default build target
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "RELEASE as default build target")
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()

# Sources
set(SRC_CPP
    src/mpi/mpi_rpc.cc
    src/mpi/mpi_process.cc

    src/raft/raft_clock.cc
    src/raft/raft_server.cc
    src/raft/raft_client.cc
    src/raft/raft_controller.cc
    src/raft/raft_storage.cc

    src/utils/arg_parser.cc
    src/utils/serialization.cc
)

# Proto
set(SRC_PROTO
    proto/append_entry.proto
    proto/command_entry.proto
    proto/log_entry.proto
    proto/message.proto
    proto/vote.proto
    proto/search_leader.proto
    proto/election_timeout.proto
    proto/speed.proto
    proto/persistent_state.proto
)

# Directories
include_directories(src)
include_directories(src/mpi)
include_directories(src/raft)
include_directories(src/rpc)
include_directories(src/utils)
include_directories(src/storage)
# To avoid : fatal error: 'google/protobuf/port_def.inc' in some cases...
include_directories(${PROTOBUF_INCLUDE_DIRS})

# Default compilation flags
if (APPLE)
    set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -fsanitize=address")
else()
    set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -lpthread -lrt")
endif()

# Debug compilation flags
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG")

# Release compilation flags
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DRELEASE")

# LIBRARIES
if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    set(Boost_USE_STATIC_LIBS ON)
endif()

set(BOOST_LIBRARIES Boost::system Boost::filesystem Boost::program_options ${CMAKE_DL_LIBS})
set(PROTOBUF_LIBRARIES protobuf::libprotobuf)

add_executable(algorep)
target_sources(algorep PRIVATE "src/main.cc" ${SRC_CPP} ${SRC_PROTO})
target_link_libraries(algorep PRIVATE ${BOOST_LIBRARIES} ${PROTOBUF_LIBRARIES})

protobuf_generate(TARGET algorep)
