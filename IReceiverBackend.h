#ifndef IRECEIVERBACKEND_H
#define IRECEIVERBACKEND_H

#include <QObject>
#include <QByteArray>
#include "FrameData.h"

class IReceiverBackend : public QObject
{
    Q_OBJECT
public:
    explicit IReceiverBackend(QObject* parent = nullptr) : QObject(parent) {}
    ~IReceiverBackend() override = default;

public slots:
    virtual bool connectBackend(const QString& endpoint) = 0;
    virtual void disconnectBackend() = 0;
    virtual bool isBackendConnected() const = 0;
    virtual void startAcquisition(int intervalMs = 100) = 0;
    virtual void stopAcquisition() = 0;
    virtual void setPaused(bool paused) = 0;
    virtual void sendCommand(const QByteArray& command) = 0;
    virtual void sendCommand(const QString& command, bool isHex = false) = 0;

signals:
    void frameReceived(const FrameData& frame);
    void commandSent(const QByteArray& command);
    void commandError(const QString& error);
    void dataReceived(const QByteArray& data, bool isHex = false);
    /// 后端连接状态变化（true = 已连接，false = 已断开）
    void connectionStateChanged(bool connected);
};

#endif // IRECEIVERBACKEND_H
