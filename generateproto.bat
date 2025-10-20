@echo off

set "PROTOC=%~1"
set "INPUT_DIR=%~2"
set "OUTPUT_DIR=%~3"

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

for /R "%INPUT_DIR%" %%f in (*.proto) do (
    echo Compiling %%f
    "%PROTOC%" --cpp_out="%OUTPUT_DIR%" -I "%INPUT_DIR%" "%%f"
)