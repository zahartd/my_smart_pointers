cmake_minimum_required(VERSION 3.26)
project(Deque)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH              "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)

include(cmake/BuildFlags.cmake)
include(cmake/TestSolution.cmake)

include(FetchContent)

find_package(Catch REQUIRED)

add_subdirectory(allocations_checker)

# ------------------------------------------------------------------------------
# UniquePtr

add_catch(test_unique unique/test.cpp)

# ------------------------------------------------------------------------------
# SharedPtr + WeakPtr

add_catch(test_shared
    shared/test.cpp)

add_catch(test_weak
    weak/test.cpp
    weak/test_shared.cpp
    weak/test_odr.cpp)

add_catch(test_shared_from_this
    shared-from-this/test.cpp
    shared-from-this/test_shared.cpp
    shared-from-this/test_weak.cpp)

target_link_libraries(test_shared allocations_checker)
target_link_libraries(test_weak allocations_checker)
target_link_libraries(test_shared_from_this allocations_checker)

# ------------------------------------------------------------------------------
# IntrusivePtr

add_catch(test_intrusive intrusive/test.cpp)
target_link_libraries(test_intrusive allocations_checker)
