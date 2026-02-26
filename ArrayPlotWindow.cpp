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
    setWindowTitle(QStringLiteral("多通道阵列图窗口"));
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
    
    // 初始化数组图（默认8个通道）
    initArrayPlot();
    
    // 生成初始的虚拟数据
    for (int i = 0; i < m_maxDataPoints; i++) {
        double t = i * 0.1;
        m_timeAxis.append(t);
        
        // 为每个通道生成数据
        for (int ch = 0; ch < m_currentChannelCount; ch++) {
            if (ch >= m_channelValues.size()) {
                m_channelValues.resize(ch + 1);
                m_channelValues2.resize(ch + 1);
            }
            double val = 10.0 + 3.0 * ch + 5.0 * std::sin(t * (0.5 + ch * 0.04));
            m_channelValues[ch].append(val);
            
            // 复数模式的第二分量（如果启用）
            if (ch < m_channelValues2.size()) {
                m_channelValues2[ch].append(5.0 * std::cos(t * (0.5 + ch * 0.04)));
            }
        }
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
    
    // 设置默认通道数
    m_currentChannelCount = 8;
    m_channelAxisRects.clear();
    
    // 创建通道轴矩形（垂直堆叠）
    for (int i = 0; i < m_currentChannelCount; i++) {
        QCPAxisRect* axisRect = new QCPAxisRect(m_plot, true);
        m_plot->plotLayout()->addElement(i, 0, axisRect);
        m_channelAxisRects.append(axisRect);
        
        // 为每个轴矩形创建图
        QCPGraph* graph = m_plot->addGraph(axisRect->axis(QCPAxis::atBottom), axisRect->axis(QCPAxis::atLeft));
        
        // 设置线条颜色和样式
        QColor color = QColor::fromHsv((i * 36) % 360, 200, 200);
        graph->setPen(QPen(color, 2));
        
        // 配置Y轴标签
        axisRect->axis(QCPAxis::atLeft)->setLabel(QStringLiteral("通道 %1").arg(i + 1));
        
        // 隐藏底部X轴（除了最后一个）
        if (i < m_currentChannelCount - 1) {
            axisRect->axis(QCPAxis::atBottom)->setVisible(false);
        } else {
            axisRect->axis(QCPAxis::atBottom)->setLabel(QStringLiteral("时间 (s)"));
        }
    }
    
    // 同步所有X轴缩放和平移
    for (int i = 0; i < m_currentChannelCount - 1; i++) {
        QCPAxis* xAxisCurrent = m_channelAxisRects[i]->axis(QCPAxis::atBottom);
        QCPAxis* xAxisNext = m_channelAxisRects[i + 1]->axis(QCPAxis::atBottom);
        
        connect(xAxisCurrent, static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
                xAxisNext, [xAxisNext](const QCPRange &newRange) { xAxisNext->setRange(newRange); });
    }
    
    // 调整数据容器大小
    m_channelValues.resize(m_currentChannelCount);
    m_channelValues2.resize(m_currentChannelCount);
    
    m_plot->replot();
}

void ArrayPlotWindow::generateMockData()
{
    if (!m_useMockData) return;
    
    // 追加新的数据点
    double t = m_timeAxis.isEmpty() ? 0.0 : m_timeAxis.last() + 0.1;
    m_timeAxis.append(t);
    
    // 生成虚拟数据
    for (int ch = 0; ch < m_currentChannelCount; ch++) {
        if (ch >= m_channelValues.size()) {
            m_channelValues.resize(ch + 1);
            m_channelValues2.resize(ch + 1);
        }
        
        double val = 10.0 + 3.0 * ch + 5.0 * std::sin(t * (0.5 + ch * 0.04) + (m_frameCount * 0.01));
        m_channelValues[ch].append(val);
        
        // 如果是复数模式，生成第二分量
        if (ch < m_channelValues2.size() && m_lastMode == FrameData::MultiChannelComplex) {
            m_channelValues2[ch].append(5.0 * std::cos(t * (0.5 + ch * 0.04) + (m_frameCount * 0.01)));
        }
    }
    
    // 如果数据超过最大点数，删除最旧的数据
    if (m_timeAxis.size() > m_maxDataPoints) {
        m_timeAxis.removeFirst();
        for (auto &vec : m_channelValues) {
            if (!vec.isEmpty()) vec.removeFirst();
        }
        for (auto &vec : m_channelValues2) {
            if (!vec.isEmpty()) vec.removeFirst();
        }
    }
    
    m_frameCount++;
    updateArrayData();
}

