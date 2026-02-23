#include "DataCacheManager.h"
#include <QDateTime>
#include <QDebug>

DataCacheManager* DataCacheManager::m_instance = nullptr;

DataCacheManager::DataCacheManager(QObject *parent) : QObject(parent)
{
}

DataCacheManager* DataCacheManager::instance()
{
    if (!m_instance) {
        m_instance = new DataCacheManager;
    }
    return m_instance;
}

void DataCacheManager::addFrame(const FrameData& frame)
{
    QWriteLocker locker(&m_rwLock); // 写锁：独占访问

    // 添加新帧
    m_frameCache.append(frame);

    // 清理超出最大缓存的旧帧
    while (m_frameCache.size() > m_maxCacheSize) {
        m_frameCache.removeFirst();
    }

    // 清理过期数据
    cleanExpiredFrames();

    // 发送轻量更新通知
    emit dataUpdated();

    // 检测报警帧
    if (frame.isAlarm || frame.temperature > 80) {
        emit criticalFrameReceived(frame);
    }
}

FrameData DataCacheManager::getLatestFrame()
{
    QReadLocker locker(&m_rwLock); // 读锁：共享访问

    if (m_frameCache.isEmpty()) {
        FrameData emptyFrame;
        emptyFrame.timestamp = 0;
        emptyFrame.frameId = UINT16_MAX; // 无效帧ID
        return emptyFrame;
    }
    return m_frameCache.last();
}

QVector<FrameData> DataCacheManager::getLastNFrames(int n)
{
    QReadLocker locker(&m_rwLock);

    if (m_frameCache.isEmpty() || n <= 0) {
        return {};
    }

    int startIdx = qMax(0, m_frameCache.size() - n);
    return m_frameCache.mid(startIdx);
}

QVector<FrameData> DataCacheManager::getFramesInTimeRange(qint64 startTimeMs, qint64 endTimeMs)
{
    QReadLocker locker(&m_rwLock);
    QVector<FrameData> result;

    for (const auto& frame : m_frameCache) {
        if (frame.timestamp >= startTimeMs && frame.timestamp <= endTimeMs) {
            result.append(frame);
        }
    }
    return result;
}

QVector<FrameData> DataCacheManager::getAllFrames()
{
    QReadLocker locker(&m_rwLock);
    return m_frameCache;
}

int DataCacheManager::getCacheSize()
{
    QReadLocker locker(&m_rwLock);
    return m_frameCache.size();
}

void DataCacheManager::setMaxCacheSize(int maxSize)
{
    QWriteLocker locker(&m_rwLock);
    m_maxCacheSize = maxSize;
    while (m_frameCache.size() > m_maxCacheSize) {
        m_frameCache.removeFirst();
    }
}

void DataCacheManager::setExpireTimeMs(qint64 expireMs)
{
    QWriteLocker locker(&m_rwLock);
    m_expireTimeMs = expireMs;
}

void DataCacheManager::clearCache()
{
    QWriteLocker locker(&m_rwLock);
    m_frameCache.clear();
}

void DataCacheManager::cleanExpiredFrames()
{
    if (m_expireTimeMs <= 0) return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i < m_frameCache.size(); ) {
        if (now - m_frameCache[i].timestamp > m_expireTimeMs) {
            m_frameCache.remove(i);
        } else {
            i++;
        }
    }
}