cmake_minimum_required(VERSION 4.0)
project(pragmabackend)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(_WIN32_WINNT=0x0A00) # target at least win10
add_compile_definitions(FMT_UNICODE=0)

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

file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
    COMMENT "Copying resources directory to binary dir"
)

add_executable(pragmabackend main.cpp)
add_dependencies(pragmabackend copy_resources)

find_package(Boost REQUIRED COMPONENTS beast bimap)
find_package(boost_asio CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_program(PROTOC_COMPILER protoc REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
target_compile_definitions(pragmabackend PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
target_compile_definitions(pragmabackend PRIVATE CERT_DIR="${CMAKE_SOURCE_DIR}/certs")
file(GLOB_RECURSE packetimplementors CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Implementation/*.cpp")
target_sources(pragmabackend PRIVATE ${packetimplementors})
file(GLOB_RECURSE persistenceimplementors CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Persistence/Implementation/*.cpp")
target_sources(pragmabackend PRIVATE ${persistenceimplementors})
target_link_libraries(pragmabackend PRIVATE Boost::beast Boost::bimap Boost::asio spdlog::spdlog_header_only)
target_link_libraries(pragmabackend PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(pragmabackend PRIVATE SQLiteCpp)
target_link_libraries(pragmabackend PRIVATE protobuf::libprotoc protobuf::libprotobuf)
target_link_libraries(pragmabackend PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(pragmabackend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Model")
target_include_directories(pragmabackend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Persistence/Model")

file(GLOB_RECURSE PERSISTENCE_PROTOBUF_FILES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Persistence/Protoc/*.proto")
if(PERSISTENCE_PROTOBUF_FILES)
PROTOBUF_GENERATE_CPP(PERSISTENCE_PROTO_SRCS PERSISTENCE_PROTO_HEADERS ${PERSISTENCE_PROTOBUF_FILES})
target_sources(pragmabackend PRIVATE ${PERSISTENCE_PROTO_SRCS})
endif()
file(GLOB_RECURSE PACKETS_PROTOBUF_FILES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Protoc/*.proto")
if(PACKETS_PROTOBUF_FILES)
PROTOBUF_GENERATE_CPP(PACKETS_PROTO_SRCS PACKETS_PROTO_HEADERS ${PACKETS_PROTOBUF_FILES})
target_sources(pragmabackend PRIVATE ${PACKETS_PROTO_SRCS})
endif()
target_include_directories(pragmabackend PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(Patcher)