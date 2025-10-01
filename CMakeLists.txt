cmake_minimum_required(VERSION 4.0)
project(pragmabackend)

set(CMAKE_CXX_STANDARD 20)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/vcpkg.exe")
    execute_process(
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg"
        RESULT_VARIABLE bootstrap_result
    )
    if(bootstrap_result)
        message(FATAL_ERROR "Failed to bootstrap vcpkg")
    endif()
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/responses/.copied
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/responses
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/responses ${CMAKE_CURRENT_BINARY_DIR}/responses
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/responses/.copied
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/responses
    COMMENT "Copying responses directory to current binary dir"
)

add_custom_target(copy_responses DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/responses/.copied)

add_executable(pragmabackend main.cpp)
add_dependencies(pragmabackend copy_responses)

find_package(Boost REQUIRED COMPONENTS beast bimap)
find_package(boost_asio CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
target_compile_definitions(pragmabackend PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
target_compile_definitions(pragmabackend PRIVATE CERT_DIR="${CMAKE_SOURCE_DIR}/certs")
file(GLOB_RECURSE packetimplementors CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Implementation/*.cpp")
target_sources(pragmabackend PRIVATE ${packetimplementors})
add_compile_definitions(_WIN32_WINNT=0x0A00) # target at least win10
add_compile_definitions(FMT_UNICODE=0)
add_compile_options(/ZI)
target_link_libraries(pragmabackend PRIVATE Boost::beast Boost::bimap Boost::asio spdlog::spdlog_header_only)
target_link_libraries(pragmabackend PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(pragmabackend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Model")

add_subdirectory(Patcher)