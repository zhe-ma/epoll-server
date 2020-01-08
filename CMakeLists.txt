cmake_minimum_required(VERSION 3.1.0)

if(POLICY CMP0074)
    # find_package() uses <PackageName>_ROOT variables.
    # This policy was introduced in CMake version 3.12.
    cmake_policy(SET CMP0074 NEW)
endif()

project(EpollServer)

# C++ standard requirements.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake 3.1.0+ required.
# See: https://stackoverflow.com/a/29871891
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
add_definitions(-DSPDLOG_LEVEL_NAMES={\"TRACE\",\"DEBUG\",\"INFO\",\"WARN\",\"ERROR\",\"FATAL\",\"OFF\"})

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    # ${PROJECT_BINARY_DIR}/src
    )

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
include_directories(${THIRD_PARTY_DIR})

# add_subdirectory(src/unittest)
add_subdirectory(src/epoll_server)
