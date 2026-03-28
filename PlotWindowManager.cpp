#include "PlotWindowManager.h"
#include "PlotWindow.h"
#include "HeatMapPlotWindow.h"
#include "ArrayPlotWindow.h"
#include "DataCacheManager.h"
#include "AppConfig.h"
#include "PlotDataHub.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QDebug>

// 静态成员初始化
PlotWindowManager* PlotWindowManager::m_instance = nullptr;

PlotWindowManager::PlotWindowManager(QObject *parent)
    : QObject(parent)
    , m_updateTimer(nullptr)
    , m_isInitialized(false)
{
    initialize();
}

PlotWindowManager::~PlotWindowManager()
{
    cleanup();
}

PlotWindowManager* PlotWindowManager::instance()
{
    if (!m_instance) {
        m_instance = new PlotWindowManager();
    }
    return m_instance;
}

void PlotWindowManager::destroy()
{
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

void PlotWindowManager::initialize()
{
    if (m_isInitialized) {
        return;
    }

    // 创建更新定时器
    m_updateTimer = new QTimer(this);
    int intervalMs = 50;
    if (AppConfig* config = AppConfig::instance()) {
        intervalMs = qBound(10, config->plotRefreshIntervalMs(), 1000);
    }
    m_baseUpdateIntervalMs = intervalMs;
    m_updateTimer->setInterval(intervalMs);
    connect(m_updateTimer, &QTimer::timeout, this, &PlotWindowManager::onUpdateTimer);
    if (AppConfig* config = AppConfig::instance()) {
        PlotDataHub::instance()->setMaxPoints(config->maxPlotPoints());
    }

    // 连接DataCacheManager的报警信号
    DataCacheManager* cacheManager = DataCacheManager::instance();
    if (cacheManager) {
        connect(cacheManager, &DataCacheManager::criticalFrameReceived,
                this, &PlotWindowManager::criticalFrameReceived);
    }

    m_isInitialized = true;
    qInfo() << "PlotWindowManager 初始化完成";
}

void PlotWindowManager::cleanup()
{
    stopUpdates();
    
    // 注意：这里不删除窗口，窗口由其父对象管理
    m_windows.clear();
    
    if (m_updateTimer) {
        m_updateTimer->deleteLater();
        m_updateTimer = nullptr;
    }

    m_lastDispatchedTimestamp = 0;
    PlotDataHub::instance()->reset();
    
    m_isInitialized = false;
}

PlotWindow* PlotWindowManager::createWindow(PlotType type, QWidget* parent)
{
    PlotWindow* window = nullptr;
    QString title;

    switch (type) {
    case CombinedPlot:
        window = new PlotWindow(parent);
        title = "组合监控";
        break;
    case TemperaturePlot:
        window = new PlotWindow(parent);
        title = "监控1";
        break;
    case HumidityPlot:
        window = new PlotWindow(parent);
        title = "监控2";
        break;
    case VoltagePlot:
        window = new PlotWindow(parent);
        title = "监控3";
        break;
    case HistoryPlot:
        window = new PlotWindow(parent);
        title = "历史数据4";
        break;
    case HeatmapPlot:
        window = new HeatMapPlotWindow(parent);
        title = "热力图5";
        break;
    case ArrayPlot:
        window = new ArrayPlotWindow(parent);
        title = "阵列图6";
        break;
    default:
        window = new PlotWindow(parent);
        title = "未知监控";
        break;
    }

    if (!window) {
        qCritical() << "创建PlotWindow失败";
        return nullptr;
    }

    window->setWindowTitle(title);
    registerWindow(window);
    return window;
}

PlotWindow* PlotWindowManager::createWindowInMdiArea(QMdiArea* mdiArea, PlotType type)
{
    qDebug() << "[PlotWindowManager] createWindowInMdiArea enter, type" << type << "mdiArea" << mdiArea;
    if (!mdiArea) {
        qCritical() << "MDI区域无效";
        return nullptr;
    }
    
    // 创建PlotWindow实例（父对象为null，将由QMdiSubWindow管理）
    PlotWindow* plotWindow = createWindow(type);
    qDebug() << "[PlotWindowManager] createWindow returned" << plotWindow;
    if (!plotWindow) {
        return nullptr;
    }
    
    // 创建MDI子窗口
    qDebug() << "[PlotWindowManager] adding subwindow for" << plotWindow;
    QMdiSubWindow* subWindow = mdiArea->addSubWindow(plotWindow);
    qDebug() << "[PlotWindowManager] subwindow result" << subWindow;
    if (!subWindow) {
        qCritical() << "创建MDI子窗口失败";
        delete plotWindow;
        return nullptr;
    }
    
    // 配置子窗口属性
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle(plotWindow->windowTitle());
    subWindow->resize(600, 400);
    subWindow->show();
    
    // 当子窗口关闭时，从管理器注销PlotWindow
    // 使用QPointer安全地跟踪PlotWindow
    QPointer<PlotWindow> plotWindowPtr(plotWindow);
    connect(subWindow, &QMdiSubWindow::destroyed, this, [this, plotWindowPtr]() {
        qDebug() << "[PlotWindowManager] subWindow destroyed, unregistering" << plotWindowPtr;
        if (plotWindowPtr) {
            unregisterWindow(plotWindowPtr);
        }
    });
    
    qInfo() << "在MDI区域创建窗口:" << plotWindow->windowTitle();
    qDebug() << "[PlotWindowManager] createWindowInMdiArea exit";
    
    return plotWindow;
}

void PlotWindowManager::registerWindow(PlotWindow* window)
{
    if (!window || m_windows.contains(window)) {
        qDebug() << "registerWindow skipped for" << window;
        return;
    }

    qDebug() << "registering window" << window << "title" << window->windowTitle();
    m_windows.append(window);
    
    // 连接数据更新信号
    connect(this, &PlotWindowManager::dataUpdated,
            window, &PlotWindow::onDataUpdated);
    connect(this, &PlotWindowManager::plotSnapshotUpdated,
            window, &PlotWindow::onPlotSnapshotUpdated);
    
    // 连接报警信号
    connect(this, &PlotWindowManager::criticalFrameReceived,
            window, &PlotWindow::onCriticalFrame);

    qInfo() << "注册PlotWindow:" << window->windowTitle() 
            << "，当前窗口数:" << m_windows.size();
    
    emit windowAdded(window);
}

void PlotWindowManager::unregisterWindow(PlotWindow* window)
{
    if (!window || !m_windows.contains(window)) {
        return;
    }

    // 断开信号连接
    disconnect(this, &PlotWindowManager::dataUpdated,
               window, &PlotWindow::onDataUpdated);
    disconnect(this, &PlotWindowManager::plotSnapshotUpdated,
               window, &PlotWindow::onPlotSnapshotUpdated);
    disconnect(this, &PlotWindowManager::criticalFrameReceived,
               window, &PlotWindow::onCriticalFrame);

    m_windows.removeOne(window);
    
    qInfo() << "注销PlotWindow:" << window->windowTitle()
            << "，剩余窗口数:" << m_windows.size();
    
    emit windowRemoved(window);
}

void PlotWindowManager::updateAllWindows()
{
    QVector<FrameData> frames = getRecentFrames(5);
    if (!frames.isEmpty()) {
        emit dataUpdated(frames);
        emit plotSnapshotUpdated(PlotDataHub::instance()->appendFrames(frames));
    }
}

void PlotWindowManager::setUpdateInterval(int intervalMs)
{
    if (intervalMs < 10) {
        intervalMs = 10; // 最小10ms（100Hz）
    } else if (intervalMs > 1000) {
        intervalMs = 1000; // 最大1000ms（1Hz）
    }
    
    if (m_updateTimer) {
        m_updateTimer->setInterval(intervalMs);
        qInfo() << "更新间隔设置为:" << intervalMs << "ms";
    }
}

void PlotWindowManager::startUpdates()
{
    if (m_updateTimer && !m_updateTimer->isActive()) {
        m_updateTimer->start();
        qInfo() << "PlotWindowManager 开始数据更新";
    }
}

void PlotWindowManager::stopUpdates()
{
    if (m_updateTimer && m_updateTimer->isActive()) {
        m_updateTimer->stop();
        qInfo() << "PlotWindowManager 停止数据更新";
    }
}

QVector<FrameData> PlotWindowManager::getRecentFrames(int count)
{
    DataCacheManager* cacheManager = DataCacheManager::instance();
    if (cacheManager) {
        return cacheManager->getLastNFrames(count);
    }
    
    return QVector<FrameData>();
}

void PlotWindowManager::onUpdateTimer()
{
    // 自适应拉取：窗口越多，单次拉取越少，降低UI线程突发负载
    int fetchCount = 5;
    if (m_windows.size() > 8) {
        fetchCount = 2;
    } else if (m_windows.size() > 4) {
        fetchCount = 3;
    }

    QVector<FrameData> frames = getRecentFrames(fetchCount);
    if (frames.isEmpty()) {
        return;
    }

    // 仅分发“新增帧”，避免重复绘制最近窗口数据
    QVector<FrameData> incrementalFrames;
    incrementalFrames.reserve(frames.size());
    qint64 maxTimestamp = m_lastDispatchedTimestamp;

    for (const FrameData& frame : frames) {
        if (frame.timestamp > m_lastDispatchedTimestamp) {
            incrementalFrames.append(frame);
            if (frame.timestamp > maxTimestamp) {
                maxTimestamp = frame.timestamp;
            }
        }
    }

    if (incrementalFrames.isEmpty()) {
        return;
    }

    m_lastDispatchedTimestamp = maxTimestamp;

    // 分发共享快照给所有注册窗口（保留旧信号用于兼容占位窗口）
    emit dataUpdated(incrementalFrames);
    emit plotSnapshotUpdated(PlotDataHub::instance()->appendFrames(incrementalFrames));
    
    // 性能监控：如果窗口过多，可以适当降低更新频率
    if (m_windows.size() > 5) {
        // 每5个窗口增加10ms延迟
        int recommendedInterval = m_baseUpdateIntervalMs + (m_windows.size() / 5) * 10;
        if (m_updateTimer->interval() != recommendedInterval) {
            setUpdateInterval(recommendedInterval);
        }
    } else if (m_updateTimer->interval() != m_baseUpdateIntervalMs) {
        setUpdateInterval(m_baseUpdateIntervalMs);
    }
}