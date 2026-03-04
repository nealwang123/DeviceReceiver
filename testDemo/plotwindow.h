#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QPen>

class QCustomPlot;
class QCPGraph;

/**
 * @brief PlotWindow - 单窗口多通道曲线显示容器
 * 
 * 功能：
 * - 支持动态添加/删除曲线
 * - 支持实时数据更新
 * - 支持曲线样式配置（颜色、线宽等）
 * - 支持坐标轴自适应
 * - 支持OpenGL加速模式
 */
class PlotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWindow(QWidget *parent = nullptr);
    ~PlotWindow();

    // 初始化绘图窗口
    void initialize(int windowIndex, int channelCount = 5);

    // 曲线管理
    int addCurve(const QString &name = "");
    void removeCurve(int curveId);
    void removeAllCurves();
    int getCurveCount() const;
    QVector<int> getCurveIds() const;  // 获取所有曲线ID

    // 数据更新
    void setCurveData(int curveId, const QVector<double> &keys, const QVector<double> &values);
    void addCurveData(int curveId, double key, double value);
    void clearCurveData(int curveId);

    // 坐标轴配置
    void setAxisRange(double xMin, double xMax, double yMin, double yMax);
    void setAxisLabels(const QString &xLabel, const QString &yLabel);
    void setYAxisAutoRange(bool enabled);
    void setXAxisAutoRange(bool enabled);

    // 性能和样式设置
    void enableOpenGL(bool enabled);
    void setInteractionMode(bool enableZoom, bool enablePan);
    void setUpdateMode(int msRefreshRate);

    // 绘图更新
    void replot();

    // 获取当前状态
    QCustomPlot* getPlotWidget() const { return m_customPlot; }
    int getWindowIndex() const { return m_windowIndex; }

private:
    // 初始化UI
    void setupPlot();

    // 应用预定义配色方案
    void applyCurveColor(QCPGraph *graph, int index);

    // 曲线ID映射
    QMap<int, QCPGraph*> m_curveMap;
    int m_nextCurveId;
    int m_windowIndex;

    // UI组件
    QCustomPlot* m_customPlot;

    // 配置参数
    bool m_openglEnabled;
    bool m_xAutoRange;
    bool m_yAutoRange;
    double m_xMin, m_xMax, m_yMin, m_yMax;

    // 预定义颜色方案（最多40条曲线）
    static constexpr int COLOR_COUNT = 40;
    static const QColor CURVE_COLORS[COLOR_COUNT];
};

#endif // PLOTWINDOW_H
