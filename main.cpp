#include <QApplication>
#include <QDebug>
#include "FrameData.h"
#include "ApplicationController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 注册FrameData类型用于跨线程信号槽
    qRegisterMetaType<FrameData>("FrameData");

    // 创建应用控制器
    ApplicationController controller;
    
    // 初始化所有模块
    if (!controller.initialize()) {
        qCritical() << "应用初始化失败，程序退出";
        return -1;
    }
    
    // 启动应用（显示窗口，开始数据接收）
    controller.start();
    
    // 程序退出清理
    QObject::connect(&app, &QApplication::aboutToQuit, [&controller]() {
        controller.stop();
        qInfo() << "程序正常退出";
    });

    return app.exec();
}
