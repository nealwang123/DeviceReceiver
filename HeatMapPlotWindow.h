#ifndef HEATMAPPLOTWINDOW_H
#define HEATMAPPLOTWINDOW_H

#include "PlotWindow.h"
#include <QVector>
#include <QLabel>
#include <QTimer>
#include <QSpinBox>
#include <QPushButton>
#include <QDoubleSpinBox>

class QCustomPlot;
class QCPColorMap;
class QCPColorScale;

class HeatMapPlotWindow : public PlotWindow
{
    Q_OBJECT
public:
    explicit HeatMapPlotWindow(QWidget *parent = nullptr);
    ~HeatMapPlotWindow() override;

    // 公有接口：与 FrameData 的连接点（暂未实现绑定，仅占位）
    void setGridSize(int width, int height);
    void applyGrid(const QVector<double>& values, int width, int height);
    void updateFromFrame(const FrameData& frame);
    void exportImage(const QString& filename);

public slots:
    void onDataUpdated(const QVector<FrameData>& frames) override;
    void onCriticalFrame(const FrameData& frame) override;
    
    // 用户交互槽
    void onGridWidthChanged(int width);
    void onGridHeightChanged(int height);
    void onDataMinChanged(double min);
    void onDataMaxChanged(double max);
    void onStartMockDataClicked();
    void onStopMockDataClicked();
    void onExportClicked();

private:
    void initUI();
    void initHeatMap();
    void updateHeatMapDisplay();
    void generateMockData();
    void setColorGradient();

private:
    QCustomPlot* m_plot{nullptr};
    QCPColorMap* m_colorMap{nullptr};
    QCPColorScale* m_colorScale{nullptr};

    int m_gridWidth{1024};
    int m_gridHeight{128};
    QVector<double> m_gridData;  // 线性存储：row-major 顺序
    
    double m_dataMin{0.0};
    double m_dataMax{100.0};
    double m_displayMin{0.0};
    double m_displayMax{100.0};

    // UI 控件
    QSpinBox* m_widthSpinBox{nullptr};
    QSpinBox* m_heightSpinBox{nullptr};
    QDoubleSpinBox* m_minSpinBox{nullptr};
    QDoubleSpinBox* m_maxSpinBox{nullptr};
    QPushButton* m_startButton{nullptr};
    QPushButton* m_stopButton{nullptr};
    QPushButton* m_exportButton{nullptr};
    QLabel* m_statsLabel{nullptr};

    QTimer* m_mockDataTimer{nullptr};
    bool m_useMockData{false};
    qint64 m_frameCount{0};
};

#endif // HEATMAPPLOTWINDOW_H