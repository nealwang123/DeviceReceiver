#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QObject>
#include <QSize>

/**
 * @brief 应用配置管理类，集中管理所有配置参数
 * 
 * 重构目标：将散落在各处的硬编码配置参数集中管理，支持从配置文件加载
 */
class AppConfig : public QObject
{
    Q_OBJECT
public:
    // 单例模式访问
    static AppConfig* instance();
    
    // 禁止拷贝和赋值
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
    // ========== 数据缓存配置 ==========
    int maxCacheSize() const { return m_maxCacheSize; }
    void setMaxCacheSize(int size) { m_maxCacheSize = size; }
    
    qint64 expireTimeMs() const { return m_expireTimeMs; }
    void setExpireTimeMs(qint64 ms) { m_expireTimeMs = ms; }
    
    // ========== 串口配置 ==========
    QString serialPort() const { return m_serialPort; }
    void setSerialPort(const QString& port) { m_serialPort = port; }
    
    int baudRate() const { return m_baudRate; }
    void setBaudRate(int rate) { m_baudRate = rate; }
    
    bool useMockData() const { return m_useMockData; }
    void setUseMockData(bool use) { m_useMockData = use; }
    
    int mockDataIntervalMs() const { return m_mockDataIntervalMs; }
    void setMockDataIntervalMs(int ms) { m_mockDataIntervalMs = ms; }
    
    // ========== 绘图配置 ==========
    int maxPlotPoints() const { return m_maxPlotPoints; }
    void setMaxPlotPoints(int points) { m_maxPlotPoints = points; }
    
    int plotRefreshIntervalMs() const { return m_plotRefreshIntervalMs; }
    void setPlotRefreshIntervalMs(int ms) { m_plotRefreshIntervalMs = ms; }
    
    // ========== 数据统计配置 ==========
    int statsIntervalMs() const { return m_statsIntervalMs; }
    void setStatsIntervalMs(int ms) { m_statsIntervalMs = ms; }
    
    // ========== 报警配置 ==========
    float temperatureAlarmThreshold() const { return m_temperatureAlarmThreshold; }
    void setTemperatureAlarmThreshold(float threshold) { m_temperatureAlarmThreshold = threshold; }
    
    // ========== 应用配置 ==========
    QString appTitle() const { return m_appTitle; }
    void setAppTitle(const QString& title) { m_appTitle = title; }
    
    QSize windowSize() const { return m_windowSize; }
    void setWindowSize(const QSize& size) { m_windowSize = size; }
    
    // ========== 文件操作 ==========
    bool loadFromFile(const QString& filename);
    bool saveToFile(const QString& filename);
    
    // 加载默认配置
    void loadDefaults();
    
private:
    explicit AppConfig(QObject *parent = nullptr);
    ~AppConfig() override = default;
    
    static AppConfig* m_instance;
    
    // 数据缓存配置
    int m_maxCacheSize = 600;          // 最大缓存帧数
    qint64 m_expireTimeMs = 60000;     // 数据过期时间（毫秒）
    
    // 串口配置
    QString m_serialPort = "COM3";     // 串口端口
    int m_baudRate = 115200;           // 波特率
    bool m_useMockData = true;         // 是否使用模拟数据
    int m_mockDataIntervalMs = 100;    // 模拟数据间隔（毫秒）
    
    // 绘图配置
    int m_maxPlotPoints = 200;         // 最大绘图点数
    int m_plotRefreshIntervalMs = 50;  // 绘图刷新间隔（毫秒）
    
    // 数据统计配置
    int m_statsIntervalMs = 1000;      // 统计间隔（毫秒）
    
    // 报警配置
    float m_temperatureAlarmThreshold = 80.0f; // 温度报警阈值（℃）
    
    // 应用配置
    QString m_appTitle = "实时数据监控";
    QSize m_windowSize = QSize(800, 600);
};

#endif // APPCONFIG_H