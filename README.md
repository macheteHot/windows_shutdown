<!-- filepath: /Users/machete/myCode/pc-shutdown/README.md -->

# 🖥️ HomePCServer

## 📡 ESP32 方案简介

本项目推荐与 [ESP32 单片机应用](https://github.com/macheteHot/c3Homekit) 配合使用：

- 🍏 ESP32 可实现 HomeKit 智能家居接入，支持苹果 Home App 控制。
- 🔘 按钮单击：ESP32 控制主板开机（如继电器短接开机针脚）。
- 🔘 按钮双击：ESP32 通过 UDP 广播发送 `SHUTDOWN_ESP` + 目标 PC MAC，实现远程关机。
- 📶 ESP32 监听心跳包，判断 PC 在线状态，实现自动化联动。

---

## 📝 简介

本项目为 Windows 下的局域网心跳与远程关机服务，适用于与 [单片机应用](https://github.com/macheteHot/c3Homekit) 配合，实现远程安全关机和设备在线检测。

---

## ⚙️ 原理

- 程序后台运行，定时（默认 200ms）向局域网广播心跳包，内容包含本机以太网网卡的 MAC 地址。
- 监听 UDP 40000 端口，收到包含 `SHUTDOWN_ESP` 和本机任一 MAC 地址的 UDP 包时自动关机。
- 仅局域网网卡（IP 为 10.x.x.x、192.168.x.x、172.16-31.x.x）参与心跳广播。

---

## 🛠️ 编译

### Windows

1. 安装 Visual Studio 或 MinGW（需支持 ws2_32 和 iphlpapi 库）。
2. 编译命令示例（MinGW）：
   ```sh
   gcc src/main.c src/network_utils.c -o home_pc_server.exe -lws2_32 -liphlpapi
   ```
3. 也可用 GitHub Actions 自动编译并发布 exe。

### Linux/macOS

> ⚠️ 本项目仅支持 Windows。

---

## 🚀 使用方法

1. 运行 `home_pc_server.exe`，程序将自动后台运行，无窗口。
2. 程序会定时广播心跳包。
3. 监听 UDP 40000 端口，收到合法关机包后自动关机。

---

## 🏁 自启动

推荐方式：使用 [NSSM](https://nssm.cc/) 将 `home_pc_server.exe` 注册为 Windows 服务。

**操作方法：**

1. 下载 NSSM 工具：[NSSM 官方下载](https://nssm.cc/download)。
2. 将 NSSM 解压到任意目录，并将其路径添加到系统环境变量。
3. 打开命令提示符（管理员权限），运行以下命令：
   ```sh
   nssm install HomePCServer
   ```
4. 在弹出的窗口中：
   - **Path**：选择 `home_pc_server.exe` 的完整路径。
   - **Startup directory**：选择 `home_pc_server.exe` 所在的目录。
   - 点击 **Install service**。
5. 启动服务：
   ```sh
   nssm start HomePCServer
   ```

完成后，`home_pc_server.exe` 将作为 Windows 服务运行，并在系统启动时自动启动。

---

## 📦 心跳包格式

```
HEARTBEAT|AABBCCDDEEFF
```

- 其中 `AABBCCDDEEFF` 为本机以太网网卡的 MAC 地址（无分隔符）。
- 多网卡时，每张网卡分别广播。

---

## ⏹️ 关机包格式

- UDP 40000 端口，内容需包含 `SHUTDOWN_ESP` 和本机 MAC 地址（无分隔符）。

---

## 💡 典型应用场景

- 局域网内远程安全关机
- 单片机/智能家居设备检测 PC 在线状态

---

## 🤝 与 ESP32 配合使用

本程序可与 [ESP32 单片机应用](https://github.com/macheteHot/c3Homekit) 协同，实现远程控制 PC 的开关机：

- **按键按一下**：ESP32 发送“开机”信号（如通过继电器或主板开机针脚），实现物理开机。
- **按键按两下**：ESP32 通过 UDP 广播发送包含 `SHUTDOWN_ESP` 和目标 PC 网卡 MAC 地址的关机包，PC 端收到后自动关机。

ESP32 可通过监听心跳包判断 PC 是否在线，实现状态同步和智能控制。

---

## ⚠️ 免责声明

- 请勿在生产环境或未经授权的网络中使用本程序。
- 关机操作需谨慎，建议测试环境充分验证。

---

## ©️ License

This project is licensed under the MIT License. See the LICENSE file for details.

---
