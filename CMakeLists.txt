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

# Define import roots for protoc
set(PROTOBUF_IMPORT_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/Persistence/Protoc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Protoc"
)

# Convert to -I flags
set(PROTOBUF_FLAGS "")
foreach(dir IN LISTS PROTOBUF_IMPORT_DIRS)
    list(APPEND PROTOBUF_FLAGS "-I${dir}")
endforeach()

# Collect all .proto files
file(GLOB_RECURSE PERSISTENCE_PROTOBUF_FILES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Persistence/Protoc/*.proto")
file(GLOB_RECURSE PACKETS_PROTOBUF_FILES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Packets/Protoc/*.proto")

# Generate output files
set(GENERATED_PROTO_SRCS "")
set(GENERATED_PROTO_HDRS "")

foreach(proto_file IN LISTS PACKETS_PROTOBUF_FILES PERSISTENCE_PROTOBUF_FILES)
    get_filename_component(proto_name ${proto_file} NAME_WE)
    set(proto_src_abs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(proto_head_abs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    add_custom_command(
        OUTPUT ${proto_src_abs} ${proto_head_abs}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS
        ${PROTOBUF_FLAGS}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${proto_file}
        COMMENT "Compiling protobuf file: ${proto_name}.proto"
    )

    list(APPEND GENERATED_PROTO_SRCS ${proto_src_abs})
    list(APPEND GENERATED_PROTO_HDRS ${proto_head_abs})
endforeach()

add_custom_target(generate_protos DEPENDS ${GENERATED_PROTO_SRCS} ${GENERATED_PROTO_HDRS})
add_dependencies(pragmabackend generate_protos)

target_sources(pragmabackend PRIVATE ${GENERATED_PROTO_SRCS})
target_include_directories(pragmabackend PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(Patcher)