# MIROOT - 小米解锁 BL ROOT 工具

<div align="center">

![GitHub stars](https://img.shields.io/github/stars/lichen780/MIROOT?style=for-the-badge)
![GitHub forks](https://img.shields.io/github/forks/lichen780/MIROOT?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Windows-blue?style=for-the-badge)

**⚠️ 仅供学习研究使用，请勿用于非法用途 ⚠️**

</div>

---

## 📖 项目简介

本工具整合了网上已有的小米解锁/Root 方案，提供统一的图形化操作界面。**核心文件来源于小绿书平台的公开分享，本人仅做整合打包。**

### 功能菜单

```
├── [1] 免解 BL ROOT
│   ├── 设置 SELinux 宽容模式
│   └── 安装 KernelSU 管理器（可选）
├── [2] 骁龙 8E5 解 BL 锁
├── [3] 骁龙 8E  解 BL 锁
├── [4] 骁龙 8G3 解 BL 锁
└── [0] 退出程序
```

---

## 📱 支持机型

### 免解 BL ROOT（骁龙处理器通用）

| 系列 | 机型 |
|------|------|
| 小米数字系列 | 小米 13 / 14 / 15 全系列 |
| Redmi K 系列 | 骁龙处理器机型理论上可用 |

> 联发科、三星 Exynos 处理器不支持

### 骁龙 8E5 解 BL 锁

仅支持骁龙 8 Elite (Gen 5) 平台设备，需 `gbl_efi_unlock.efi` 文件。

| 系列 | 机型 |
|------|------|
| Xiaomi | 17、17 Pro、17 Pro Max |
| Redmi | K90 Pro Max |

> 基础版 Redmi K90 不支持（非骁龙 8 Elite 处理器）

### 骁龙 8E 解 BL 锁

| 系列 | 机型 |
|------|------|
| Redmi | K80 Pro、K90 |
| Xiaomi | 15、15 Pro、15 Ultra |
| 平板 | Pad 8 Pro |

### 骁龙 8G3 解 BL 锁

| 系列 | 机型 |
|------|------|
| Redmi | K70 Pro、K80 |
| Xiaomi | 14、14 Pro、14 Ultra |
| MIX | Flip、Fold4 |

---

## 🔒 安全补丁版本要求

| 安全补丁日期 | 支持情况 |
|-------------|---------|
| 2026-01-01 之前 | ✅ 完全支持 |
| 2026-02-01 | ⚠️ 需自行测试 |
| 2026-03-01 及之后 | ❌ 漏洞已修复 |

---

## 🚀 使用方法

1. 下载 [MIROOT](https://github.com/lichen780/MIROOT) 最新版本
2. 解压到任意目录
3. 双击运行 `免解 BL ROOT 工具.exe`
4. 按菜单提示操作

### 编译

```bash
cl /EHsc /std:c++latest /utf-8 /O2 miroot.cpp /link urlmon.lib shell32.lib
```

---

## 📁 文件说明

| 文件/目录 | 说明 |
|-----------|------|
| `免解 BL ROOT 工具.exe` | 主程序 |
| `adb/` | ADB/Fastboot 工具（首次运行自动下载） |
| `8e5-unlock/` | 骁龙 8E5 解锁资源 |
| `8e-unlock/` | 骁龙 8E 解锁资源 |
| `8g3-unlock/` | 骁龙 8G3 解锁资源 |
| `KernelSU.apk` | KernelSU 管理器 |

---

## ⚠️ 免责声明

- 解锁 BL **必定清除所有数据**，请提前备份
- 操作不当可能导致设备变砖
- Root 后可能失去官方保修
- **本工具仅供学习研究，使用后果自负**

---

## 🙏 代码来源

本工具核心文件来源于小绿书平台的公开分享：

- **免解 BL ROOT / 骁龙 8E5 解锁** 
- **骁龙 8E 解锁** 
- **骁龙 8G3 解锁** 
- [KernelSU](https://github.com/tiann/KernelSU) - 内核级 Root 方案
- [Google ADB](https://developer.android.com/studio/releases/platform-tools) - Android 调试工具

**本人仅做整合打包，不做任何担保。**
