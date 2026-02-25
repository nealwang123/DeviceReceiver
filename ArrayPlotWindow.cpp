#include "ArrayPlotWindow.h"
#include "FrameData.h"
#include "qcustomplot.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <cmath>
#include <algorithm>

ArrayPlotWindow::ArrayPlotWindow(QWidget *parent)
    : PlotWindow(parent)
{
    setWindowTitle(QStringLiteral("阵列图窗口"));
    resize(1200, 700);
    
    // 清除继承的布局
    QLayout* existingLayout = layout();
    if (existingLayout) {
        QLayoutItem* item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
    }
    
    // 创建新的主布局
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // 创建控制按钮面板
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QPushButton *startButton = new QPushButton(QStringLiteral("启动模拟"));
    QPushButton *stopButton = new QPushButton(QStringLiteral("停止模拟"));
    stopButton->setEnabled(false);
    m_statsLabel = new QLabel(QStringLiteral("状态：就绪"));
    
    controlLayout->addWidget(startButton);
    controlLayout->addWidget(stopButton);
    controlLayout->addWidget(m_statsLabel);
    controlLayout->addStretch();
    
    // 创建QCustomPlot实例
    m_plot = new QCustomPlot(this);
    
    // 初始化数组图
    initArrayPlot();
    
    // 生成初始的虚拟数据
    for (int i = 0; i < m_maxDataPoints; i++) {
        double t = i * 0.1;
        m_timeAxis.append(t);
        m_temperatureValues.append(20.0 + 10.0 * std::sin(t));
        m_humidityValues.append(50.0 + 30.0 * std::cos(t));
        m_voltageValues.append(5.0 + 2.0 * std::sin(t + 1.0));
    }
    
    // 更新绘图
    updateArrayData();
    
    // 添加到主布局
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_plot, 1);
    centralWidget->setLayout(mainLayout);
    
    // 创建主窗口布局并添加中心部件
    if (!this->layout()) {
        setLayout(new QVBoxLayout());
    }
    this->layout()->addWidget(centralWidget);
    
    // 设置定时器用于模拟数据更新
    m_mockDataTimer = new QTimer(this);
    connect(m_mockDataTimer, &QTimer::timeout, this, &ArrayPlotWindow::generateMockData);
    
    // 按钮信号连接
    connect(startButton, &QPushButton::clicked, this, [this, startButton, stopButton]() {
        m_useMockData = true;
        m_frameCount = 0;
        m_mockDataTimer->start(100); // 每100ms更新一次
        m_statsLabel->setText(QStringLiteral("状态：模拟数据运行中..."));
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
    });
    
    connect(stopButton, &QPushButton::clicked, this, [this, startButton, stopButton]() {
        m_useMockData = false;
        m_mockDataTimer->stop();
        m_statsLabel->setText(QStringLiteral("状态：已停止"));
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
    });
}

ArrayPlotWindow::~ArrayPlotWindow()
{
    if (m_mockDataTimer) {
        m_mockDataTimer->stop();
        m_mockDataTimer->deleteLater();
    }
}

void ArrayPlotWindow::initArrayPlot()
{
    if (!m_plot) return;
    
    // 清除已有的布局和图
    m_plot->clearPlottables();
    m_plot->clearItems();
    m_plot->plotLayout()->clear();
    
    // 创建3个QCPAxisRect（垂直堆叠）
    // 第1行：温度
    m_axisRectTemp = new QCPAxisRect(m_plot, true);
    m_plot->plotLayout()->addElement(0, 0, m_axisRectTemp);
    
    // 第2行：湿度
    m_axisRectHumid = new QCPAxisRect(m_plot, true);
    m_plot->plotLayout()->addElement(1, 0, m_axisRectHumid);
    
    // 第3行：电压
    m_axisRectVolt = new QCPAxisRect(m_plot, true);
    m_plot->plotLayout()->addElement(2, 0, m_axisRectVolt);
    
    // 为每个轴矩形创建图
    QCPGraph *graphTemp = m_plot->addGraph(m_axisRectTemp->axis(QCPAxis::atBottom), m_axisRectTemp->axis(QCPAxis::atLeft));
    QCPGraph *graphHumid = m_plot->addGraph(m_axisRectHumid->axis(QCPAxis::atBottom), m_axisRectHumid->axis(QCPAxis::atLeft));
    QCPGraph *graphVolt = m_plot->addGraph(m_axisRectVolt->axis(QCPAxis::atBottom), m_axisRectVolt->axis(QCPAxis::atLeft));
    
    // 设置线条颜色和样式
    graphTemp->setPen(QPen(Qt::blue, 2));
    graphHumid->setPen(QPen(Qt::green, 2));
    graphVolt->setPen(QPen(Qt::red, 2));
    
    // 配置Y轴标签和范围
    // 温度轴
    m_axisRectTemp->axis(QCPAxis::atLeft)->setLabel(QStringLiteral("温度 (°C)"));
    m_axisRectTemp->axis(QCPAxis::atLeft)->setRange(0, 40);
    m_axisRectTemp->axis(QCPAxis::atBottom)->setVisible(false);
    
    // 湿度轴
    m_axisRectHumid->axis(QCPAxis::atLeft)->setLabel(QStringLiteral("湿度 (%RH)"));
    m_axisRectHumid->axis(QCPAxis::atLeft)->setRange(0, 100);
    m_axisRectHumid->axis(QCPAxis::atBottom)->setVisible(false);
    
    // 电压轴（X轴在此行显示）
    m_axisRectVolt->axis(QCPAxis::atLeft)->setLabel(QStringLiteral("电压 (V)"));
    m_axisRectVolt->axis(QCPAxis::atLeft)->setRange(0, 10);
    m_axisRectVolt->axis(QCPAxis::atBottom)->setLabel(QStringLiteral("时间 (s)"));
    
    // 同步X轴缩放和平移
    QCPAxis *xAxisTemp = m_axisRectTemp->axis(QCPAxis::atBottom);
    QCPAxis *xAxisHumid = m_axisRectHumid->axis(QCPAxis::atBottom);
    QCPAxis *xAxisVolt = m_axisRectVolt->axis(QCPAxis::atBottom);
    
    connect(xAxisTemp, static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            xAxisHumid, [xAxisHumid](const QCPRange &newRange) { xAxisHumid->setRange(newRange); });
    connect(xAxisTemp, static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            xAxisVolt, [xAxisVolt](const QCPRange &newRange) { xAxisVolt->setRange(newRange); });
    connect(xAxisHumid, static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            xAxisVolt, [xAxisVolt](const QCPRange &newRange) { xAxisVolt->setRange(newRange); });
    
    m_plot->replot();
}

