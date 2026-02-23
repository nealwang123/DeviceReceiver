# PowerShell 构建脚本 - 实时数据接收器
# 用法: .\build.ps1 [-BuildType Release|Debug] [-Clean] [-Run] [-Help]

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Release", "Debug")]
    [string]$BuildType = "Release",
    
    [Parameter(Mandatory=$false)]
    [switch]$Clean,
    
    [Parameter(Mandatory=$false)]
    [switch]$Run,
    
    [Parameter(Mandatory=$false)]
    [switch]$Help
)

if ($Help) {
    Write-Host "实时数据接收器构建脚本" -ForegroundColor Cyan
    Write-Host "用法: .\build.ps1 [-BuildType Release|Debug] [-Clean] [-Run] [-Help]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "参数:" -ForegroundColor Green
    Write-Host "  -BuildType    : 构建类型 (Release 或 Debug, 默认: Release)" -ForegroundColor White
    Write-Host "  -Clean        : 清理构建文件" -ForegroundColor White
    Write-Host "  -Run          : 构建成功后运行程序" -ForegroundColor White
    Write-Host "  -Help         : 显示此帮助信息" -ForegroundColor White
    Write-Host ""
    Write-Host "示例:" -ForegroundColor Green
    Write-Host "  .\build.ps1                     # 构建Release版本" -ForegroundColor White
    Write-Host "  .\build.ps1 -BuildType Debug    # 构建Debug版本" -ForegroundColor White
    Write-Host "  .\build.ps1 -Clean              # 清理构建文件" -ForegroundColor White
    Write-Host "  .\build.ps1 -Run                # 构建并运行Release版本" -ForegroundColor White
    Write-Host "  .\build.ps1 -Clean -Run         # 清理后构建并运行" -ForegroundColor White
    exit 0
}

# 颜色函数
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Gray
    Write-Host "步骤: $Message" -ForegroundColor Magenta
    Write-Host "========================================" -ForegroundColor Gray
    Write-Host ""
}

# 0. 脚本信息
Write-Info "实时数据接收器构建脚本"
Write-Info "构建类型: $BuildType"
Write-Info "当前目录: $PWD"

if ($Clean) {
    Write-Info "清理模式已启用"
}

if ($Run) {
    Write-Info "运行模式已启用"
}

# 1. 检查 Visual Studio 环境
Write-Step "1. 设置 Visual Studio 开发环境"

$vcvarsPath = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
if (!(Test-Path $vcvarsPath)) {
    Write-Error "找不到 vcvarsall.bat: $vcvarsPath"
    Write-Warning "请确保 Visual Studio 2019/2022 已安装"
    exit 1
}

Write-Info "找到 Visual Studio: $vcvarsPath"

# 更好的环境变量设置方法
$envSetupScript = @"
@echo off
call "$vcvarsPath" x64 >nul 2>&1
set
"@

$envSetupScript | Out-File -FilePath "$env:TEMP\setup_vs_env.bat" -Encoding ASCII
$envVars = cmd /c "$env:TEMP\setup_vs_env.bat" 2>$null

$envVars | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        $name = $matches[1]
        $value = $matches[2]
        [Environment]::SetEnvironmentVariable($name, $value, 'Process')
    }
}

Remove-Item "$env:TEMP\setup_vs_env.bat" -Force -ErrorAction SilentlyContinue

# 验证编译器
try {
    $clPath = Get-Command "cl" -ErrorAction Stop
    Write-Info "找到 MSVC 编译器: $($clPath.Source)"
} catch {
    Write-Error "找不到 MSVC 编译器 (cl.exe)"
    Write-Warning "请检查 Visual Studio 环境设置"
    exit 1
}

# 2. 检查 Qt 环境
Write-Step "2. 检查 Qt 开发环境"

$qmakePath = "C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe"
if (!(Test-Path $qmakePath)) {
    Write-Error "找不到 qmake: $qmakePath"
    Write-Warning "请确保 Qt 5.15.2 (msvc2019_64) 已安装"
    exit 1
}

Write-Info "找到 qmake: $qmakePath"

# 3. 清理构建文件（如果指定）
if ($Clean) {
    Write-Step "3. 清理构建文件"
    
    $directoriesToClean = @(
        "build",
        "debug",
        "release",
        ".qtc_clangd",
        ".qtcreator"
    )
    
    $filesToClean = @(
        "Makefile",
        "Makefile.Debug",
        "Makefile.Release",
        ".qmake.stash",
        "vc140.pdb"
    )
    
    $cleanCount = 0
    
    foreach ($dir in $directoriesToClean) {
        if (Test-Path $dir) {
            try {
                Remove-Item -Path $dir -Recurse -Force -ErrorAction Stop
                Write-Info "已删除目录: $dir"
                $cleanCount++
            } catch {
                Write-Warning "无法删除目录 $dir : $_"
            }
        }
    }
    
    foreach ($file in $filesToClean) {
        if (Test-Path $file) {
            try {
                Remove-Item -Path $file -Force -ErrorAction Stop
                Write-Info "已删除文件: $file"
                $cleanCount++
            } catch {
                Write-Warning "无法删除文件 $file : $_"
            }
        }
    }
    
    Write-Info "清理完成，共清理 $cleanCount 个项目"
}

