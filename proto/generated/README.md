# proto/generated

此目录由 `protoc` 自动生成，**不提交到版本控制**（已加入 .gitignore）。

## 手动生成

请确保已安装 gRPC C++ 并设置好 `GRPC_ROOT` 环境变量，然后执行：

```powershell
# Windows PowerShell
$env:GRPC_ROOT = "C:\grpc"   # 改为实际路径

protoc `
  --proto_path=../proto `
  --cpp_out=. `
  --grpc_out=. `
  --plugin=protoc-gen-grpc="$env:GRPC_ROOT/bin/grpc_cpp_plugin.exe" `
  ../proto/device_data.proto
```

```bash
# Linux / macOS
GRPC_ROOT=/usr/local   # 改为实际路径

protoc \
  --proto_path=../proto \
  --cpp_out=. \
  --grpc_out=. \
  --plugin=protoc-gen-grpc=$GRPC_ROOT/bin/grpc_cpp_plugin \
  ../proto/device_data.proto
```

生成后将产生以下四个文件（已加入 .pro SOURCES/HEADERS）：
- `device_data.pb.h`
- `device_data.pb.cc`
- `device_data.grpc.pb.h`
- `device_data.grpc.pb.cc`

## qmake 自动生成

当以 `CONFIG+=grpc_client GRPC_ROOT=<path>` 运行 qmake 时，会通过
`QMAKE_EXTRA_COMPILERS` 自动调用 protoc，无需手动执行以上命令。
