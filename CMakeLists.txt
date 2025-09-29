cmake_minimum_required(VERSION 4.0)
project(pragmabackend)

set(CMAKE_CXX_STANDARD 20)

add_executable(pragmabackend main.cpp)

target_include_directories(pragmabackend PRIVATE
        "$ENV{USERPROFILE}/local/boost_1_86_0"
)
target_compile_definitions(pragmabackend PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
target_link_libraries(pragmabackend PRIVATE Ws2_32)