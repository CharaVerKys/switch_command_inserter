cmake_minimum_required(VERSION 3.5)
project(MyProject)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/asio-1.28.0/include
    ${CMAKE_SOURCE_DIR}/libs/rapidjson/include
    ${CMAKE_SOURCE_DIR}/libs/SQliteCpp/include
    ${CMAKE_SOURCE_DIR}/libs/libssh2/include
)
add_subdirectory(src)
add_subdirectory(libs)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()