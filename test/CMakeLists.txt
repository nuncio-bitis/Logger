cmake_minimum_required(VERSION 3.10)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(testLog testLog.cpp)

target_include_directories(testLog PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(testLog PUBLIC ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libLogger.a)

install(TARGETS testLog DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
