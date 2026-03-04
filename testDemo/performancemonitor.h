#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QVector>

class PerformanceMonitor : public QObject
{
    Q_OBJECT

public:
    explicit PerformanceMonitor(QObject *parent = nullptr);
    
    // 记录一帧（用于计算FPS）
    void recordFrame();
    
    // 获取当前FPS
    double currentFps() const;
    
    // 获取CPU使用率（估算值）
    double cpuUsage() const;
    
    // 获取内存使用（MB）
    double memoryUsageMB() const;
    
    // 重置统计
    void reset();

private:
    // 计算CPU使用率（Windows系统）
    double calculateCpuUsage() const;
    
    // 计算内存使用（Windows系统）
    double calculateMemoryUsageMB() const;
    
    mutable QElapsedTimer m_fpsTimer;
    QVector<qint64> m_frameTimes;
    int m_frameCount;
    double m_cachedCpuUsage;
    double m_cachedMemoryUsage;
    
    static constexpr int FPS_WINDOW_SIZE = 60; // 统计最近60帧
    static constexpr double CPU_UPDATE_INTERVAL = 1.0; // CPU使用率更新间隔（秒）
};

#endif // PERFORMANCEMONITOR_H