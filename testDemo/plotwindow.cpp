#include "plotwindow.h"
#include "qcustomplot.h"
#include <QVBoxLayout>
#include <QDebug>

// 预定义的40种不同颜色，用于区分不同通道
const QColor PlotWindow::CURVE_COLORS[COLOR_COUNT] = {
    QColor(31, 119, 180),    // 0: Blue
    QColor(255, 127, 14),    // 1: Orange
    QColor(44, 160, 44),     // 2: Green
    QColor(214, 39, 40),     // 3: Red
    QColor(148, 103, 189),   // 4: Purple
    QColor(140, 86, 75),     // 5: Brown
    QColor(227, 119, 194),   // 6: Pink
    QColor(127, 127, 127),   // 7: Gray
    QColor(188, 143, 143),   // 8: Dark Gray
    QColor(23, 190, 207),    // 9: Cyan
    QColor(65, 105, 225),    // 10: Royal Blue
    QColor(220, 20, 60),     // 11: Crimson
    QColor(34, 139, 34),     // 12: Forest Green
    QColor(184, 134, 11),    // 13: Dark Goldenrod
    QColor(72, 61, 139),     // 14: Dark Slate Blue
    QColor(178, 34, 34),     // 15: Firebrick
    QColor(0, 100, 0),       // 16: Dark Green
    QColor(105, 105, 105),   // 17: Dim Gray
    QColor(139, 69, 19),     // 18: Saddle Brown
    QColor(0, 0, 205),       // 19: Medium Blue
    QColor(220, 20, 60),     // 20: Crimson
    QColor(0, 206, 209),     // 21: Dark Turquoise
    QColor(148, 0, 211),     // 22: Dark Violet
    QColor(255, 20, 147),    // 23: Deep Pink
    QColor(0, 191, 255),     // 24: Deep Sky Blue
    QColor(184, 134, 11),    // 25: Dark Goldenrod
    QColor(139, 0, 0),       // 26: Dark Red
    QColor(0, 139, 139),     // 27: Dark Cyan
    QColor(85, 107, 47),     // 28: Dark Olive Green
    QColor(139, 35, 69),     // 29: Dark Slate Gray
    QColor(255, 69, 0),      // 30: Orange Red
    QColor(0, 0, 139),       // 31: Dark Blue
    QColor(255, 140, 0),     // 32: Dark Orange
    QColor(139, 90, 43),     // 33: Sienna
    QColor(47, 79, 79),      // 34: Dark Slate Gray
    QColor(204, 85, 0),      // 35: Chocolate
    QColor(112, 128, 144),   // 36: Slate Gray
    QColor(192, 192, 192),   // 37: Silver
    QColor(119, 136, 153),   // 38: Light Slate Gray
    QColor(176, 196, 222)    // 39: Light Steel Blue
};

PlotWindow::PlotWindow(QWidget *parent)
    : QWidget(parent)
    , m_nextCurveId(0)
    , m_windowIndex(0)
    , m_customPlot(nullptr)
    , m_openglEnabled(false)
    , m_xAutoRange(true)
    , m_yAutoRange(true)
    , m_xMin(0), m_xMax(10)
    , m_yMin(-1.5), m_yMax(1.5)
{
    setupPlot();
}

PlotWindow::~PlotWindow()
{
    // QCustomPlot 由 Qt 的对象树管理
}

void PlotWindow::setupPlot()
{
    // 创建 QCustomPlot 窗口
    m_customPlot = new QCustomPlot(this);

    // 设置背景
    m_customPlot->setBackground(QColor(255, 255, 255));
    m_customPlot->axisRect()->setBackground(QColor(240, 240, 240));

    // 配置坐标轴
    m_customPlot->xAxis->setLabel(QStringLiteral("时间 (s)"));
    m_customPlot->yAxis->setLabel(QStringLiteral("数值"));

    // 启用交互
    m_customPlot->setInteraction(QCP::iRangeDrag, true);
    m_customPlot->setInteraction(QCP::iRangeZoom, true);
    m_customPlot->setInteraction(QCP::iSelectPlottables, true);

    // 设置初始范围
    m_customPlot->xAxis->setRange(m_xMin, m_xMax);
    m_customPlot->yAxis->setRange(m_yMin, m_yMax);

    // 配置网格
    m_customPlot->xAxis->grid()->setVisible(true);
    m_customPlot->yAxis->grid()->setVisible(true);
    m_customPlot->xAxis->grid()->setZeroLinePen(QPen(Qt::gray, 1, Qt::DashLine));
    m_customPlot->yAxis->grid()->setZeroLinePen(QPen(Qt::gray, 1, Qt::DashLine));

    // 配置图例
    m_customPlot->legend->setVisible(true);
    m_customPlot->legend->setFont(QFont("Helvetica", 9));
    m_customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));

    // 设置布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_customPlot);
    setLayout(layout);

    // 默认大小
    setMinimumSize(350, 250);
    setStyleSheet("QWidget { border: 1px solid #ccc; }");
}

void PlotWindow::initialize(int windowIndex, int channelCount)
{
    m_windowIndex = windowIndex;
    removeAllCurves();

    // 添加指定数量的曲线
    for (int i = 0; i < channelCount && i < COLOR_COUNT; ++i) {
        addCurve(QStringLiteral("CH%1").arg(i + 1));
    }

    replot();
}

