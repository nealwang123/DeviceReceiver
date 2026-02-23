#include "DataProcessor.h"
#include "DataCacheManager.h"
#include <QDateTime>
#include <QDebug>
#include <algorithm>
DataProcessor::DataProcessor(QObject *parent) : QObject(parent)
{
    // 1Hz统计定时器
    m_statTimer = new QTimer(this);
    m_statTimer->setInterval(1000);
    connect(m_statTimer, &QTimer::timeout, this, &DataProcessor::calcStats);
    m_statTimer->start();
}

void DataProcessor::calcStats()
{
    // 获取过去1秒内的所有帧
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto frames = DataCacheManager::instance()->getFramesInTimeRange(now - 1000, now);
    
    if (frames.isEmpty()) {
        qInfo() << "【统计】过去1秒无数据";
        return;
    }

    // 计算统计值
    int frameCount = frames.size();
    double tempSum = 0, tempMax = 0, tempMin = 100;
    double humiSum = 0, humiMax = 0, humiMin = 100;

    for (const auto& frame : frames) {
        tempSum += frame.temperature;
        tempMax = std::max(tempMax, static_cast<double>(frame.temperature));
        tempMin = std::min(tempMin, static_cast<double>(frame.temperature));

        humiSum += frame.humidity;
        humiMax = std::max(humiMax, static_cast<double>(frame.humidity));
        humiMin = std::min(humiMin, static_cast<double>(frame.humidity));
    }

    // 输出统计结果
    qInfo() << QString("【1秒统计】帧数：%1 | 温度：平均%2(最大%3/最小%4) | 湿度：平均%5(最大%6/最小%7)")
                .arg(frameCount)
                .arg(tempSum/frameCount, 0, 'f', 1)
                .arg(tempMax, 0, 'f', 1)
                .arg(tempMin, 0, 'f', 1)
                .arg(humiSum/frameCount, 0, 'f', 1)
                .arg(humiMax, 0, 'f', 1)
                .arg(humiMin, 0, 'f', 1);
}
