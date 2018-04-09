cmake_minimum_required(VERSION 3.10)

# Must use g++-7, this needs to be set before the project line
set(CMAKE_C_COMPILER "gcc-7")
set(CMAKE_CXX_COMPILER "g++-7")

project(FHEVideo)

include_directories(include)
include_directories(homo)
include_directories(SEAL/SEAL) 
link_directories(SEAL/bin) 


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3 -DHAVE_CONFIG_H -std=c++11 -pthread -g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Make the FHE demo programs and put them to BIN
add_executable(client homo/client.cpp ${LIBRARY_SOURCES})
add_executable(server homo/server.cpp ${LIBRARY_SOURCES})
target_link_libraries(client seal)
target_link_libraries(server seal)