int PlotWindow::addCurve(const QString &name)
{
    if (m_customPlot->graphCount() >= COLOR_COUNT) {
        qWarning() << "PlotWindow: Maximum curve count reached!";
        return -1;
    }

    QCPGraph *graph = m_customPlot->addGraph();
    
    // 曲线ID就是图表中的索引
    int curveId = m_customPlot->graphCount() - 1;

    // 设置曲线名称和样式
    QString curveName = name.isEmpty() ? QStringLiteral("Curve %1").arg(curveId) : name;
    graph->setName(curveName);

    // 应用颜色和线宽
    applyCurveColor(graph, curveId % COLOR_COUNT);
    graph->setPen(QPen(graph->pen().color(), 1));  // 线宽设置为1
    graph->setLineStyle(QCPGraph::lsLine);

    // 设置抗锯齿
    graph->setAntialiased(true);

    m_curveMap[curveId] = graph;

    return curveId;
}

void PlotWindow::removeCurve(int curveId)
{
    if (m_curveMap.contains(curveId)) {
        m_customPlot->removeGraph(m_curveMap[curveId]);
        m_curveMap.remove(curveId);
        replot();
    }
}

void PlotWindow::removeAllCurves()
{
    m_customPlot->clearGraphs();
    m_curveMap.clear();
    m_nextCurveId = 0;
}

int PlotWindow::getCurveCount() const
{
    return m_curveMap.size();
}

QVector<int> PlotWindow::getCurveIds() const
{
    return m_curveMap.keys().toVector();
}

void PlotWindow::setCurveData(int curveId, const QVector<double> &keys, const QVector<double> &values)
{
    if (!m_curveMap.contains(curveId)) {
        qWarning() << "PlotWindow: Curve ID not found:" << curveId;
        return;
    }

    QCPGraph *graph = m_curveMap[curveId];
    graph->setData(keys, values, true); // true: 数据已排序
}

void PlotWindow::addCurveData(int curveId, double key, double value)
{
    if (!m_curveMap.contains(curveId)) {
        qWarning() << "PlotWindow: Curve ID not found:" << curveId;
        return;
    }

    QCPGraph *graph = m_curveMap[curveId];
    graph->addData(key, value);
}

void PlotWindow::clearCurveData(int curveId)
{
    if (m_curveMap.contains(curveId)) {
        m_curveMap[curveId]->data()->clear();
    }
}

void PlotWindow::setAxisRange(double xMin, double xMax, double yMin, double yMax)
{
    m_xMin = xMin;
    m_xMax = xMax;
    m_yMin = yMin;
    m_yMax = yMax;

    // 始终设置范围，不管自动范围设置如何
    m_customPlot->xAxis->setRange(xMin, xMax);
    m_customPlot->yAxis->setRange(yMin, yMax);
}

void PlotWindow::setAxisLabels(const QString &xLabel, const QString &yLabel)
{
    m_customPlot->xAxis->setLabel(xLabel);
    m_customPlot->yAxis->setLabel(yLabel);
}

void PlotWindow::setYAxisAutoRange(bool enabled)
{
    m_yAutoRange = enabled;
    if (enabled) {
        m_customPlot->yAxis->setRangeLower(m_yMin);
        m_customPlot->yAxis->setRangeUpper(m_yMax);
    } else {
        m_customPlot->yAxis->setRange(m_yMin, m_yMax);
    }
}

void PlotWindow::setXAxisAutoRange(bool enabled)
{
    m_xAutoRange = enabled;
    if (enabled) {
        m_customPlot->xAxis->setRangeLower(m_xMin);
        m_customPlot->xAxis->setRangeUpper(m_xMax);
    } else {
        m_customPlot->xAxis->setRange(m_xMin, m_xMax);
    }
}

void PlotWindow::enableOpenGL(bool enabled)
{
    m_openglEnabled = enabled;
#ifdef QCUSTOMPLOT_USE_OPENGL
    m_customPlot->setOpenGl(enabled);
    if (enabled) {
        qDebug() << "PlotWindow" << m_windowIndex << ": OpenGL acceleration enabled";
    }
#else
    if (enabled) {
        qWarning() << "PlotWindow" << m_windowIndex << ": OpenGL not compiled in";
    }
#endif
}

void PlotWindow::setInteractionMode(bool enableZoom, bool enablePan)
{
    m_customPlot->setInteraction(QCP::iRangeZoom, enableZoom);
    m_customPlot->setInteraction(QCP::iRangeDrag, enablePan);
}

void PlotWindow::setUpdateMode(int msRefreshRate)
{
    // 控制刷新频率的机制由外部定时器管理
    // 这里仅记录配置信息
    Q_UNUSED(msRefreshRate);
}

void PlotWindow::replot()
{
    if (m_customPlot) {
        m_customPlot->replot();
    }
}

void PlotWindow::applyCurveColor(QCPGraph *graph, int index)
{
    if (index < 0 || index >= COLOR_COUNT) {
        index = 0;
    }
    graph->setPen(QPen(CURVE_COLORS[index]));
}
