#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>
#include <QVector>
#include <QElapsedTimer>
#include <QMutex>
#include <QRandomGenerator>

class DataGenerator : public QObject
{
    Q_OBJECT

public:
    explicit DataGenerator(QObject *parent = nullptr);
    ~DataGenerator();

    // 生成指定通道数的随机数据
    QVector<QVector<double>> generateData(int channelCount);
    
    // 启动/停止数据生成（预留接口，支持后台线程生成）
    void start();
    void stop();
    
    // 设置数据生成参数
    void setAmplitude(double amplitude) { m_amplitude = amplitude; }
    void setFrequency(double frequency) { m_frequency = frequency; }
    void setNoiseLevel(double noiseLevel) { m_noiseLevel = noiseLevel; }
    
    // 获取当前时间戳（用于x轴）
    double currentTime() const;

private:
    // 生成单个通道的数据
    QVector<double> generateChannelData(int channelIndex, double currentTime);
    
    // 生成正弦波数据
    double generateSineWave(int channelIndex, double time);
    
    // 添加随机噪声
    double addNoise(double value);
    
    mutable QMutex m_mutex;
    QRandomGenerator m_random;
    QElapsedTimer m_timer;
    
    // 生成参数
    double m_amplitude;
    double m_frequency;
    double m_noiseLevel;
    
    // 状态
    bool m_isRunning;
    double m_startTime;
    
    static constexpr int HISTORY_POINTS = 1000;  // 每条曲线最多1000个历史点
};

#endif // DATAGENERATOR_H