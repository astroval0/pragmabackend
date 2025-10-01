find_package(safetyhook CONFIG REQUIRED)

add_library(hook SHARED "hookmain.cpp" consolecmd.cpp hooking.cpp decrypt.cpp lwshook.cpp)
target_link_libraries(hook PRIVATE safetyhook::safetyhook)
target_compile_definitions(hook PRIVATE NDEBUG)

add_library(dxgi SHARED dxgi.cpp)
target_compile_definitions(dxgi PRIVATE NDEBUG)
add_library(theiadumper SHARED theiadumper.cpp)
target_compile_definitions(theiadumper PRIVATE NDEBUG)

if(NOT SPECTRE_BIN_DIR_CACHE)
set(TARGET_FILENAME "SpectreClient-Win64-Shipping.exe")
message(STATUS "Locating spectre divide, this may take a bit...")

execute_process(
    COMMAND powershell -Command "Get-PSDrive -PSProvider 'FileSystem' | ForEach-Object { Get-ChildItem -Path ($_.Root) -Recurse -Filter '${TARGET_FILENAME}' -ErrorAction SilentlyContinue } | Select-Object -First 1 -ExpandProperty FullName"
    OUTPUT_VARIABLE FOUND_FILE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

get_filename_component(SPECTRE_BIN_DIR "${FOUND_FILE_PATH}" DIRECTORY)
set(SPECTRE_BIN_DIR_CACHE ${SPECTRE_BIN_DIR} CACHE PATH "directory of spectre exe")
endif()
message(STATUS "Spectre bin path: ${SPECTRE_BIN_DIR_CACHE}")

# Only add the copy command if the path was found
if(SPECTRE_BIN_DIR_CACHE)
    target_compile_definitions(dxgi PRIVATE THEIADUMPER_DLL_PATH="${SPECTRE_BIN_DIR_CACHE}/theiadumper.dll")
    target_compile_definitions(dxgi PRIVATE HOOK_DLL_PATH="${SPECTRE_BIN_DIR_CACHE}/hook.dll")
    add_custom_command(TARGET dxgi POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxgi>
            "${SPECTRE_BIN_DIR_CACHE}/dxgi.dll"
    )
    add_custom_command(TARGET theiadumper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:theiadumper>
            "${SPECTRE_BIN_DIR_CACHE}/theiadumper.dll"
    )
    add_custom_command(TARGET hook POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:hook>
            "${SPECTRE_BIN_DIR_CACHE}/hook.dll"
    )
    add_executable(RunSpectre RunSpectre.cpp)
    add_dependencies(RunSpectre dxgi theiadumper hook pragmabackend)
    target_compile_definitions(RunSpectre PRIVATE STEAM_APP_ID="2641470")
    target_compile_definitions(RunSpectre PRIVATE STEAM_ID="76561198370592897")
    target_compile_definitions(RunSpectre PRIVATE GAME_PATH="${SPECTRE_BIN_DIR_CACHE}/SpectreClient-Win64-Shipping.exe")
    target_compile_definitions(RunSpectre PRIVATE BACKEND_PATH="${CMAKE_CURRENT_BINARY_DIR}/../pragmabackend.exe")
    target_compile_definitions(RunSpectre PRIVATE BACKEND_DIR="${CMAKE_CURRENT_BINARY_DIR}/../")
    set_target_properties(RunSpectre PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
else()
    message(ERROR "Steam path not found. Skipping DLL copy.")
endif()