void ArrayPlotWindow::updateArrayData()
{
    if (!m_plot || m_plot->graphCount() < m_currentChannelCount) return;
    
    // 更新每个通道的数据
    for (int i = 0; i < m_currentChannelCount; i++) {
        if (i < m_plot->graphCount()) {
            QCPGraph* graph = m_plot->graph(i);
            graph->setData(m_timeAxis, m_channelValues[i]);
            
            // 自动范围缩放每个Y轴
            if (!m_channelValues[i].isEmpty()) {
                auto [minVal, maxVal] = std::minmax_element(m_channelValues[i].begin(), m_channelValues[i].end());
                double range = *maxVal - *minVal;
                if (range < 0.1) range = 0.1;
                if (i < m_channelAxisRects.size()) {
                    m_channelAxisRects[i]->axis(QCPAxis::atLeft)->setRange(*minVal - 0.1*range, *maxVal + 0.1*range);
                }
            }
        }
    }
    
    // 设置X轴范围（共享）
    if (!m_timeAxis.isEmpty() && !m_channelAxisRects.isEmpty()) {
        double firstTime = m_timeAxis.first();
        double lastTime = m_timeAxis.last();
        
        for (auto axisRect : m_channelAxisRects) {
            axisRect->axis(QCPAxis::atBottom)->setRange(firstTime, lastTime);
        }
    }
    
    m_plot->replot();
}

void ArrayPlotWindow::onDataUpdated(const QVector<FrameData>& frames)
{
    if (frames.isEmpty() || m_useMockData) return;
    
    for (const FrameData& frame : frames) {
        // 检查模式变化
        if (frame.detectMode != m_lastMode || frame.channelCount != m_currentChannelCount) {
            // 模式或通道数变化，需要重建布局
            m_lastMode = frame.detectMode;
            m_currentChannelCount = frame.channelCount;
            
            // 清空现有数据
            m_timeAxis.clear();
            m_channelValues.clear();
            m_channelValues2.clear();
            
            // 重新初始化绘图
            initArrayPlot();
        }
        
        double t = m_timeAxis.isEmpty() ? 0.0 : m_timeAxis.last() + 0.1;
        m_timeAxis.append(t);
        
        // 根据模式处理数据
        if (frame.detectMode == FrameData::MultiChannelReal) {
            // 实数模式
            for (int ch = 0; ch < m_currentChannelCount; ch++) {
                if (ch >= m_channelValues.size()) {
                    m_channelValues.resize(ch + 1);
                }
                double val = (ch < frame.channels_comp0.size()) ? frame.channels_comp0.at(ch) : qQNaN();
                m_channelValues[ch].append(val);
            }
        } else if (frame.detectMode == FrameData::MultiChannelComplex) {
            // 复数模式
            for (int ch = 0; ch < m_currentChannelCount; ch++) {
                if (ch >= m_channelValues.size()) {
                    m_channelValues.resize(ch + 1);
                    m_channelValues2.resize(ch + 1);
                }
                double re = (ch < frame.channels_comp0.size()) ? frame.channels_comp0.at(ch) : qQNaN();
                double im = (ch < frame.channels_comp1.size()) ? frame.channels_comp1.at(ch) : qQNaN();
                m_channelValues[ch].append(re);
                m_channelValues2[ch].append(im);
            }
        }
        
        // 如果数据超过最大点数，删除最旧的数据
        if (m_timeAxis.size() > m_maxDataPoints) {
            m_timeAxis.removeFirst();
            for (auto &vec : m_channelValues) {
                if (!vec.isEmpty()) vec.removeFirst();
            }
            for (auto &vec : m_channelValues2) {
                if (!vec.isEmpty()) vec.removeFirst();
            }
        }
    }
    
    updateArrayData();
}

void ArrayPlotWindow::onCriticalFrame(const FrameData& frame)
{
    QString alarmMsg;
    if (frame.detectMode == FrameData::MultiChannelReal) {
        alarmMsg = QStringLiteral("警报！帧ID:%1 实数模式 通道数:%2")
            .arg(frame.frameId)
            .arg(frame.channelCount);
    } else if (frame.detectMode == FrameData::MultiChannelComplex) {
        alarmMsg = QStringLiteral("警报！帧ID:%1 复数模式 通道数:%2")
            .arg(frame.frameId)
            .arg(frame.channelCount);
    } else {
        alarmMsg = QStringLiteral("警报！帧ID:%1 Legacy模式（已弃用）")
            .arg(frame.frameId);
    }
    
    m_statsLabel->setText(alarmMsg);
}