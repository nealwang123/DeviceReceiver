#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("QCustomPlot Performance Demo");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("PlotTest");
    
    // 设置全局样式（使用Fusion风格，更现代化）
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 创建主窗口
    MainWindow w;
    w.show();
    
    return a.exec();
}