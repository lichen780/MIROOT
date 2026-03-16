# MIROOT - 小米设备免解锁 BL Root 工具

<div align="center">

![GitHub stars](https://img.shields.io/github/stars/lichen780/MIROOT?style=for-the-badge)
![GitHub forks](https://img.shields.io/github/forks/lichen780/MIROOT?style=for-the-badge)
![GitHub license](https://img.shields.io/github/license/lichen780/MIROOT?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Windows-blue?style=for-the-badge)

**⚠️ 仅供学习研究使用，请勿用于非法用途 ⚠️**

</div>

---

## 📖 项目简介

MIROOT 是一款专为小米/Redmi 设备设计的 Root 工具，支持**免解锁 Bootloader**获取 Root 权限。本工具采用漏洞利用方式，无需官方解锁等待期，即可快速完成设备 Root。

> **说明**：本软件是根据近期网上已有工具整理汇总，仅供学习研究使用。

### ✨ 核心功能

- 🔓 **免解 BL Root** - 无需解锁 Bootloader 即可获取 Root 权限
- 🛠️ **SELinux 宽容模式** - 设置设备为 Permissive 模式
- 📱 **KernelSU 管理器安装** - 一键安装 KernelSU
- 🔐 **骁龙 8E5 解锁 BL** - 支持特定机型解锁 Bootloader

---

## 🔬 漏洞原理

### 1. 免解锁 BL Root 原理

本工具利用的是 **Fastboot OEM 命令参数注入漏洞**。

#### 漏洞描述

在部分小米设备的 Fastboot 实现中，`oem` 命令的参数处理存在安全缺陷。正常情况下，Fastboot 的 `oem` 命令用于执行设备制造商自定义的操作，参数应该经过严格的验证和过滤。

然而，在某些固件版本中，Fastboot 守护进程在处理 `oem set-gpu-preemption` 命令时，未能正确过滤传递给内核启动参数的内容，导致攻击者可以注入任意的内核命令行参数。

#### 利用方式

```bash
fastboot oem set-gpu-preemption 0 androidboot.selinux=permissive
```

上述命令中：
- `set-gpu-preemption 0` 是合法的设备 OEM 命令
- `androidboot.selinux=permissive` 是被注入的内核启动参数

#### 技术细节

1. **命令解析缺陷**：Fastboot 守护进程将整行命令作为字符串处理，未对额外参数进行边界检查
2. **参数传递链**：
   ```
   Fastboot 命令 → OEM 处理函数 → 内核启动参数 → Bootloader 验证绕过
   ```
3. **SELinux 宽容模式**：注入的 `androidboot.selinux=permissive` 参数使内核以宽容模式启动 SELinux，允许所有操作仅记录日志

#### 影响范围

- **受影响设备**：部分搭载 MIUI 的小米/Redmi 设备
- **受影响固件**：特定版本之前的 Fastboot 实现
- **漏洞类型**：CWE-20 (Improper Input Validation)

### 2. 骁龙 8E5 BL 解锁原理

#### 漏洞描述

在骁龙 8E5 平台的部分小米设备中，存在通过 **系统服务调用链** 实现 Bootloader 解锁的方法。

#### 利用流程

```
ADB → 系统服务调用 → MQSAS 服务 → 分区写入 → BL 解锁
```

#### 核心技术

1. **服务调用**：
   ```bash
   adb shell service call miui.mqsas.IMQSNative 21 i32 1 s16 "dd" \
     i32 1 s16 'if=/data/local/tmp/gbl_efi_unlock.efi of=/dev/block/by-name/efisp' \
     s16 '/data/mqsas/log.txt' i32 60
   ```

2. **参数解析**：
   - `miui.mqsas.IMQSNative` - 小米质量服务抽象层接口
   - `21` - 方法索引号
   - `s16 'if=...of=...'` - 注入的 shell 命令参数

3. **权限提升**：该服务以系统权限运行，可访问受保护的分区设备节点

---

## 🚀 快速开始

### 环境要求

- **操作系统**：Windows 10/11
- **设备要求**：
  - 小米/Redmi 手机
  - USB 调试已开启
  - 电脑已授权 ADB 调试

### 使用方法

#### 方式一：直接使用（推荐）

1. 下载最新版本的 [Release](https://github.com/lichen780/MIROOT/releases)
2. 解压到任意目录
3. 双击运行 `MIROOT.exe`
4. 按提示操作

#### 方式二：自行编译

```bash
# 克隆仓库
git clone https://github.com/lichen780/MIROOT.git
cd MIROOT

# 使用 MSVC 编译
cl /EHsc /std:c++20 /utf-8 miroot.cpp /link urlmon.lib shell32.lib
```

### 功能菜单

```
主菜单
├── [1] 免解 BL ROOT
│   ├── [1] 设置 SELinux 宽容模式
│   └── [2] 安装 KernelSU 管理器
└── [2] 骁龙 8E5 解 BL 锁
```

---

## 📋 支持设备

### 免解 BL Root 支持设备

**已验证支持设备：**

| 系列 | 机型 | 状态 |
|------|------|------|
| **小米数字系列** | 小米 13 / 13 Pro / 13 Ultra | ✅ 已验证 |
| | 小米 14 / 14 Pro / 14 Ultra | ✅ 已验证 |
| | 小米 15 / 15 Pro / 15 Ultra | ✅ 已验证 |

**其他设备：**

| 系列 | 支持情况 | 备注 |
|------|---------|------|
| 小米其他系列 | ⚠️ 自行测试 | 骁龙处理器理论上可用 |
| Redmi K 系列 | ⚠️ 自行测试 | 骁龙处理器理论上可用 |
| Redmi Note 系列 | ⚠️ 自行测试 | 骁龙处理器理论上可用 |
| 其他机型 | ❓ 待测试 | 欢迎反馈 |

> **重要提示**：
> - ✅ **骁龙处理器**：理论上全部支持
> - ❌ **联发科 (MediaTek) 处理器**：不支持
> - ❌ **其他 CPU（三星 Exynos 等）**：不支持
> - 请在 **设置 → 关于手机 → 处理器信息** 确认您的设备 CPU 型号

### 骁龙 8 Elite 解锁 BL 支持设备

**仅支持搭载高通骁龙 8 Elite (Gen 5) 处理器的小米/Redmi 手机：**

| 系列 | 机型 | 发布年份 |
|------|------|---------|
| **小米数字系列** | 小米 17 / 17 Pro / 17 Pro Max | 2025 |
| **Redmi K 系列** | Redmi K90 Pro Max | 2025 |

> **注意**：
> - 基础版 Redmi K90 **不支持**（非骁龙 8 Elite 处理器）
> - 不同设备/固件版本的兼容性可能有所不同，使用前请确认。

---

## 🔒 安全补丁版本要求

**两个漏洞均对安卓安全补丁版本有要求：**

| 安全补丁日期 | 支持情况 | 说明 |
|-------------|---------|------|
| **2026-01-01 之前** | ✅ 完全支持 | 推荐 |
| **2026-02-01** | ⚠️ 部分支持 | 需要自行测试 |
| **2026-03-01 及之后** | ❌ 不支持 | 漏洞已修复 |

> **重要提示**：
> - 请在 **设置 → 关于手机 → 安卓版本 → 安全补丁级别** 查看当前补丁日期
> - 如果设备已更新到 2026-03-01 或更新的安全补丁，漏洞已被修复，无法使用本工具
> - 部分 2026-02-01 补丁设备可能仍可使用，但需要自行测试确认

---

## ⚠️ 风险提示

使用本工具存在以下风险：

1. **设备变砖风险**：操作不当可能导致设备无法启动
2. **数据丢失风险**：建议操作前备份重要数据
3. **保修失效风险**：Root 操作可能影响官方保修
4. **安全风险**：SELinux 宽容模式可能降低设备安全性

**本工具仅供学习研究使用，使用本工具造成的一切后果由使用者自行承担！**

---

## 🛠️ 故障排除

### 常见问题

#### Q1: ADB 无法识别设备
- 确保已安装正确的 USB 驱动
- 检查 USB 调试是否已开启
- 尝试更换 USB 数据线或端口

#### Q2: 下载文件失败
- 检查网络连接
- 关闭防火墙/杀毒软件后重试
- 手动下载所需文件到程序目录

#### Q3: Root 后无法开机
- 尝试进入 Fastboot 模式重启
- 刷入官方固件恢复
- 联系专业人员协助

---

## 📁 文件说明

| 文件 | 说明 |
|------|------|
| `MIROOT.exe` | 主程序 |
| `KernelSU.apk` | KernelSU 管理器（可选） |
| `gbl_efi_unlock.efi` | BL 解锁文件（可选） |
| `adb/` | ADB 工具目录（自动下载） |

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

- 报告 Bug
- 功能建议
- 设备兼容性反馈
- 文档改进

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

感谢以下项目和贡献者：

- [KernelSU](https://github.com/tiann/KernelSU) - 优秀的内核级 Root 方案
- [Google ADB](https://developer.android.com/studio/releases/platform-tools) - Android 调试桥接工具
- 所有为本项目做出贡献的开发者

---

## 📬 联系方式

- GitHub: [@lichen780](https://github.com/lichen780)
- 项目地址：[https://github.com/lichen780/MIROOT](https://github.com/lichen780/MIROOT)

---

<div align="center">

**如果本项目对你有帮助，请给一个 ⭐ Star！**

Made with ❤️ by MIROOT Team

</div>
