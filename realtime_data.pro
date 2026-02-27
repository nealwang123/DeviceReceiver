QT += core gui widgets
# 根据编译目标条件添加模块
!wasm {
    QT += serialport
} else {
    # WebAssembly环境需要额外添加的模块
    QT += network
}
# 编译标准
CONFIG += c++17
CONFIG += console
# CONFIG += wasm  # 不要在此硬编码！WASM构建时由 wasm_build.bat 通过 qmake CONFIG+=wasm 传入
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 为C++代码定义编译宏，方便代码中做条件判断
wasm {
    DEFINES += QT_COMPILE_FOR_WASM
    # 嵌入中文字体后需要更大的初始内存（默认16MB不够）
    QMAKE_LFLAGS += -s TOTAL_MEMORY=33554432
}
# 可执行文件名称
TARGET = realtime_data
TEMPLATE = app

#自定义程序信息
#程序版本
VERSION = 1.0.1
#程序图标
#RC_ICONS = main.ico
#产品名称
QMAKE_TARGET_PRODUCT = QtTemplateApp
#版权所有
QMAKE_TARGET_COPYRIGHT = "某某科技"
#文件说明
QMAKE_TARGET_DESCRIPTION = "台湾是中国不可分割的一部分"
# 中文（简体）
RC_LANG = 0x0804


#目标文件目录
wasm {
    # WASM构建：输出到当前build-wasm目录
    DESTDIR  = ./
    TEMP_DESTDIR = .
}

!wasm:Debug:{
    DESTDIR  = ./build/debug
    TEMP_DESTDIR = ./build/temp/debug/$$TARGET
}

!wasm:Release:{
    DESTDIR  = ./build/release
    TEMP_DESTDIR = ./build/temp/release/$$TARGET
}

#指定编译生成的文件到temp目录 分门别类存储
MOC_DIR         = $$TEMP_DESTDIR/moc
RCC_DIR         = $$TEMP_DESTDIR/rcc
UI_DIR          = $$TEMP_DESTDIR/qui
OBJECTS_DIR     = $$TEMP_DESTDIR/obj

#编译优化
msvc:CONFIG(release, debug|release) {
    QMAKE_CFLAGS_RELEASE   -= -O2              # 取消C优化
    QMAKE_CFLAGS_RELEASE   += -Zi              # 生成调试信息，放到pdb文件中
    QMAKE_CXXFLAGS_RELEASE -= -O2              # 取消C++优化
    QMAKE_CXXFLAGS_RELEASE += -Zi              # 生成调试信息
    QMAKE_LFLAGS_RELEASE   -= /INCREMENTAL:NO  # 选择增量链接
    QMAKE_LFLAGS_RELEASE   += /DEBUG           # 将调试信息放到PDB文件中
    message( MSVC Release )
}

mingw:CONFIG(release, debug|release) {
    QMAKE_CFLAGS_RELEASE   -= -O2              # 取消C优化
    QMAKE_CFLAGS_RELEASE   += -O0              # 显示指定禁止优化
    QMAKE_CFLAGS_RELEASE   += -g               # 生成C调试信息
    QMAKE_CXXFLAGS_RELEASE -= -O2              # 取消C++优化
    QMAKE_CXXFLAGS_RELEASE += -O0              # 显示指定禁止优化
    QMAKE_CXXFLAGS_RELEASE += -g               # 生成C++调试信息
    QMAKE_LFLAGS_RELEASE   -= -Wl,-s           # 取消Release模式删除所有符号表和重新定位信息的设置
    QMAKE_LFLAGS_RELEASE   += -g               # 链接器生成调试信息
    message( Mingw release)
}

# 如果不加unix，MinGW也会进入这里
unix:gcc:CONFIG(release, debug|release) {
    QMAKE_CFLAGS_RELEASE   -= -O2              # 取消C优化
    QMAKE_CFLAGS_RELEASE   += -O0              # 显示指定禁止优化
    QMAKE_CFLAGS_RELEASE   += -g               # 生成C调试信息
    QMAKE_CXXFLAGS_RELEASE -= -O2              # 取消C++优化
    QMAKE_CXXFLAGS_RELEASE += -O0              # 显示指定禁止优化
    QMAKE_CXXFLAGS_RELEASE += -g               # 生成C++调试信息
    QMAKE_LFLAGS_RELEASE   -= -Wl,-O1          # 取消Release模式链接器优化
    QMAKE_LFLAGS_RELEASE   += -g               # 链接器生成调试信息
    message( GCC Release )
}

*msvc* {
    QMAKE_CXXFLAGS += /MP #使用多线程库
}
# DEFINES +=QT_NO_DEBUG_OUTPUT # disable debug output
#DEFINES -=QT_NO_DEBUG_OUTPUT # enable debug output

#禁用qdebug打印输出
#DEFINES += QT_NO_DEBUG_OUTPUT

#设置Release模式可调试
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

# 头文件路径
INCLUDEPATH += ./

#以下为pri引入依赖文件
INCLUDEPATH += $$PWD/3rd_qcustomplot/3rd_qcustomplot
include( $$PWD/3rd_qcustomplot/3rd_qcustomplot/3rd_qcustomplot.pri )

# 源文件列表
SOURCES += main.cpp \
           DataCacheManager.cpp \
           SerialReceiver.cpp \
           PlotWindow.cpp \
           PlotWindowBase.cpp \
           HeatMapPlotWindow.cpp \
           ArrayPlotWindow.cpp \
           DataProcessor.cpp \
           ApplicationController.cpp \
           AppConfig.cpp \
           PlotWindowManager.cpp \
           MainWindow.cpp

# 头文件列表
HEADERS += FrameData.h \
           DataCacheManager.h \
           SerialReceiver.h \
           PlotWindow.h \
           PlotWindowBase.h \
           HeatMapPlotWindow.h \
           ArrayPlotWindow.h \
           DataProcessor.h \
           ApplicationController.h \
           AppConfig.h \
           PlotWindowManager.h \
           MainWindow.h

# UI文件列表
FORMS += MainWindow.ui

# 资源文件列表
RESOURCES += realtime_data.qrc

# 编译选项
!msvc {
    QMAKE_CXXFLAGS += -Wall -Wextra
}
msvc {
    QMAKE_CXXFLAGS += /W3
}

#中文乱码
msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
