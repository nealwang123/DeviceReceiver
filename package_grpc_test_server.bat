@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

set "ROOT_DIR=%~dp0"
pushd "%ROOT_DIR%" >nul

set "PYTHON_EXE="
set "PYTHON_ARGS="

if exist "%CD%\.venv\Scripts\python.exe" (
    call :try_python "%CD%\.venv\Scripts\python.exe" ""
)

if not defined PYTHON_EXE (
    for /f "usebackq delims=" %%I in (`where python 2^>nul`) do (
        if not defined PYTHON_EXE call :try_python "%%I" ""
    )
)

if not defined PYTHON_EXE (
    for /f "usebackq delims=" %%I in (`where py 2^>nul`) do (
        if not defined PYTHON_EXE call :try_python "%%I" "-3"
    )
)

if not defined PYTHON_EXE (
    echo [ERROR] 未找到满足条件的 Python 解释器（需可 import grpc 和 google.protobuf）
    echo [INFO] 可先执行: python -m pip install grpcio protobuf
    popd >nul
    exit /b 1
)

echo [INFO] 使用解释器: "%PYTHON_EXE%" %PYTHON_ARGS%

if not exist "grpc_test_server.py" (
    echo [ERROR] 未找到 grpc_test_server.py
    popd >nul
    exit /b 1
)

set "PROTO_PY_SRC=%CD%\proto\generated_py"

if not exist "%PROTO_PY_SRC%\device_data_pb2.py" (
    echo [ERROR] 未找到 proto\generated_py\device_data_pb2.py
    echo [INFO] 请先生成 Python protobuf 文件后再打包
    popd >nul
    exit /b 1
)

"%PYTHON_EXE%" %PYTHON_ARGS% -m pip show pyinstaller >nul 2>&1
if errorlevel 1 (
    echo [INFO] 正在安装 PyInstaller...
    "%PYTHON_EXE%" %PYTHON_ARGS% -m pip install pyinstaller
    if errorlevel 1 (
        echo [ERROR] PyInstaller 安装失败
        popd >nul
        exit /b 1
    )
)

set "DIST_DIR=build\release"
set "WORK_DIR=build\temp\pyinstaller"

if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
if not exist "%WORK_DIR%" mkdir "%WORK_DIR%"

echo [INFO] 开始打包 grpc_test_server.exe ...
"%PYTHON_EXE%" %PYTHON_ARGS% -m PyInstaller ^
    --noconfirm ^
    --clean ^
    --onefile ^
    --name grpc_test_server ^
    --distpath "%DIST_DIR%" ^
    --workpath "%WORK_DIR%" ^
    --specpath "%WORK_DIR%" ^
    --paths "%PROTO_PY_SRC%" ^
    --add-data "%PROTO_PY_SRC%;proto\generated_py" ^
    --hidden-import device_data_pb2 ^
    --hidden-import device_data_pb2_grpc ^
    --hidden-import grpc ^
    --hidden-import google.protobuf ^
    --collect-submodules google.protobuf ^
    --hidden-import grpc._cython.cygrpc ^
    grpc_test_server.py

if errorlevel 1 (
    echo [ERROR] 打包失败
    popd >nul
    exit /b 1
)

if exist "build\debug\" (
    copy /Y "%DIST_DIR%\grpc_test_server.exe" "build\debug\grpc_test_server.exe" >nul 2>&1
)

echo [OK] 打包完成: %DIST_DIR%\grpc_test_server.exe
popd >nul
exit /b 0

:try_python
set "CAND_PY_EXE=%~1"
set "CAND_PY_ARGS=%~2"

if "%CAND_PY_EXE%"=="" exit /b 0
if not exist "%CAND_PY_EXE%" exit /b 0

"%CAND_PY_EXE%" %CAND_PY_ARGS% -c "import grpc, google.protobuf" >nul 2>&1
if errorlevel 1 (
    echo [WARN] 跳过解释器（缺少 grpc/protobuf）: "%CAND_PY_EXE%" %CAND_PY_ARGS%
    exit /b 0
)

set "PYTHON_EXE=%CAND_PY_EXE%"
set "PYTHON_ARGS=%CAND_PY_ARGS%"
exit /b 0
