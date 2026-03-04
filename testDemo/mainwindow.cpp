#include "mainwindow.h"
#include "plotwindow.h"
#include "datagenerator.h"
#include "performancemonitor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QStatusBar>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QApplication>
#include <QThread>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isRunning(false)
    , m_updateInterval(100) // 默认100ms
    , m_totalDataPoints(0)
{
    // 设置窗口属性
    setWindowTitle("QCustomPlot性能测试 - 3窗口 × 40通道实时曲线");
    resize(1400, 900);
    
    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_statsTimer = new QTimer(this);
    
    // 初始化核心组件
    m_dataGenerator.reset(new DataGenerator());
    m_performanceMonitor.reset(new PerformanceMonitor());
    
    // 创建3个绘图窗口（在setupUI之前，因为setupUI会用到它们）
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        PlotWindow* plotWindow = new PlotWindow(this);
        m_plotWindows.append(plotWindow);
    }
    
    // 初始化UI（现在可以访问m_plotWindows了）
    setupUI();
    setupConnections();
    
    // 初始化绘图窗口
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        m_plotWindows[i]->initialize(i, 5); // 初始化为5个通道
    }
    
    // 设置初始通道数
    for (int i = 0; i < m_channelSpins.size(); ++i) {
        m_channelSpins[i]->setValue(5);
    }
    
    // 连接定时器
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateData);
    connect(m_statsTimer, &QTimer::timeout, this, &MainWindow::updatePerformanceStats);
    
    // 启动性能统计定时器（每秒更新一次）
    m_statsTimer->start(1000);
    
    // 更新初始性能显示
    updatePerformanceStats();
    
    // 状态栏信息
    statusBar()->showMessage("就绪 - 点击'开始测试'按钮启动性能测试");
}

MainWindow::~MainWindow()
{
    stopTest();
    
    // 清理绘图窗口
    for (PlotWindow* window : m_plotWindows) {
        window->close();
        window->deleteLater();
    }
    m_plotWindows.clear();
}

