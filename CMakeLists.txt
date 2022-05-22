cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(Logger VERSION 2.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

configure_file(version.h.in ${PROJECT_SOURCE_DIR}/version.h)

add_library(Logger Logger.cpp)
install(TARGETS Logger DESTINATION "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" )

target_include_directories(Logger PUBLIC ${PROJECT_SOURCE_DIR})


add_subdirectory(test)

target_include_directories(Logger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
