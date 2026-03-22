#pragma once

#include <QString>

/**
 * gRPC 目标地址解析（IPv4 / 主机名 / IPv6）。
 *
 * 约定：
 *   - IPv4 或域名：  host:port  （仅当 host 不含冒号时）
 *   - IPv6：         [addr]:port  （RFC 5952，与 gRPC C++ CreateChannel 一致）
 *
 * 输出：
 *   - grpcTarget：用于 grpc::CreateChannel 的规范字符串（IPv6 带方括号）
 *   - outHost：    主机部分，IPv6 为无括号地址（便于 proto 等仅填 ip 字段）
 *   - outPort：    端口号
 */
namespace GrpcEndpointUtils {

inline bool parseHostPort(const QString& text, QString* grpcTarget, QString* outHost, int* outPort)
{
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        return false;
    }

    // ---- [IPv6]:port ----
    if (t.startsWith(QLatin1Char('['))) {
        const int closeBracket = t.indexOf(QLatin1Char(']'));
        if (closeBracket < 1) {
            return false;
        }
        const QString inner = t.mid(1, closeBracket - 1);
        if (t.size() <= closeBracket + 1 || t.at(closeBracket + 1) != QLatin1Char(':')) {
            return false;
        }
        const QString portStr = t.mid(closeBracket + 2).trimmed();
        bool ok = false;
        const int p = portStr.toInt(&ok);
        if (!ok || p <= 0 || p > 65535) {
            return false;
        }
        const QString canon = QStringLiteral("[%1]:%2").arg(inner).arg(p);
        if (grpcTarget) {
            *grpcTarget = canon;
        }
        if (outHost) {
            *outHost = inner;
        }
        if (outPort) {
            *outPort = p;
        }
        return true;
    }

    // ---- host:port（IPv4 或域名；host 中不得含冒号，否则须使用 [IPv6]:port）----
    const int lastColon = t.lastIndexOf(QLatin1Char(':'));
    if (lastColon <= 0) {
        return false;
    }
    const QString hostPart = t.left(lastColon).trimmed();
    const QString portStr = t.mid(lastColon + 1).trimmed();
    if (hostPart.isEmpty() || hostPart.contains(QLatin1Char(':'))) {
        return false;
    }
    bool ok = false;
    const int p = portStr.toInt(&ok);
    if (!ok || p <= 0 || p > 65535) {
        return false;
    }
    const QString canon = QStringLiteral("%1:%2").arg(hostPart).arg(p);
    if (grpcTarget) {
        *grpcTarget = canon;
    }
    if (outHost) {
        *outHost = hostPart;
    }
    if (outPort) {
        *outPort = p;
    }
    return true;
}

} // namespace GrpcEndpointUtils
