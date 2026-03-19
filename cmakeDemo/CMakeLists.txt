cmake_minimum_required(VERSION 3.16)
project(NetBoxTools VERSION 1.0.0 LANGUAGES CXX)

# 可选控制：是否启用 legacy IProtocolParser（迁移期间使用，最终可删除）
option(ENABLE_PROTOCOL_PARSER "Enable legacy IProtocolParser support" OFF)

# 设置Qt路径（Qt 5.15 MSVC2019 64位）
set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.15.2/5.15.2/msvc2019_64")

# 查找Qt库（Qt 5.15 显式指定版本更稳定）
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Network Widgets PrintSupport SerialPort)

# 设置C++标准（Rerun SDK 需要 C++17，Qt 5.15 兼容 C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含各模块头文件目录（改用 target_include_directories 更规范）
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/threadtools
    ${CMAKE_CURRENT_SOURCE_DIR}/devices
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/qcustomplot
    ${CMAKE_CURRENT_SOURCE_DIR}/logTools
    ${CMAKE_CURRENT_SOURCE_DIR}/forms  # 新增：包含UI头文件生成路径
)

# Qt 自动处理开关（Qt 5.15 需放在 find_package 之后）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 配置 AUTOUIC 搜索路径（仅指定目录，不要包含文件名）
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/forms")

# 源文件列表
set(SOURCE_FILES
    main.cpp
    widget.h
    widget.cpp
    asyncfilewriter.h
    asyncfilewriter.cpp
    threadtools/mythread.h
    threadtools/workmovetothread.h
    threadtools/workmovetothread.cpp
    threadtools/ringbuffer.h
    # 设备层
    protocolparser/datapacket.h
    devices/idevice.h
    devices/tcpdevice.h
    devices/tcpdevice.cpp
    devices/udpdevice.h
    devices/udpdevice.cpp
    devices/serialdevice.h
    devices/serialdevice.cpp
    devices/devicechannel.h
    devices/devicechannel.cpp
    devices/devicemanager.h
    devices/devicemanager.cpp
    devices/parserworker.h
    devices/parserworker.cpp
    # protocol parser
    protocolparser/protocol_parser.hpp
    protocolparser/tr360_5b_parser.hpp
    protocolparser/8t8r_parser.hpp
    protocolparser/protocol_parser.cpp
    protocolparser/tr360_5b_parser.cpp
    protocolparser/8t8r_parser.cpp
    # UI 组件
    widgets/cachetimelinewidget.h
    widgets/cachetimelinewidget.cpp
    # QCustomPlot
    ThirdParty/qcustomplot/qcustomplot.h
    ThirdParty/qcustomplot/qcustomplot.cpp
    # 日志工具（提前加入，避免后续 target_sources 分散）
    logTools/LogManager.cpp
    logTools/spdlog_helpers.cpp
    # Rerun管理器
    rerunmanager.h
    rerunmanager.cpp
)

# UI 文件列表（仅写文件名，因为 AUTOUIC_SEARCH_PATHS 已指定 forms 目录）
set(UI_FILES
    forms/widget.ui
)
# 3. 合并成单一列表（给Qt Creator识别用）
list(APPEND PROJECT_SOURCES ${SOURCE_FILES} ${UI_FILES})
# 关键修复：将 UI_FILES 加入可执行文件编译列表
add_executable(tcpclient
    ${PROJECT_SOURCES}
)

# 链接Qt库
target_link_libraries(tcpclient
    Qt5::Core
    Qt5::Network
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::SerialPort
)

# 设置目标属性
set_target_properties(tcpclient PROPERTIES
    WIN32_EXECUTABLE FALSE  # 保留控制台窗口（调试用）
    MACOSX_BUNDLE FALSE
)

# Use CPM to fetch spdlog (async, compiled lib)
if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD "https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake" "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
endif()
include(${CMAKE_BINARY_DIR}/cmake/CPM.cmake)

# 第一步：先设置全局编译选项（必须在引入第三方库之前！）
if(MSVC)
    # 全局强制 UTF-8 编码（对所有目标生效，包括 FetchContent 引入的库）
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"  # C++ 文件
        "$<$<COMPILE_LANGUAGE:C>:/utf-8>"    # C 文件
        /wd4819                               # 禁用 C4819 编码警告
    )
else()
    # GCC/Clang 编码配置
    add_compile_options(
        "-finput-charset=UTF-8"
        "-fexec-charset=UTF-8"
    )
endif()

# Force compiled spdlog to enable async logger
set(SPDLOG_COMPILED_LIB ON CACHE BOOL "Use compiled spdlog library" FORCE)
if(COMMAND CPMAddPackage)
    CPMAddPackage(NAME spdlog GITHUB_REPOSITORY "gabime/spdlog" VERSION 1.17.0)
else()
    message(STATUS "CPMAddPackage not available, falling back to FetchContent for spdlog")
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# 确保 spdlog::spdlog 目标存在
if(NOT TARGET spdlog::spdlog)
    if(TARGET spdlog)
        add_library(spdlog::spdlog ALIAS spdlog)
    else()
        # 创建接口库
        add_library(spdlog_fallback INTERFACE)
        # 尝试多种可能的 include 路径
        if(EXISTS ${spdlog_SOURCE_DIR}/include)
            target_include_directories(spdlog_fallback INTERFACE ${spdlog_SOURCE_DIR}/include)
        elseif(EXISTS ${spdlog_SOURCE_DIR}/spdlog/include)
            target_include_directories(spdlog_fallback INTERFACE ${spdlog_SOURCE_DIR}/spdlog/include)
        else()
            message(WARNING "spdlog include directory not found at ${spdlog_SOURCE_DIR}/include or ${spdlog_SOURCE_DIR}/spdlog/include")
            target_include_directories(spdlog_fallback INTERFACE ${spdlog_SOURCE_DIR})
        endif()
        target_compile_definitions(spdlog_fallback INTERFACE SPDLOG_COMPILED_LIB=0)
        add_library(spdlog::spdlog ALIAS spdlog_fallback)
    endif()
endif()

# Download and link Rerun SDK
include(FetchContent)
FetchContent_Declare(rerun_sdk
    URL https://github.com/rerun-io/rerun/releases/latest/download/rerun_cpp_sdk.zip
)
# 禁用 rerun_sdk 的编译警告（可选，进一步消除干扰）
set(RERUN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RERUN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rerun_sdk)


# 链接 spdlog 和 Rerun SDK
target_link_libraries(tcpclient
    spdlog::spdlog
    rerun_sdk
)

# 对 tcpclient 补充专属编译选项（冗余保障，全局已生效）
if(MSVC)
    target_compile_options(tcpclient PRIVATE "/utf-8" /wd4819)
else()
    target_compile_options(tcpclient PRIVATE 
        "-finput-charset=UTF-8"
        "-fexec-charset=UTF-8"
    )
endif()

# 添加Qt宏定义，确保正确处理UTF-8字符串
target_compile_definitions(tcpclient PRIVATE "QT_USE_UTF8STRING")

# Qt 5.15 额外配置：确保 UI 生成的头文件可被找到
target_include_directories(tcpclient PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}  # UI 头文件生成在编译目录
    ${CMAKE_CURRENT_SOURCE_DIR}/forms
)