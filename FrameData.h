#ifndef FRAMEDATA_H
#define FRAMEDATA_H

#include <cstdint>
#include <QMetaType>

// 实时数据帧结构体（与硬件协议对应）
struct FrameData
{
    int64_t timestamp;     // 时间戳（毫秒）
    uint16_t frameId;      // 帧ID（0-65535）
    float temperature;     // 温度（℃）
    float humidity;        // 湿度（%）
    double voltage;        // 电压（V）
    bool isAlarm;          // 报警标志

    FrameData() :
        timestamp(0),
        frameId(0),
        temperature(0.0f),
        humidity(0.0f),
        voltage(0.0),
        isAlarm(false)
    {
    }
};

Q_DECLARE_METATYPE(FrameData)

#endif // FRAMEDATA_H
