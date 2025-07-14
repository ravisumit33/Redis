cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES
  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/RedisServer.cpp
  ${CMAKE_SOURCE_DIR}/src/ClientConnection.cpp
  ${CMAKE_SOURCE_DIR}/src/Command.cpp
  ${CMAKE_SOURCE_DIR}/src/RespTypeParser.cpp
  ${CMAKE_SOURCE_DIR}/src/RedisStore.cpp
  ${CMAKE_SOURCE_DIR}/src/ArgParser.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)

add_executable(server ${SOURCE_FILES})

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/header)
target_link_libraries(server PRIVATE asio asio::asio)
target_link_libraries(server PRIVATE Threads::Threads)