# 4. 运行 qmake
Write-Step "4. 生成 Makefile"

Write-Info "运行 qmake..."
& $qmakePath realtime_data.pro
if ($LASTEXITCODE -ne 0) {
    Write-Error "qmake 失败，退出码: $LASTEXITCODE"
    Write-Warning "请检查 realtime_data.pro 文件"
    exit $LASTEXITCODE
}

Write-Success "qmake 成功完成"

# 5. 运行 nmake
Write-Step "5. 编译项目"

try {
    $nmakePath = Get-Command "nmake" -ErrorAction Stop
    Write-Info "找到 nmake: $($nmakePath.Source)"
} catch {
    Write-Error "找不到 nmake"
    Write-Warning "请确保 nmake 在 PATH 中"
    exit 1
}

Write-Info "正在编译 $BuildType 版本..."

if ($BuildType -eq "Release") {
    & "nmake" "release"
} else {
    & "nmake" "debug"
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "编译失败，退出码: $LASTEXITCODE"
    Write-Warning "请检查编译错误信息"
    exit $LASTEXITCODE
}

Write-Success "编译成功完成"

# 6. 确定可执行文件路径
Write-Step "6. 构建结果"

$executableName = "realtime_data.exe"
if ($BuildType -eq "Release") {
    $exePath = "build\release\$executableName"
    $buildDir = "build\release"
} else {
    $exePath = "build\debug\$executableName"
    $buildDir = "build\debug"
}

if (Test-Path $exePath) {
    $exeInfo = Get-Item $exePath
    $fileSize = "{0:N2}" -f ($exeInfo.Length / 1MB)
    Write-Success "可执行文件已生成: $exePath"
    Write-Info "文件大小: $fileSize MB"
    Write-Info "构建时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    
    # 显示构建目录内容
    if (Test-Path $buildDir) {
        $buildFiles = Get-ChildItem $buildDir -File | Select-Object -First 10
        Write-Info "构建目录内容 ($buildDir):"
        foreach ($file in $buildFiles) {
            Write-Info "  - $($file.Name) ({0:N2} KB)" -f ($file.Length / 1KB)
        }
    }
} else {
    Write-Warning "可执行文件未找到: $exePath"
    Write-Info "正在搜索构建目录..."
    
    # 尝试查找可执行文件
    $foundExes = Get-ChildItem -Path "build", "release", "debug" -Filter "*.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 5
    if ($foundExes) {
        Write-Info "找到的可执行文件:"
        foreach ($exe in $foundExes) {
            Write-Info "  - $($exe.FullName)"
        }
        $exePath = $foundExes[0].FullName
        Write-Info "将使用第一个找到的文件: $exePath"
    } else {
        Write-Error "未找到任何可执行文件"
    }
}

# 7. 运行程序（如果指定）
if ($Run -and (Test-Path $exePath)) {
    Write-Step "7. 运行程序"
    
    Write-Info "正在启动程序: $exePath"
    Write-Warning "程序将在新窗口中启动，按任意键继续..."
    pause
    
    try {
        Start-Process -FilePath $exePath -WorkingDirectory (Split-Path $exePath -Parent)
        Write-Success "程序已启动"
    } catch {
        Write-Error "启动程序失败: $_"
        Write-Info "尝试直接运行..."
        
        # 尝试直接执行
        try {
            & $exePath
        } catch {
            Write-Error "无法运行程序"
        }
    }
} elseif ($Run -and !(Test-Path $exePath)) {
    Write-Error "无法运行程序: 可执行文件不存在"
}

# 8. 完成
Write-Step "8. 构建完成"

Write-Success "========================================"
Write-Success "实时数据接收器构建完成!"
Write-Success "========================================"
Write-Info "构建类型: $BuildType"
Write-Info "可执行文件: $exePath"
if ($BuildType -eq "Release") {
    Write-Info "发布目录: build\release"
} else {
    Write-Info "调试目录: build\debug"
}
Write-Info "构建时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"

if ($Run -and (Test-Path $exePath)) {
    Write-Info "程序已启动运行"
} elseif ($Run) {
    Write-Warning "程序未启动（可执行文件不存在）"
}

Write-Info ""
Write-Info "提示:"
Write-Info "  - 使用 .\build.ps1 -Help 查看帮助"
Write-Info "  - 使用 .\build.ps1 -Clean 清理构建文件"
Write-Info "  - 使用 .\build.ps1 -BuildType Debug 构建调试版本"
Write-Info "  - 使用 .\build.ps1 -Run 构建并运行程序"

exit 0