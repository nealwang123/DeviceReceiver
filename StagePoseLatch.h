#ifndef STAGEPOSELATCH_H
#define STAGEPOSELATCH_H

#include <QMutex>
#include "FrameData.h"

/**
 * 线程安全的「最近台位」快照：Stage 线程 update，DUT 采集线程 applyToFrame 后写入缓存。
 *
 * 参数语义与 stage.proto AxisValue 一致：Mm 为毫米物理量；Pulse 为下位机位置计数（非 mm）。
 */
class StagePoseLatch
{
public:
    void update(double xMm, double yMm, double zMm, int xPulse, int yPulse, int zPulse, qint64 unixMs);
    void applyToFrame(FrameData& frame) const;
    void clear();

private:
    mutable QMutex m_mutex;
    bool m_valid = false;
    qint64 m_unixMs = 0;
    double m_xMm = 0.0;
    double m_yMm = 0.0;
    double m_zMm = 0.0;
    int m_xPulse = 0;
    int m_yPulse = 0;
    int m_zPulse = 0;
};

#endif // STAGEPOSELATCH_H
