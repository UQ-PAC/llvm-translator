cmake_minimum_required(VERSION 3.10)

# set the project name
project(llvm-translator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM 14 REQUIRED CONFIG
    PATHS /usr/lib/llvm14)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

execute_process(COMMAND "${LLVM_TOOLS_BINARY_DIR}/llvm-config" --libfiles core support irreader
    OUTPUT_VARIABLE LLVM_LIBRARY_FILES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Using LLVM libraries: ${LLVM_LIBRARY_FILES}")

add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

#link_libraries(${llvm_libs})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# add the executable
add_executable(llvm-translator src/main.cpp src/state.cpp src/context.cpp
    src/capstone.cpp src/remill.cpp src/asl.cpp)

target_link_libraries(llvm-translator ${LLVM_LIBRARY_FILES})

target_compile_options(llvm-translator PRIVATE -g -fsanitize=address -Wall)
target_link_options(llvm-translator PRIVATE -g -fsanitize=address)





