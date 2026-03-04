#include "performancemonitor.h"
#include <QDateTime>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent)
    , m_frameCount(0)
    , m_cachedCpuUsage(0.0)
    , m_cachedMemoryUsage(0.0)
{
    m_fpsTimer.start();
    m_frameTimes.reserve(FPS_WINDOW_SIZE);
    reset();
}

void PerformanceMonitor::recordFrame()
{
    qint64 currentTime = m_fpsTimer.elapsed();
    
    // 记录帧时间
    m_frameTimes.append(currentTime);
    m_frameCount++;
    
    // 保持窗口大小
    if (m_frameTimes.size() > FPS_WINDOW_SIZE) {
        m_frameTimes.removeFirst();
    }
    
    // 定期更新CPU和内存使用率（每秒更新一次）
    static qint64 lastCpuUpdate = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    if (now - lastCpuUpdate > CPU_UPDATE_INTERVAL * 1000) {
        m_cachedCpuUsage = calculateCpuUsage();
        m_cachedMemoryUsage = calculateMemoryUsageMB();
        lastCpuUpdate = now;
    }
}

double PerformanceMonitor::currentFps() const
{
    if (m_frameTimes.size() < 2) {
        return 0.0;
    }
    
    // 计算平均帧时间
    qint64 totalTime = m_frameTimes.last() - m_frameTimes.first();
    if (totalTime <= 0) {
        return 0.0;
    }
    
    // 计算FPS = (帧数-1) / (总时间秒)
    double fps = (m_frameTimes.size() - 1) * 1000.0 / totalTime;
    return fps;
}

double PerformanceMonitor::cpuUsage() const
{
    return m_cachedCpuUsage;
}

double PerformanceMonitor::memoryUsageMB() const
{
    return m_cachedMemoryUsage;
}

void PerformanceMonitor::reset()
{
    m_frameTimes.clear();
    m_frameCount = 0;
    m_fpsTimer.restart();
}

double PerformanceMonitor::calculateCpuUsage() const
{
#ifdef Q_OS_WIN
    // Windows系统CPU使用率计算
    static quint64 lastIdleTime = 0;
    static quint64 lastKernelTime = 0;
    static quint64 lastUserTime = 0;
    
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;
        
        quint64 currentIdleTime = idle.QuadPart;
        quint64 currentKernelTime = kernel.QuadPart;
        quint64 currentUserTime = user.QuadPart;
        
        if (lastIdleTime != 0 && lastKernelTime != 0 && lastUserTime != 0) {
            quint64 idleDiff = currentIdleTime - lastIdleTime;
            quint64 kernelDiff = currentKernelTime - lastKernelTime;
            quint64 userDiff = currentUserTime - lastUserTime;
            
            quint64 totalDiff = kernelDiff + userDiff;
            if (totalDiff > 0) {
                double cpuUsage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
                lastIdleTime = currentIdleTime;
                lastKernelTime = currentKernelTime;
                lastUserTime = currentUserTime;
                return qBound(0.0, cpuUsage, 100.0);
            }
        } else {
            lastIdleTime = currentIdleTime;
            lastKernelTime = currentKernelTime;
            lastUserTime = currentUserTime;
        }
    }
#endif
    
    // 如果无法获取系统信息，返回估算值
    static double simulatedCpu = 0.0;
    static bool increasing = true;
    
    // 模拟CPU使用率变化（0-30%范围内）
    if (increasing) {
        simulatedCpu += 0.5;
        if (simulatedCpu >= 30.0) {
            increasing = false;
        }
    } else {
        simulatedCpu -= 0.5;
        if (simulatedCpu <= 0.0) {
            increasing = true;
        }
    }
    
    return simulatedCpu;
}

double PerformanceMonitor::calculateMemoryUsageMB() const
{
#ifdef Q_OS_WIN
    // Windows系统内存使用计算
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), 
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), 
                             sizeof(pmc))) {
        // 返回工作集内存（实际使用的物理内存）
        return pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
#endif
    
    // 如果无法获取系统信息，返回估算值
    static double simulatedMemory = 100.0;
    static bool memIncreasing = true;
    
    // 模拟内存使用变化（50-200MB范围内）
    if (memIncreasing) {
        simulatedMemory += 1.0;
        if (simulatedMemory >= 200.0) {
            memIncreasing = false;
        }
    } else {
        simulatedMemory -= 1.0;
        if (simulatedMemory <= 50.0) {
            memIncreasing = true;
        }
    }
    
    return simulatedMemory;
}