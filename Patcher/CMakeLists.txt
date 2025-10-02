find_package(safetyhook CONFIG REQUIRED)

add_library(hook SHARED "hookmain.cpp" consolecmd.cpp hooking.cpp decrypt.cpp lwshook.cpp)
target_link_libraries(hook PRIVATE safetyhook::safetyhook)
target_compile_definitions(hook PRIVATE NDEBUG)

add_library(dxgi SHARED dxgi.cpp)
target_compile_definitions(dxgi PRIVATE NDEBUG)
add_library(theiadumper SHARED theiadumper.cpp)
target_compile_definitions(theiadumper PRIVATE NDEBUG)

if(NOT DEFINED SPECTRE_BIN_DIR_CACHE OR NOT SPECTRE_BIN_DIR_CACHE)
  set(TARGET_FILENAME "SpectreClient-Win64-Shipping.exe")
  message(STATUS "Locating Spectre Divide in Steam libraries...")

  set(_find_spectre_ps1 "${CMAKE_CURRENT_BINARY_DIR}/FindSpectre.ps1")
  file(WRITE "${_find_spectre_ps1}" [=[
param([string]$TargetFilename = 'SpectreClient-Win64-Shipping.exe')
$ErrorActionPreference = 'SilentlyContinue'
$steamPath = (Get-ItemProperty -Path 'HKCU:\Software\Valve\Steam' -Name 'SteamPath' -ErrorAction SilentlyContinue).SteamPath
if (-not $steamPath) { $steamPath = (Get-ItemProperty -Path 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam' -Name 'InstallPath' -ErrorAction SilentlyContinue).InstallPath }
$libs = @(); if ($steamPath) { $libs += $steamPath }
if ($steamPath) {
  $vdf = Join-Path $steamPath 'steamapps\libraryfolders.vdf'
  if (Test-Path $vdf) {
    $content = Get-Content $vdf -Raw
    foreach ($m in [regex]::Matches($content, '"path"\s*"([^"]+)"')) {
      $p = $m.Groups[1].Value -replace '\\\\','\'
      if ($p -and (Test-Path $p)) { $libs += $p }
    }
  }
}
$libs = $libs | Sort-Object -Unique
$target = $null
foreach ($lib in $libs) {
  $gameDir = Join-Path $lib 'steamapps\common\Spectre Divide'
  if (Test-Path $gameDir) {
    $hit = Get-ChildItem -Path $gameDir -Filter $TargetFilename -Recurse -File -ErrorAction SilentlyContinue |
           Select-Object -First 1 -ExpandProperty FullName
    if ($hit) { $target = $hit; break }
  }
}
if ($target) { Write-Output $target }
]=])

  execute_process(
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${_find_spectre_ps1}" -TargetFilename "${TARGET_FILENAME}"
    OUTPUT_VARIABLE FOUND_FILE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(FOUND_FILE_PATH)
    file(TO_CMAKE_PATH "${FOUND_FILE_PATH}" FOUND_FILE_PATH_CMAKE)
    get_filename_component(SPECTRE_BIN_DIR "${FOUND_FILE_PATH_CMAKE}" DIRECTORY)
    set(SPECTRE_BIN_DIR_CACHE "${SPECTRE_BIN_DIR}" CACHE PATH "directory of spectre exe")
  else()
    message(WARNING "Spectre Divide not found under Steam libraries.")
  endif()
endif()
message(STATUS "Spectre bin path: ${SPECTRE_BIN_DIR_CACHE}")

# Only add the copy command if the path was found
if(SPECTRE_BIN_DIR_CACHE)
  target_compile_definitions(dxgi PRIVATE HOOK_DLL_PATH="${SPECTRE_BIN_DIR_CACHE}/hook.dll")
  add_custom_command(TARGET dxgi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:dxgi>
      "${SPECTRE_BIN_DIR_CACHE}/dxgi.dll"
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