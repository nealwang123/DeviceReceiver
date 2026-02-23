# 实时数据接收器 - 构建脚本说明

## 项目概述
这是一个使用Qt 5.15.2开发的实时数据接收器应用程序，包含串口通信、数据处理和图形显示功能。

## 开发环境要求
- **Qt版本**: 5.15.2 msvc2019_64
- **Visual Studio**: 2022 (v18) 或更高版本
- **编译器**: MSVC 2019 (v14.2+)
- **操作系统**: Windows 10/11

## 构建脚本说明

### 主要构建脚本

#### 1. `full_build.bat` - 基础构建脚本
最简单的构建脚本，执行完整的构建流程。

**使用方法:**
```cmd
full_build.bat
```

**功能:**
- 设置Visual Studio开发环境
- 运行qmake生成Makefile
- 使用nmake编译Release版本
- 输出可执行文件位置

#### 2. `build_and_run.bat` - 增强构建脚本（推荐）
功能完整的构建脚本，支持多种参数选项。

**基本用法:**
```cmd
build_and_run.bat [参数]
```

**可用参数:**
| 参数 | 说明 | 示例 |
|------|------|------|
| `-Help` | 显示帮助信息 | `build_and_run.bat -Help` |
| `-Debug` | 构建调试版本 | `build_and_run.bat -Debug` |
| `-Clean` | 清理构建文件 | `build_and_run.bat -Clean` |
| `-Run` | 构建后运行程序 | `build_and_run.bat -Run` |
| 无参数 | 构建Release版本 | `build_and_run.bat` |

**组合使用示例:**
```cmd
# 清理后构建Release版本
build_and_run.bat -Clean

# 构建Debug版本
build_and_run.bat -Debug

# 清理、构建并运行
build_and_run.bat -Clean -Run

# 构建Debug版本并运行
build_and_run.bat -Debug -Run
```

### 其他脚本

#### `build_and_run_en.bat`
英文版本的增强构建脚本，功能与中文版相同。

#### `build.ps1`, `build_simple.ps1`, `build_simple_legacy.ps1`
PowerShell构建脚本，适用于不同版本的PowerShell环境。

#### `build_ps.ps1`
原始的PowerShell构建脚本。

## 构建目录结构

```
build/
├── release/           # Release版本输出目录
│   ├── realtime_data.exe  # 主程序
│   └── realtime_data.pdb  # 程序数据库文件
└── debug/             # Debug版本输出目录
    ├── realtime_data.exe
    └── realtime_data.pdb
```

## 常见问题解决

### 1. 中文显示乱码问题
如果在PowerShell中运行脚本出现中文乱码，请使用以下方法：

**方法一：使用cmd.exe运行**
```cmd
cmd /c "build_and_run.bat -Help"
```

**方法二：在PowerShell中设置编码**
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::GetEncoding(936)
.\build_and_run.bat -Help
```

### 2. 环境变量设置失败
如果出现"找不到vcvarsall.bat"或"找不到qmake.exe"错误，请检查：
- Visual Studio是否安装正确
- Qt 5.15.2 (msvc2019_64) 是否安装到默认路径

**默认安装路径:**
- Qt: `C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe`
- Visual Studio: `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat`

### 3. 构建失败
如果构建过程中出现编译错误：
1. 使用`-Clean`参数清理项目
2. 检查Qt依赖是否正确配置
3. 确认所有源文件完整

## 在Qt Creator中使用

项目也可以通过Qt Creator正常打开和构建：
1. 打开 `realtime_data.pro`
2. 选择合适的构建套件 (Desktop Qt 5.15.2 MSVC2019 64-bit)
3. 点击构建或运行

## 脚本工作原理

构建脚本按照以下步骤执行：

1. **环境设置**: 调用Visual Studio的vcvarsall.bat设置编译环境
2. **项目生成**: 运行qmake根据.pro文件生成Makefile
3. **编译**: 使用nmake编译项目
4. **输出**: 生成可执行文件到build目录
5. **运行**: 根据需要启动程序

## 注意事项

1. 确保系统PATH环境变量中包含nmake和cl.exe
2. 构建过程中不要关闭命令行窗口
3. 如果修改了.pro文件，建议使用`-Clean`参数重新构建
4. Debug版本包含调试信息，文件较大但便于调试

## 版本历史

- v1.0: 基础构建脚本
- v1.1: 添加参数支持、中文编码修复
- v1.2: 完善README文档

---

如有问题，请检查错误信息并根据提示进行调整。