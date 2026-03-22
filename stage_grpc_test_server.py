#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
stage_grpc_test_server.py — 三轴台 StageService gRPC 桩（Python）
================================================================
与 proto/stage.proto 一致，供主窗口「三轴台测试装置」面板及 StageReceiverBackend 联调。

功能概要
  - 实现 Connect / GetPositions / PositionStream / Jog / MoveAbs/Rel / SetSpeed / 扫描等 RPC
  - 用于无真实工装时的功能验证；默认 0.0.0.0:50052（可与被测设备 50051 并存）

运行
  python stage_grpc_test_server.py [--port 50052] [--host 0.0.0.0] [--external]
  或 Windows：run_stage_grpc_test_server.bat（勿用 python 运行 .bat）
  无公网 IP 需穿透：scripts/start_stage_with_ngrok.bat（见 scripts/README_stage_tunnel.md）

依赖
  pip install grpcio protobuf；桩：proto/generated_py/stage_pb2*.py（改 proto 后需 protoc）

相关文件
  - 被测设备数据流：grpc_test_server.py（DeviceDataService）
  - 打独立 exe：package_stage_grpc_test_server.bat

命名区分（勿混淆）
  - grpc_test_server.py  → 被测设备 DeviceDataService（Subscribe/SendCommand）
  - stage_grpc_test_server.py → 三轴台 StageService（Connect / PositionStream / Jog / …）

默认监听 0.0.0.0:50052；控制端 Receiver/StageGrpcEndpoint 或界面地址需同端口（IPv6 用 [::1]:50052）。

