# PowerShell 构建脚本
Write-Host "开始构建实时数据接收器..." -ForegroundColor Cyan

# 1. 设置 Visual Studio 环境
Write-Host "设置 Visual Studio 环境..." -ForegroundColor Yellow
$vcvars = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
if (!(Test-Path $vcvars)) {
    Write-Host "错误: 找不到 vcvarsall.bat" -ForegroundColor Red
    exit 1
}

# 调用 vcvarsall.bat 并捕获环境变量
cmd /c "`"$vcvars`" x64 && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        $name = $matches[1]
        $value = $matches[2]
        [Environment]::SetEnvironmentVariable($name, $value, 'Process')
    }
}

Write-Host "检查编译器..." -ForegroundColor Yellow
try {
    $clPath = Get-Command "cl" -ErrorAction Stop
    Write-Host "找到 cl: $($clPath.Source)" -ForegroundColor Green
} catch {
    Write-Host "错误: 找不到 cl 编译器" -ForegroundColor Red
    Write-Host "尝试查找 Visual Studio 安装..." -ForegroundColor Yellow
    $vsPaths = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community",
        "C:\Program Files\Microsoft Visual Studio\18\Professional",
        "C:\Program Files\Microsoft Visual Studio\18\Enterprise"
    )
    foreach ($vsPath in $vsPaths) {
        if (Test-Path $vsPath) {
            Write-Host "找到 Visual Studio: $vsPath" -ForegroundColor Yellow
        }
    }
    exit 1
}

# 2. 运行 qmake
Write-Host "运行 qmake..." -ForegroundColor Yellow
$qmakePath = "C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe"
if (!(Test-Path $qmakePath)) {
    Write-Host "错误: 找不到 qmake" -ForegroundColor Red
    exit 1
}

& $qmakePath realtime_data.pro
if ($LASTEXITCODE -ne 0) {
    Write-Host "qmake 失败" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "qmake 成功" -ForegroundColor Green

# 3. 运行 nmake
Write-Host "运行 nmake..." -ForegroundColor Yellow
try {
    $nmakePath = Get-Command "nmake" -ErrorAction Stop
    Write-Host "找到 nmake: $($nmakePath.Source)" -ForegroundColor Green
} catch {
    Write-Host "错误: 找不到 nmake" -ForegroundColor Red
    exit 1
}

& "nmake" "release"
if ($LASTEXITCODE -ne 0) {
    Write-Host "nmake 失败" -ForegroundColor Red
    Write-Host "尝试更简单的编译方法..." -ForegroundColor Yellow
    
    # 尝试直接编译单个文件测试
    Write-Host "测试编译器是否正常工作..." -ForegroundColor Yellow
    & "cl" "/?" > $null 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "编译器测试失败" -ForegroundColor Red
    } else {
        Write-Host "编译器测试成功" -ForegroundColor Green
    }
    exit $LASTEXITCODE
}

Write-Host "构建成功完成!" -ForegroundColor Green
Write-Host "可执行文件: build/release/TemplateApp.exe" -ForegroundColor Cyan