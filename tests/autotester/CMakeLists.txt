cmake_minimum_required(VERSION 3.5)
project(autotester)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g3 -W -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=return-type -Werror=pointer-arith -Winit-self")

# You first need to build the cemucore library. Basically, type `make` in the core directory.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${CMAKE_SOURCE_DIR}/../../core/ -lcemucore")

set(SOURCE_FILES
    autotester.cpp
    autotester_cli.cpp)

add_executable(autotester ${SOURCE_FILES})