绑定失败（Failed to bind）常见原因：端口已被占用。可换 --port 50053，或 netstat 查占用（Windows）。
"""

from __future__ import annotations

import argparse
import os
import signal
import socket
import sys
import threading
import time
from concurrent import futures

# ── 将 proto/generated_py 加入搜索路径（与 grpc_test_server.py 一致）────────
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RUNTIME_BASE_DIR = getattr(sys, "_MEIPASS", SCRIPT_DIR)
for _candidate in [
    os.path.join(RUNTIME_BASE_DIR, "proto", "generated_py"),
    os.path.join(SCRIPT_DIR, "proto", "generated_py"),
    SCRIPT_DIR,
]:
    if os.path.isfile(os.path.join(_candidate, "stage_pb2.py")):
        if _candidate not in sys.path:
            sys.path.insert(0, _candidate)
        break

import grpc
import stage_pb2
import stage_pb2_grpc


def _unix_ms_now() -> int:
    return int(time.time() * 1000)


def _mm_to_pulse(mm: float) -> int:
    return int(round(mm * 1000.0))


def _fill_positions(x_mm: float, y_mm: float, z_mm: float) -> stage_pb2.PositionsReply:
    r = stage_pb2.PositionsReply()
    r.x.pulse = _mm_to_pulse(x_mm)
    r.x.mm = x_mm
    r.y.pulse = _mm_to_pulse(y_mm)
    r.y.mm = y_mm
    r.z.pulse = _mm_to_pulse(z_mm)
    r.z.mm = z_mm
    r.unixMs = _unix_ms_now()
    return r


class StageTestServicer(stage_pb2_grpc.StageServiceServicer):
    """模拟三轴台：位置 + Jog + 扫描状态，供控制端全功能验证。"""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._x = 0.0
        self._y = 0.0
        self._z = 1.0
        self._jog_vx = 0.0
        self._jog_vy = 0.0
        self._jog_vz = 0.0
        self._speed_pulse_per_sec = 20000
        self._accel_ms = 100
        self._scan_running = False
        self._scan_detail = ""

    def _jog_scale(self) -> float:
        if self._speed_pulse_per_sec < 1:
            return 1.0
        ref = 20000.0
        return max(0.1, min(3.0, self._speed_pulse_per_sec / ref))

    def Connect(self, request, context):
        msg = f"stage-grpc-test-server: accepted stage TCP target {request.ip}:{request.port}"
        print(f"[Connect] {msg}")
        return stage_pb2.Result(ok=True, message=msg)

    def Disconnect(self, request, context):
        with self._lock:
            self._jog_vx = self._jog_vy = self._jog_vz = 0.0
        print("[Disconnect]")
        return stage_pb2.Result(ok=True, message="stage-grpc-test-server: disconnected (simulated)")

    def GetPositions(self, request, context):
        with self._lock:
            return _fill_positions(self._x, self._y, self._z)

    def PositionStream(self, request, context):
        interval_ms = max(10, request.intervalMs)
        print(f"[PositionStream] interval_ms={interval_ms}")
        try:
            while context.is_active():
                t0 = time.monotonic()
                with self._lock:
                    dt = interval_ms / 1000.0
                    self._x += self._jog_vx * dt
                    self._y += self._jog_vy * dt
                    self._z += self._jog_vz * dt
                    reply = _fill_positions(self._x, self._y, self._z)
                yield reply
                elapsed = time.monotonic() - t0
                sleep_s = max(0.0, interval_ms / 1000.0 - elapsed)
                if sleep_s > 0:
                    time.sleep(sleep_s)
        finally:
            print("[PositionStream] ended")

    def Jog(self, request, context):
        base = 2.0 * self._jog_scale()
        with self._lock:
            if request.axis == stage_pb2.Axis.Y:
                vp = "_jog_vy"
            elif request.axis == stage_pb2.Axis.Z:
                vp = "_jog_vz"
            else:
                vp = "_jog_vx"
            if not request.enable:
                setattr(self, vp, 0.0)
                print(f"[Jog] axis={request.axis} off")
                return stage_pb2.Result(ok=True, message="jog off")
            v = base if request.plus else -base
            setattr(self, vp, v)
            print(f"[Jog] axis={request.axis} plus={request.plus} on")
            return stage_pb2.Result(ok=True, message="jog on")

    def MoveAbs(self, request, context):
        with self._lock:
            self._x = request.xMm
            self._y = request.yMm
            self._z = request.zMm
            msg = (
                f"move_abs ok -> ({self._x}, {self._y}, {self._z}) mm, "
                f"timeoutMs={request.timeoutMs}"
            )
        print(f"[MoveAbs] {msg}")
        return stage_pb2.Result(ok=True, message=msg)

    def MoveRel(self, request, context):
        d = request.deltaMm
        with self._lock:
            if request.axis == stage_pb2.Axis.Y:
                self._y += d
            elif request.axis == stage_pb2.Axis.Z:
                self._z += d
            else:
                self._x += d
            msg = f"move_rel ok axis={request.axis} delta={d} mm"
        print(f"[MoveRel] {msg}")
        return stage_pb2.Result(ok=True, message=msg)

    def SetSpeed(self, request, context):
        with self._lock:
            self._speed_pulse_per_sec = request.speedPulsePerSec
            self._accel_ms = request.accelMs
            msg = f"set_speed ok pulse/s={self._speed_pulse_per_sec} accel_ms={self._accel_ms}"
        print(f"[SetSpeed] {msg}")
        return stage_pb2.Result(ok=True, message=msg)

    def StartScan(self, request, context):
        mode_name = stage_pb2.ScanMode.Name(request.mode)
        detail = (
            f"mode={mode_name} xs={request.xs} xe={request.xe} ys={request.ys} ye={request.ye} "
            f"yStep={request.yStep} zFix={request.zFix}"
        )
        with self._lock:
            self._scan_running = True
            self._scan_detail = detail
        print(f"[StartScan] {detail}")
        return stage_pb2.Result(ok=True, message="scan started (simulated): " + detail)

    def StopScan(self, request, context):
        with self._lock:
            self._scan_running = False
        print("[StopScan]")
        return stage_pb2.Result(ok=True, message="scan stopped (simulated)")

    def GetScanStatus(self, request, context):
        with self._lock:
            running = self._scan_running
            detail = self._scan_detail
        if running:
            status = "running: " + detail
        else:
            status = "idle" if not detail else f"idle; last={detail}"
        return stage_pb2.ScanStatusReply(running=running, status=status)


def _guess_primary_ipv4() -> str | None:
    """Best-effort：通过 UDP「假连接」得到本机用于出网的典型 IPv4（多网卡时仅供参考）。"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            s.settimeout(0.5)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            return str(ip) if ip else None
        finally:
            s.close()
    except OSError:
        return None


def _hostname_ipv4_candidates() -> list[str]:
    """getaddrinfo(主机名) 得到的非 127 IPv4，去重。"""
    seen: set[str] = set()
    out: list[str] = []
    try:
        hn = socket.gethostname()
        for info in socket.getaddrinfo(hn, None, socket.AF_INET, socket.SOCK_STREAM):
            ip = info[4][0]
            if not ip or ip.startswith("127."):
                continue
            if ip not in seen:
                seen.add(ip)
                out.append(ip)
    except OSError:
        pass
    return out


def _print_external_hints(port: int, listen_host: str) -> None:
    """--external 时附加说明：局域网、无公网 IP、内网穿透脚本。"""
    primary = _guess_primary_ipv4()
    extra = _hostname_ipv4_candidates()
    print("  ─────────────────────────────────────────────")
    print("  [外网/局域网提示] --external")
    print(f"  当前监听: {listen_host}:{port}")
    if primary:
        print(f"  本机典型 IPv4（出网）: {primary}  → 同网段设备可试 {primary}:{port}")
    if extra:
        print("  本机其它 IPv4: " + ", ".join(extra))
    if not primary and not extra:
        print("  （未能枚举本机 IPv4，请用 ipconfig 查看）")
    print("  无公网 IPv4（运营商 CGNAT）时，端口转发无效，请使用 scripts/start_stage_with_ngrok.*")
    print("  Windows 防火墙若拦截入站，请放行本端口的 TCP。")
    print("  gRPC 为明文 insecure，勿长期暴露不可信公网；生产请 TLS 或 VPN。")
    print("  ─────────────────────────────────────────────")