void MainWindow::setupUI()
{
    // 中央窗口
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // 控制面板组
    QGroupBox* controlGroup = new QGroupBox("测试控制", this);
    QGridLayout* controlLayout = new QGridLayout(controlGroup);
    
    // 添加窗口控制
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        QLabel* windowLabel = new QLabel(QString("窗口%1通道数:").arg(i + 1), this);
        QSpinBox* channelSpin = new QSpinBox(this);
        channelSpin->setRange(1, MAX_CHANNELS_PER_WINDOW);
        channelSpin->setValue(5);
        channelSpin->setSuffix(" 通道");
        
        // 使用lambda捕获窗口索引
        connect(channelSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                [this, i](int value) { onChannelCountChanged(i, value); });
        
        m_windowLabels.append(windowLabel);
        m_channelSpins.append(channelSpin);
        
        controlLayout->addWidget(windowLabel, i, 0);
        controlLayout->addWidget(channelSpin, i, 1);
    }
    
    // 更新间隔控制
    QLabel* intervalTitle = new QLabel("更新间隔:", this);
    m_updateIntervalSlider = new QSlider(Qt::Horizontal, this);
    m_updateIntervalSlider->setRange(MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL);
    m_updateIntervalSlider->setValue(m_updateInterval);
    m_updateIntervalSlider->setTickPosition(QSlider::TicksBelow);
    m_updateIntervalSlider->setTickInterval(100);
    
    m_intervalLabel = new QLabel(QString("%1 ms").arg(m_updateInterval), this);
    
    controlLayout->addWidget(intervalTitle, WINDOW_COUNT, 0);
    controlLayout->addWidget(m_updateIntervalSlider, WINDOW_COUNT, 1);
    controlLayout->addWidget(m_intervalLabel, WINDOW_COUNT, 2);
    
    // OpenGL控制
    QLabel* openglTitle = new QLabel("OpenGL加速:", this);
    m_openglCheckbox = new QCheckBox("启用", this);
    m_openglCheckbox->setChecked(false);
    
    controlLayout->addWidget(openglTitle, WINDOW_COUNT + 1, 0);
    controlLayout->addWidget(m_openglCheckbox, WINDOW_COUNT + 1, 1);
    
    // 开始/停止按钮
    m_startStopButton = new QPushButton("开始测试", this);
    m_startStopButton->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; }");
    
    controlLayout->addWidget(m_startStopButton, WINDOW_COUNT + 2, 0, 1, 3);
    
    // 性能显示组
    QGroupBox* perfGroup = new QGroupBox("性能统计", this);
    QGridLayout* perfLayout = new QGridLayout(perfGroup);
    
    m_fpsLabel = new QLabel("FPS: --", this);
    m_cpuLabel = new QLabel("CPU使用率: --%", this);
    m_memoryLabel = new QLabel("内存使用: -- MB", this);
    m_dataRateLabel = new QLabel("数据点/秒: --", this);
    
    perfLayout->addWidget(m_fpsLabel, 0, 0);
    perfLayout->addWidget(m_cpuLabel, 0, 1);
    perfLayout->addWidget(m_memoryLabel, 1, 0);
    perfLayout->addWidget(m_dataRateLabel, 1, 1);
    
    // 绘图窗口布局
    QWidget* plotContainer = new QWidget(this);
    QHBoxLayout* plotLayout = new QHBoxLayout(plotContainer);
    plotLayout->setSpacing(5);
    plotLayout->setContentsMargins(0, 0, 0, 0);
    
    for (PlotWindow* window : m_plotWindows) {
        plotLayout->addWidget(window);
    }
    
    // 添加到主布局
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(perfGroup);
    mainLayout->addWidget(plotContainer, 1); // 拉伸因子为1
    
    // 设置控件样式
    QString labelStyle = "QLabel { font-weight: bold; color: #333; }";
    QString valueStyle = "QLabel { color: #0066CC; font-weight: bold; }";
    
    for (QLabel* label : m_windowLabels) {
        label->setStyleSheet(labelStyle);
    }
    
    QLabel* perfLabels[] = { m_fpsLabel, m_cpuLabel, m_memoryLabel, m_dataRateLabel };
    for (QLabel* label : perfLabels) {
        label->setStyleSheet(valueStyle);
    }
}

void MainWindow::setupConnections()
{
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(m_updateIntervalSlider, &QSlider::valueChanged, this, &MainWindow::onUpdateIntervalChanged);
    connect(m_openglCheckbox, &QCheckBox::toggled, this, &MainWindow::onOpenGLToggled);
}

void MainWindow::onStartStopClicked()
{
    if (m_isRunning) {
        stopTest();
    } else {
        startTest();
    }
}

void MainWindow::onUpdateIntervalChanged(int value)
{
    m_updateInterval = value;
    m_intervalLabel->setText(QString("%1 ms").arg(value));
    
    if (m_isRunning) {
        m_updateTimer->setInterval(value);
    }
}

void MainWindow::onChannelCountChanged(int windowIndex, int count)
{
    if (windowIndex >= 0 && windowIndex < m_plotWindows.size()) {
        m_plotWindows[windowIndex]->removeAllCurves();
        m_plotWindows[windowIndex]->initialize(windowIndex, count);
        
        // 更新总数据点计算
        m_totalDataPoints = 0;
        for (int i = 0; i < m_plotWindows.size(); ++i) {
            m_totalDataPoints += m_channelSpins[i]->value() * HISTORY_POINTS;
        }
    }
}

void MainWindow::onOpenGLToggled(bool enabled)
{
    for (PlotWindow* window : m_plotWindows) {
        window->enableOpenGL(enabled);
    }
    
    statusBar()->showMessage(enabled ? "OpenGL加速已启用" : "OpenGL加速已禁用");
}

