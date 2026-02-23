#ifndef SERIALRECEIVER_H
#define SERIALRECEIVER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include "FrameData.h"

// 串口数据接收线程类（独立线程运行，避免阻塞UI）
class SerialReceiver : public QObject
{
    Q_OBJECT
public:
    explicit SerialReceiver(QObject *parent = nullptr);
    ~SerialReceiver();

    // 串口操作接口
    bool openSerial(const QString& portName, int baudRate = 115200);
    void closeSerial();
    bool isSerialOpen() const;

    // 【测试用】开启模拟数据（无硬件时使用）
    Q_INVOKABLE void startMockData(int intervalMs = 100);

private slots:
    void onSerialReadyRead();    // 串口数据接收槽函数
    void onMockDataTimer();      // 模拟数据定时器槽函数

private:
    void processSerialBuffer();  // 处理串口粘包/拆包
    FrameData parseRawData(const QByteArray& rawFrame); // 解析原始数据为结构体

private:
    QSerialPort* m_serialPort;
    QByteArray m_serialBuffer;   // 串口缓存（处理粘包）
    QTimer* m_mockTimer;         // 模拟数据定时器

    // 帧协议配置（根据硬件修改）
    const int FRAME_LENGTH = 16;                     // 固定帧长度
    const QByteArray FRAME_HEAD = QByteArray::fromHex("AA55"); // 帧头
};

#endif // SERIALRECEIVER_H