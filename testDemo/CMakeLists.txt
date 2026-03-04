cmake_minimum_required(VERSION 3.16)
project(QCustomPlotPerformanceDemo VERSION 1.0.0 LANGUAGES CXX)

# 设置Qt路径
set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.15.2/5.15.2/msvc2019_64")

# 查找Qt库
find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 自动包含MOC文件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 选项：启用OpenGL加速
option(ENABLE_OPENGL "Enable OpenGL acceleration for QCustomPlot" OFF)
option(ENABLE_PERF_OPTIMIZATION "Enable performance optimization" ON)

# 包含QCustomPlot头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_qcustomplot)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_qcustomplot/3rd_qcustomplot)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_qcustomplot/3rd_qcustomplot/v2_1)

# 添加可执行文件
add_executable(QCustomPlotPerformanceDemo
    main.cpp
    mainwindow.h
    mainwindow.cpp
    plotwindow.h
    plotwindow.cpp
    datagenerator.h
    datagenerator.cpp
    performancemonitor.h
    performancemonitor.cpp
    # QCustomPlot库文件
    3rd_qcustomplot/3rd_qcustomplot/v2_1/qcustomplot.cpp
    3rd_qcustomplot/3rd_qcustomplot/smoothcurve.cpp
)

# 链接Qt库
target_link_libraries(QCustomPlotPerformanceDemo
    Qt5::Core
    Qt5::Widgets
    Qt5::PrintSupport
)

# 根据选项添加OpenGL支持
if(ENABLE_OPENGL)
    message(STATUS "Enabling OpenGL support")
    find_package(Qt5 COMPONENTS OpenGL REQUIRED)
    target_link_libraries(QCustomPlotPerformanceDemo Qt5::OpenGL)
    target_compile_definitions(QCustomPlotPerformanceDemo PRIVATE QCUSTOMPLOT_USE_OPENGL)
    
    # 链接OpenGL库（Windows）
    if(WIN32)
        target_link_libraries(QCustomPlotPerformanceDemo opengl32 glu32)
    endif()
endif()

# 性能优化编译选项
if(ENABLE_PERF_OPTIMIZATION AND MSVC)
    target_compile_options(QCustomPlotPerformanceDemo PRIVATE
        /O2      # 优化速度
        /Oi      # 生成内部函数
        /Ot      # 偏好快速代码
        /GL      # 全程序优化
    )
    target_link_options(QCustomPlotPerformanceDemo PRIVATE /LTCG)
endif()

# 添加UTF-8编码支持（解决中文乱码问题）
if(MSVC)
    target_compile_options(QCustomPlotPerformanceDemo PRIVATE "/utf-8")
else()
    target_compile_options(QCustomPlotPerformanceDemo PRIVATE "-finput-charset=UTF-8")
    target_compile_options(QCustomPlotPerformanceDemo PRIVATE "-fexec-charset=UTF-8")
endif()

# 添加Qt宏定义，确保正确处理UTF-8字符串
target_compile_definitions(QCustomPlotPerformanceDemo PRIVATE "QT_USE_UTF8STRING")

# 设置目标属性
set_target_properties(QCustomPlotPerformanceDemo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

# 复制freeglut DLL到输出目录（如果启用OpenGL）
if(ENABLE_OPENGL AND WIN32)
    add_custom_command(TARGET QCustomPlotPerformanceDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/3rd_qcustomplot/freeglut/bin/x64/freeglut.dll"
            $<TARGET_FILE_DIR:QCustomPlotPerformanceDemo>
        COMMENT "Copying freeglut.dll to output directory"
    )
endif()
