@echo off
chcp 936 >nul
echo 实时数据接收器构建脚本
echo 设置 Visual Studio 2026 环境...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    echo 错误: 设置 Visual Studio 环境失败
    pause
    exit /b %errorlevel%
)

echo 运行 qmake...
"C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe" realtime_data.pro
if %errorlevel% neq 0 (
    echo qmake 失败
    pause
    exit /b %errorlevel%
)

echo 使用 nmake 构建...
nmake
if %errorlevel% neq 0 (
    echo nmake 失败
    pause
    exit /b %errorlevel%
)

echo 构建成功!
echo.
echo 可执行文件位于: build/release/realtime_data.exe
pause