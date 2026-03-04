#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QSharedPointer>

// 前向声明
class PlotWindow;
class DataGenerator;
class PerformanceMonitor;
class QLabel;
class QSpinBox;
class QSlider;
class QCheckBox;
class QPushButton;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopClicked();
    void onUpdateIntervalChanged(int value);
    void onChannelCountChanged(int windowIndex, int count);
    void onOpenGLToggled(bool enabled);
    void updatePerformanceStats();
    void updateData();

private:
    void setupUI();
    void setupConnections();
    void startTest();
    void stopTest();
    
    // UI组件
    QVector<QSpinBox*> m_channelSpins;
    QVector<QLabel*> m_windowLabels;
    QSlider* m_updateIntervalSlider;
    QLabel* m_intervalLabel;
    QCheckBox* m_openglCheckbox;
    QPushButton* m_startStopButton;
    
    // 性能显示
    QLabel* m_fpsLabel;
    QLabel* m_cpuLabel;
    QLabel* m_memoryLabel;
    QLabel* m_dataRateLabel;
    
    // 核心组件
    QVector<PlotWindow*> m_plotWindows;
    QSharedPointer<DataGenerator> m_dataGenerator;
    QSharedPointer<PerformanceMonitor> m_performanceMonitor;
    
    // 定时器
    QTimer* m_updateTimer;
    QTimer* m_statsTimer;
    
    // 状态
    bool m_isRunning;
    int m_updateInterval; // 毫秒
    int m_totalDataPoints;
    
    static constexpr int WINDOW_COUNT = 3;
    static constexpr int MAX_CHANNELS_PER_WINDOW = 40;
    static constexpr int HISTORY_POINTS = 1000;  // 每条曲线最多1000个历史点
    static constexpr int MIN_UPDATE_INTERVAL = 10;   // 毫秒
    static constexpr int MAX_UPDATE_INTERVAL = 1000; // 毫秒
};

#endif // MAINWINDOW_H