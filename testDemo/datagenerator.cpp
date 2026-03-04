#include "datagenerator.h"
#include <QtMath>
#include <QDateTime>

DataGenerator::DataGenerator(QObject *parent)
    : QObject(parent)
    , m_random(static_cast<quint32>(QDateTime::currentMSecsSinceEpoch()))
    , m_amplitude(1.0)
    , m_frequency(1.0)  // 1Hz
    , m_noiseLevel(0.1)
    , m_isRunning(false)
    , m_startTime(0.0)
{
    m_timer.start();
}

DataGenerator::~DataGenerator()
{
    stop();
}

void DataGenerator::start()
{
    QMutexLocker locker(&m_mutex);
    m_isRunning = true;
    m_startTime = m_timer.elapsed() / 1000.0; // 转换为秒
}

void DataGenerator::stop()
{
    QMutexLocker locker(&m_mutex);
    m_isRunning = false;
}

QVector<QVector<double>> DataGenerator::generateData(int channelCount)
{
    QMutexLocker locker(&m_mutex);
    
    QVector<QVector<double>> allData;
    allData.reserve(channelCount);
    
    // 获取当前时间（在锁定的状态下）
    double currentTimeVal;
    if (!m_isRunning) {
        currentTimeVal = 0.0;
    } else {
        currentTimeVal = (m_timer.elapsed() / 1000.0) - m_startTime;
    }
    
    for (int i = 0; i < channelCount; ++i) {
        allData.append(generateChannelData(i, currentTimeVal));
    }
    
    return allData;
}

QVector<double> DataGenerator::generateChannelData(int channelIndex, double currentTime)
{
    QVector<double> data;
    data.reserve(HISTORY_POINTS);
    
    // 生成1000个历史数据点
    // 每个点代表100ms，所以1000个点 = 100秒
    for (int i = 0; i < HISTORY_POINTS; ++i) {
        // 计算时间点（最近的数据在最后）
        // 改为100ms间隔（0.1秒），而不是1ms
        double timeOffset = (i - HISTORY_POINTS + 1) * 0.1;  // 每个点间隔100ms
        double time = currentTime + timeOffset;
        
        // 生成基波（不同通道有不同相位偏移）
        double value = generateSineWave(channelIndex, time);
        
        // 添加随机噪声
        value = addNoise(value);
        
        data.append(value);
    }
    
    return data;
}

double DataGenerator::generateSineWave(int channelIndex, double time)
{
    // 每个通道有不同的频率倍数和相位偏移，使曲线看起来不同
    double channelFreq = m_frequency * (1.0 + channelIndex * 0.1);
    double channelPhase = channelIndex * M_PI / 6.0; // 30度相位差
    
    return m_amplitude * qSin(2.0 * M_PI * channelFreq * time + channelPhase);
}

double DataGenerator::addNoise(double value)
{
    // 添加随机噪声（-noiseLevel 到 +noiseLevel）
    double noise = m_random.generateDouble() * 2.0 * m_noiseLevel - m_noiseLevel;
    return value + noise;
}

double DataGenerator::currentTime() const
{
    QMutexLocker locker(&m_mutex);
    if (!m_isRunning) {
        return 0.0;
    }
    return (m_timer.elapsed() / 1000.0) - m_startTime;
}