void MainWindow::updatePerformanceStats()
{
    if (!m_performanceMonitor) return;
    
    // 记录性能监控数据
    m_performanceMonitor->recordFrame();
    
    // 更新显示
    double fps = m_performanceMonitor->currentFps();
    m_fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    m_cpuLabel->setText(QString("CPU使用率: %1%").arg(m_performanceMonitor->cpuUsage(), 0, 'f', 1));
    m_memoryLabel->setText(QString("内存使用: %1 MB").arg(m_performanceMonitor->memoryUsageMB(), 0, 'f', 1));
    
    // 计算数据速率
    if (m_isRunning) {
        int channels = 0;
        for (QSpinBox* spin : m_channelSpins) {
            channels += spin->value();
        }
        int dataPointsPerSecond = (channels * 1000) / m_updateInterval;
        m_dataRateLabel->setText(QString("数据点/秒: %1").arg(dataPointsPerSecond));
    } else {
        m_dataRateLabel->setText("数据点/秒: --");
    }
}

void MainWindow::updateData()
{
    if (!m_dataGenerator || !m_isRunning) return;
    
    double currentTime = m_dataGenerator->currentTime();
    
    for (int windowIdx = 0; windowIdx < m_plotWindows.size(); ++windowIdx) {
        PlotWindow* window = m_plotWindows[windowIdx];
        int channelCount = m_channelSpins[windowIdx]->value();
        
        // 生成数据：1000个点，代表最近1秒
        QVector<QVector<double>> data = m_dataGenerator->generateData(channelCount);
        
        // 获取曲线ID
        QVector<int> curveIds = window->getCurveIds();
        
        // 确保曲线数量正确
        while (curveIds.size() < channelCount) {
            int id = window->addCurve(QString("CH%1").arg(curveIds.size() + 1));
            curveIds.append(id);
        }
        
        // 生成时间轴：1000个点，每个点间隔100ms，总共100秒的历史
        // 但只显示最近10秒（100个点），这样滚动会更清晰
        QVector<double> timeAxis;
        timeAxis.reserve(HISTORY_POINTS);
        for (int i = 0; i < HISTORY_POINTS; ++i) {
            double t = currentTime - 100.0 + i * 0.1;  // 数据范围：t-100秒到t
            timeAxis.append(t);
        }
        
        // 更新每条曲线的数据
        for (int chIdx = 0; chIdx < channelCount && chIdx < data.size(); ++chIdx) {
            window->setCurveData(curveIds[chIdx], timeAxis, data[chIdx]);
        }
        
        // 设置显示范围：x轴只显示最近10秒（更清晰的滚动效果）
        // 这样用户可以看到曲线滚动过来
        window->setAxisRange(currentTime - 10.0, currentTime, -1.5, 1.5);
        window->replot();
    }
    
    m_performanceMonitor->recordFrame();
}

void MainWindow::startTest()
{
    if (m_isRunning) return;
    
    // 启动数据生成器
    m_dataGenerator->start();
    
    // 启动更新定时器
    m_updateTimer->start(m_updateInterval);
    
    // 更新UI状态
    m_isRunning = true;
    m_startStopButton->setText("停止测试");
    m_startStopButton->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; background-color: #FF6B6B; color: white; }");
    
    // 禁用一些控件
    for (QSpinBox* spin : m_channelSpins) {
        spin->setEnabled(false);
    }
    m_openglCheckbox->setEnabled(false);
    
    statusBar()->showMessage(QString("测试运行中 - 更新间隔: %1ms").arg(m_updateInterval));
}

void MainWindow::stopTest()
{
    if (!m_isRunning) return;
    
    // 停止定时器
    m_updateTimer->stop();
    
    // 停止数据生成器
    m_dataGenerator->stop();
    
    // 更新UI状态
    m_isRunning = false;
    m_startStopButton->setText("开始测试");
    m_startStopButton->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; background-color: #4CAF50; color: white; }");
    
    // 启用控件
    for (QSpinBox* spin : m_channelSpins) {
        spin->setEnabled(true);
    }
    m_openglCheckbox->setEnabled(true);
    
    statusBar()->showMessage("测试已停止");
}