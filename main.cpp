#include <QApplication>
#include <QThread>
#include <QDebug>
#include "DataCacheManager.h"
#include "SerialReceiver.h"
#include "PlotWindow.h"
#include "DataProcessor.h"
#include "FrameData.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 注册FrameData类型用于跨线程信号槽
    qRegisterMetaType<FrameData>("FrameData");

    // ========== 1. 初始化数据缓存 ==========
    auto cacheMgr = DataCacheManager::instance();
    cacheMgr->setMaxCacheSize(600);    // 缓存600帧（10Hz=1分钟）
    cacheMgr->setExpireTimeMs(60000); // 数据1分钟过期

    // ========== 2. 初始化串口接收线程 ==========
    QThread* serialThread = new QThread;
    SerialReceiver* serialReceiver = new SerialReceiver;
    serialReceiver->moveToThread(serialThread);
    serialThread->start();

    // 【可选】打开真实串口（注释掉下面一行，启用模拟数据）
    // QMetaObject::invokeMethod(serialReceiver, "openSerial", Qt::QueuedConnection, Q_ARG(QString, "COM3"), Q_ARG(int, 115200));
    
    // 【推荐】先启用模拟数据测试（无硬件时）
    QMetaObject::invokeMethod(serialReceiver, "startMockData", Qt::QueuedConnection, Q_ARG(int, 100));

    // ========== 3. 初始化应用层模块 ==========
    PlotWindow plotWindow;    // 实时绘图窗口
    DataProcessor processor;  // 数据统计模块

    // 显示绘图窗口
    plotWindow.show();
    //qDebug()<<"###############";
    // ========== 4. 程序退出清理 ==========
    QObject::connect(&a, &QApplication::aboutToQuit, [&]() {
        serialReceiver->closeSerial();
        serialThread->quit();
        serialThread->wait(1000);
        serialThread->deleteLater();
        qInfo() << "程序正常退出";
    });

    return a.exec();
}
