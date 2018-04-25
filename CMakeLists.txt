cmake_minimum_required(VERSION 3.1)

# Must use g++-7, this needs to be set before the project line
set(CMAKE_C_COMPILER "gcc-7")
set(CMAKE_CXX_COMPILER "g++-7")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(FHEVideo)

include_directories(include)
include_directories(homo)
include_directories(SEAL/SEAL) 
link_directories(SEAL/bin)


# Use OpenCV for reading in files and debugging
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
message(STATUS "OpenCV library status:")
message(STATUS "\tversion: ${OpenCV_VERSION}")
message(STATUS "\tlibraries: ${OpenCV_LIBS}")
message(STATUS "\tinnclude path: ${OpenCV_INCLUDE_DIRS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3 -DHAVE_CONFIG_H -std=c++11 -pthread -g -Wno-narrowing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++") 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Make the FHE demo programs and put them to BIN
add_executable(client_jpeg homo/client_jpeg.cpp ${LIBRARY_SOURCES})
add_executable(server_jpeg homo/server_jpeg.cpp ${LIBRARY_SOURCES})
target_link_libraries(client_jpeg seal)
target_link_libraries(server_jpeg seal)

# Picture resizing demos
add_executable(client_resize homo/client_resize.cpp ${LIBRARY_SOURCES})
add_executable(server_resize homo/server_resize.cpp ${LIBRARY_SOURCES})
target_link_libraries(client_resize seal ${OpenCV_LIBS})
target_link_libraries(server_resize seal ${OpenCV_LIBS})

# JPEG Decompression Demos 
add_executable(decode_client homo/client_decode.cpp ${LIBRARY_SOURCES})
add_executable(decode_server homo/server_decode.cpp ${LIBRARY_SOURCES})
target_link_libraries(decode_client seal ${OpenCV_LIBS})
target_link_libraries(decode_server seal ${OpenCV_LIBS})


# Image testing, debugging only
add_executable(image_test homo/image_test.cpp ${LIBRARY_SOURCES})
target_link_libraries(image_test seal ${OpenCV_LIBS})

# Parameter testing, debugging only
add_executable(param homo/parameters.cpp ${LIBRARY_SOURCES})
target_link_libraries(param seal ${OpenCV_LIBS})