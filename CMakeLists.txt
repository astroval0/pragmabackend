cmake_minimum_required(VERSION 4.0)
project(pragmabackend)

set(CMAKE_CXX_STANDARD 20)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/vcpkg.exe")
    execute_process(
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg"
        RESULT_VARIABLE bootstrap_result
    )
    if(RESULT NOT EQUAL 0)
        message(FATAL_ERROR "Failed to bootstrap vcpkg")
    endif()
endif()

add_executable(pragmabackend main.cpp)

find_package(Boost REQUIRED COMPONENTS beast)
find_package(boost_asio CONFIG REQUIRED)
target_compile_definitions(pragmabackend PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
add_compile_definitions(_WIN32_WINNT=0x0A00) # target at least win10
target_link_libraries(pragmabackend PRIVATE Boost::beast)
target_link_libraries(pragmabackend PRIVATE Boost::asio)