def _print_bind_failure_help(port: int, host: str, err: BaseException) -> None:
    print("\n[错误] 无法在以下地址绑定 gRPC 监听:", file=sys.stderr)
    print(f"       {host}:{port}", file=sys.stderr)
    print(f"       ({err!r})", file=sys.stderr)
    print(
        "\n常见原因与处理：\n"
        "  1) 端口已被占用 — 关闭其它 stage_grpc_test_server 实例，或在任务管理器结束残留 python.exe；\n"
        "     Windows 查看占用:  cmd 中执行  netstat -ano | findstr :" + str(port) + "\n"
        "  2) 换端口启动 —  python stage_grpc_test_server.py --port 50053\n"
        "  3) 仅本机回环 —  python stage_grpc_test_server.py --host 127.0.0.1 --port " + str(port) + "\n",
        file=sys.stderr,
    )


def serve(port: int, host: str = "0.0.0.0", *, external_mode: bool = False) -> None:
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    stage_pb2_grpc.add_StageServiceServicer_to_server(StageTestServicer(), server)
    listen_addr = f"{host}:{port}"
    try:
        bound = server.add_insecure_port(listen_addr)
        if bound == 0:
            raise RuntimeError("add_insecure_port returned 0 (bind failed)")
    except RuntimeError as e:
        _print_bind_failure_help(port, host, e)
        raise
    server.start()

    print("=" * 60)
    print("  StageService gRPC 测试服务器（三轴台工装模拟）")
    print(f"  监听地址: {listen_addr}")
    if external_mode:
        _print_external_hints(port, host)
    print("  ─────────────────────────────────────────────")
    print("  被测设备数据流测试请使用: grpc_test_server.py（默认端口 50051）")
    print("  本脚本对应: stage.proto / 三轴台面板 StageReceiverBackend")
    print("  ─────────────────────────────────────────────")
    print("  退出: 按 Ctrl+C（或向进程发送结束信号）。")
    print("  若仍无法退出: 直接关闭本窗口, 或任务管理器结束对应 python.exe。")
    print("=" * 60)

    shutdown = threading.Event()

    def on_signal(sig, frame):
        print("\n[信号] 正在关闭 Stage 测试服务器...", flush=True)
        shutdown.set()

    try:
        signal.signal(signal.SIGINT, on_signal)
    except (ValueError, OSError):
        pass
    if hasattr(signal, "SIGTERM"):
        try:
            signal.signal(signal.SIGTERM, on_signal)
        except (ValueError, OSError):
            pass

    # Windows: 主线程若无限期 Event.wait()，Ctrl+C 可能长期无法生效（阻塞在 C 层）。
    # 使用短超时轮询，让主线程能及时处理 SIGINT / KeyboardInterrupt。
    try:
        while not shutdown.is_set():
            shutdown.wait(timeout=0.3)
    except KeyboardInterrupt:
        print("\n[Ctrl+C] 正在关闭 Stage 测试服务器...", flush=True)

    server.stop(grace=2)
    print("服务器已停止。")


def main():
    parser = argparse.ArgumentParser(
        description="StageService gRPC 测试服务器（三轴台工装，区别于 grpc_test_server.py）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例：
  python stage_grpc_test_server.py                  # 默认 0.0.0.0:50052
  python stage_grpc_test_server.py --port 50051    # 独占端口时可用 50051
  python stage_grpc_test_server.py --external      # 打印本机 IPv4 与防火墙/CGNAT 提示
  python stage_grpc_test_server.py --host 127.0.0.1 --port 50052   # 仅本机（配 ngrok 时常用）
        """,
    )
    parser.add_argument(
        "--port",
        type=int,
        default=50052,
        help="监听端口（默认 50052，避免与 grpc_test_server.py 默认 50051 冲突）",
    )
    parser.add_argument(
        "--host",
        default=None,
        metavar="ADDR",
        help="监听地址（默认 0.0.0.0；仅本机可设 127.0.0.1，与 ngrok 同机时常用）",
    )
    parser.add_argument(
        "--external",
        "--external-ipv4",
        dest="external",
        action="store_true",
        help="打印局域网/外网访问提示与本机 IPv4 枚举（不改变默认绑定，可与 --host 同用）",
    )
    args = parser.parse_args()
    host = args.host if args.host is not None else "0.0.0.0"
    serve(args.port, host=host, external_mode=args.external)


if __name__ == "__main__":
    main()
