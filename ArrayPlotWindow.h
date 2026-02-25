#ifndef ARRAYPLOTWINDOW_H
#define ARRAYPLOTWINDOW_H

#include "PlotWindow.h"
#include <QVector>
#include <QLabel>
#include <QTimer>

class QCustomPlot;
class QCPAxisRect;

class ArrayPlotWindow : public PlotWindow
{
    Q_OBJECT
public:
    explicit ArrayPlotWindow(QWidget *parent = nullptr);
    ~ArrayPlotWindow() override;

public slots:
    void onDataUpdated(const QVector<FrameData>& frames) override;
    void onCriticalFrame(const FrameData& frame) override;

private:
    void initArrayPlot();
    void updateArrayData();
    void generateMockData();

private:
    QCustomPlot* m_plot{nullptr};
    QCPAxisRect* m_axisRectTemp{nullptr};
    QCPAxisRect* m_axisRectHumid{nullptr};
    QCPAxisRect* m_axisRectVolt{nullptr};
    QLabel* m_statsLabel{nullptr};
    QTimer* m_mockDataTimer{nullptr};
    
    QVector<double> m_timeAxis;           // 共享时间轴
    QVector<double> m_temperatureValues;
    QVector<double> m_humidityValues;
    QVector<double> m_voltageValues;
    
    int m_maxDataPoints{100};
    bool m_useMockData{false};
    qint64 m_frameCount{0};
};

#endif // ARRAYPLOTWINDOW_H
