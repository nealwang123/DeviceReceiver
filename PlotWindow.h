#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include <QWidget>
#include <QTimer>
#include "qcustomplot.h"
#include "FrameData.h"

// QCustomPlot实时绘图窗口（定时器控制刷新频率）
class PlotWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWindow(QWidget *parent = nullptr);
    ~PlotWindow() override;

private slots:
    void onRefreshTimer();          // 定时器刷新绘图
    void onCriticalFrame(const FrameData& frame); // 报警帧处理

private:
    void initPlot();                // 初始化绘图配置

private:
    QCustomPlot* m_plot;
    QTimer* m_refreshTimer;         // 绘图定时器（控制刷新频率）
    
    // 本地绘图缓存（减少全局缓存访问）
    QVector<double> m_xTime;        // 时间轴
    QVector<double> m_yTemp;        // 温度数据
    QVector<double> m_yHumidity;    // 湿度数据
    const int MAX_PLOT_POINTS = 200;// 最大绘图点数（避免卡顿）
};

#endif // PLOTWINDOW_H