void ArrayPlotWindow::generateMockData()
{
    if (!m_useMockData) return;
    
    // 追加新的数据点
    double t = m_timeAxis.isEmpty() ? 0.0 : m_timeAxis.last() + 0.1;
    m_timeAxis.append(t);
    
    // 生成虚拟数据（使用不同相位的正弦波）
    m_temperatureValues.append(20.0 + 10.0 * std::sin(t));
    m_humidityValues.append(50.0 + 30.0 * std::cos(t));
    m_voltageValues.append(5.0 + 2.0 * std::sin(t + 1.0));
    
    // 如果数据超过最大点数，删除最旧的数据
    if (m_timeAxis.size() > m_maxDataPoints) {
        m_timeAxis.removeFirst();
        m_temperatureValues.removeFirst();
        m_humidityValues.removeFirst();
        m_voltageValues.removeFirst();
    }
    
    m_frameCount++;
    updateArrayData();
}

void ArrayPlotWindow::updateArrayData()
{
    if (!m_plot || m_plot->graphCount() < 3) return;
    
    QCPGraph *graphTemp = m_plot->graph(0);
    QCPGraph *graphHumid = m_plot->graph(1);
    QCPGraph *graphVolt = m_plot->graph(2);
    
    // 设置数据
    graphTemp->setData(m_timeAxis, m_temperatureValues);
    graphHumid->setData(m_timeAxis, m_humidityValues);
    graphVolt->setData(m_timeAxis, m_voltageValues);
    
    // 自动范围缩放每个Y轴
    if (!m_temperatureValues.isEmpty()) {
        auto [minTemp, maxTemp] = std::minmax_element(m_temperatureValues.begin(), m_temperatureValues.end());
        double rangeTemp = *maxTemp - *minTemp;
        if (rangeTemp < 0.1) rangeTemp = 0.1;
        m_axisRectTemp->axis(QCPAxis::atLeft)->setRange(*minTemp - 0.1*rangeTemp, *maxTemp + 0.1*rangeTemp);
    }
    
    if (!m_humidityValues.isEmpty()) {
        auto [minHumid, maxHumid] = std::minmax_element(m_humidityValues.begin(), m_humidityValues.end());
        double rangeHumid = *maxHumid - *minHumid;
        if (rangeHumid < 0.1) rangeHumid = 0.1;
        m_axisRectHumid->axis(QCPAxis::atLeft)->setRange(*minHumid - 0.1*rangeHumid, *maxHumid + 0.1*rangeHumid);
    }
    
    if (!m_voltageValues.isEmpty()) {
        auto [minVolt, maxVolt] = std::minmax_element(m_voltageValues.begin(), m_voltageValues.end());
        double rangeVolt = *maxVolt - *minVolt;
        if (rangeVolt < 0.1) rangeVolt = 0.1;
        m_axisRectVolt->axis(QCPAxis::atLeft)->setRange(*minVolt - 0.1*rangeVolt, *maxVolt + 0.1*rangeVolt);
    }
    
    // 设置X轴范围
    if (!m_timeAxis.isEmpty()) {
        m_axisRectTemp->axis(QCPAxis::atBottom)->setRange(m_timeAxis.first(), m_timeAxis.last());
        m_axisRectHumid->axis(QCPAxis::atBottom)->setRange(m_timeAxis.first(), m_timeAxis.last());
        m_axisRectVolt->axis(QCPAxis::atBottom)->setRange(m_timeAxis.first(), m_timeAxis.last());
    }
    
    m_plot->replot();
}

void ArrayPlotWindow::onDataUpdated(const QVector<FrameData>& frames)
{
    if (frames.isEmpty() || m_useMockData) return;
    
    for (const FrameData& frame : frames) {
        double t = m_timeAxis.isEmpty() ? 0.0 : m_timeAxis.last() + 0.1;
        m_timeAxis.append(t);
        m_temperatureValues.append(frame.temperature);
        m_humidityValues.append(frame.humidity);
        m_voltageValues.append(frame.voltage);
        
        if (m_timeAxis.size() > m_maxDataPoints) {
            m_timeAxis.removeFirst();
            m_temperatureValues.removeFirst();
            m_humidityValues.removeFirst();
            m_voltageValues.removeFirst();
        }
    }
    
    updateArrayData();
}

void ArrayPlotWindow::onCriticalFrame(const FrameData& frame)
{
    QString alarmMsg = QStringLiteral("警报！帧ID:%1 T:%2°C H:%3%RH U:%4V")
        .arg(frame.frameId)
        .arg(frame.temperature, 0, 'f', 2)
        .arg(frame.humidity, 0, 'f', 2)
        .arg(frame.voltage, 0, 'f', 2);
    
    m_statsLabel->setText(alarmMsg);
}
