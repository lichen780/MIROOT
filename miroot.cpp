#include <filesystem>
#include <iostream>
#include <print>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdio>
#include <numeric>
#include <vector>

#define NOMINMAX
#include <Windows.h>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

const auto cwd = fs::current_path();
fs::path adb_bin = cwd / "adb" / "adb.exe";
fs::path fastboot_bin = cwd / "adb" / "fastboot.exe";
fs::path ksum = cwd / "ksu.apk";
fs::path ksud = cwd / "ksud";

// 颜色输出
enum class Color {
    RED = 12,
    GREEN = 10,
    YELLOW = 14,
    BLUE = 9,
    PURPLE = 13,
    CYAN = 11,
    WHITE = 15,
    GRAY = 8
};

void SetColor(Color color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
}

void ResetColor() {
    SetColor(Color::WHITE);
}

// 加载动画
void LoadingAnimation(const std::string& text, int ms = 1200) {
    SetColor(Color::CYAN);
    std::print("{} ", text);
    const char chars[] = { '|', '/', '-', '\\' };
    for (int i = 0; i < ms / 80; ++i) {
        std::print("\b{}", chars[i % 4]);
        std::flush(std::cout);
        std::this_thread::sleep_for(80ms);
    }
    std::print("\b✓\n");
    ResetColor();
}

// 标题分隔条
void ShowTitle(const std::string& title) {
    system("cls");
    SetColor(Color::PURPLE);
    std::println("========================================================");
    SetColor(Color::CYAN);
    std::println("               {}", title);
    SetColor(Color::PURPLE);
    std::println("========================================================\n");
    ResetColor();
}

// 成功/失败信息
void Success(const std::string& msg) {
    SetColor(Color::GREEN);
    std::println("✅ {}", msg);
    ResetColor();
}

void Error(const std::string& msg) {
    SetColor(Color::RED);
    std::println("❌ {}", msg);
    ResetColor();
}

void Info(const std::string& msg) {
    SetColor(Color::BLUE);
    std::println("ℹ️  {}", msg);
    ResetColor();
}

void Warn(const std::string& msg) {
    SetColor(Color::YELLOW);
    std::println("⚠️  {}", msg);
    ResetColor();
}

// 等待回车
void WaitEnter() {
    SetColor(Color::GRAY);
    std::print("\n请按回车键继续...");
    std::cin.ignore();
    ResetColor();
}

// 执行命令
[[maybe_unused]]
static auto Exec(const std::string& bin, const std::string& args) -> std::tuple<int, std::string> {
    std::string cmd = std::format("{} {} 2>&1", bin, args);
    FILE* pipe = _popen(cmd.c_str(), "r");

    if (!pipe) {
        int error_code = errno;
        return { error_code, strerror(error_code) };
    }

    std::string output;
    char buffer[1024];
    while (true) {
        if (fgets(buffer, sizeof(buffer), pipe) == nullptr) {
            if (ferror(pipe)) std::println("读取命令输出失败：{}", ferror(pipe));
            break;
        }
        output += buffer;
    }

    int exit_code = _pclose(pipe);
    if (exit_code != 0) {
        Error(std::format("命令执行失败，退出码：{}", exit_code));
        if (!output.empty()) std::println("输出：\n{}", output);
    }
    return { exit_code, output };
}

// ===================== 静默文件检查 =====================
bool CheckPhase1Files() {
    if (!fs::exists(adb_bin)) {
        Error("缺少 adb 文件");
        return false;
    }
    if (!fs::exists(fastboot_bin)) {
        Error("缺少 fastboot 文件");
        return false;
    }
    return true;
}

bool CheckPhase2Files() {
    if (!fs::exists(adb_bin)) {
        Error("缺少 adb 文件");
        return false;
    }
    if (!fs::exists(fastboot_bin)) {
        Error("缺少 fastboot 文件");
        return false;
    }
    if (!fs::exists(ksud)) {
        Error("缺少 ksud 文件");
        return false;
    }
    if (!fs::exists(ksum)) {
        Error("缺少 ksu.apk 文件");
        return false;
    }
    return true;
}

// ===================== 功能1：设置SELinux宽容 =====================
bool SetSELinuxPermissive() {
    ShowTitle("设置 SELinux 为宽容模式");
    Info("请完成以下操作：");
    std::println("  1. 打开开发者模式与USB调试");
    std::println("  2. 解锁手机进入桌面");
    std::println("  3. 连接电脑并允许调试授权");
    WaitEnter();

    LoadingAnimation("正在检测设备...");
    auto r1 = Exec(adb_bin.string(), "devices");
    if (get<0>(r1) != 0) return false;

    LoadingAnimation("正在重启进入 Fastboot...");
    auto r2 = Exec(adb_bin.string(), "reboot bootloader");
    if (get<0>(r2) != 0) return false;

    Info("请确认设备已进入 Fastboot 模式");
    WaitEnter();

    LoadingAnimation("正在设置 SELinux 为 Permissive...");
    auto r3 = Exec(fastboot_bin.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");
    if (get<0>(r3) != 0) return false;

    LoadingAnimation("正在开机...");
    auto r4 = Exec(fastboot_bin.string(), "continue");
    if (get<0>(r4) != 0) return false;

    Info("请等待设备完全开机");
    WaitEnter();
    LoadingAnimation("等待设备连接...");
    Exec(adb_bin.string(), "wait-for-device");

    LoadingAnimation("正在检查 SELinux 状态...");
    auto r5 = Exec(adb_bin.string(), "shell getenforce");
    if (get<0>(r5) != 0 || get<1>(r5).find("Permissive") == string::npos) {
        Error("SELinux 设置失败！");
        return false;
    }

    Success("SELinux 已成功设置为宽容模式！");
    system("pause >nul");
    return true;
}

// ===================== 功能2：安装KernelSU =====================
bool InstallKernelSU() {
    ShowTitle("安装 KernelSU 完整流程");

    Info("请打开锁屏并进入桌面，允许安装应用");
    WaitEnter();
    LoadingAnimation("等待设备连接...");
    Exec(adb_bin.string(), "wait-for-device");

    LoadingAnimation("正在推送 ksud 组件...");
    Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksud", ksud.string()));
    Exec(adb_bin.string(), "shell chmod 755 /data/local/tmp/ksud");

    LoadingAnimation("正在启动 KernelSU 守护进程...");
    Exec(adb_bin.string(), "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 '/data/local/tmp/ksud' i32 1 s16 'late-load' s16 '/data/local/tmp/ksud-log.txt' i32 60");
    std::this_thread::sleep_for(3s);

    auto r4 = Exec(adb_bin.string(), "shell grep 'kernelsu' /proc/modules");
    if (get<0>(r4) != 0 || get<1>(r4).find("kernelsu") == string::npos) {
        Error("KernelSU 模块加载失败！");
        return false;
    }
    Success("内核模块加载成功！");

    LoadingAnimation("正在卸载旧版管理器...");
    Exec(adb_bin.string(), "shell pm uninstall me.weishu.kernelsu");

    LoadingAnimation("正在安装新版 KernelSU 管理器...");
    Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksu.apk", ksum.string()));
    Exec(adb_bin.string(), "shell pm install -r /data/local/tmp/ksu.apk");

    Warn("请打开 KernelSU → 超级用户 → 授予 Shell Root 权限");
    WaitEnter();
    LoadingAnimation("等待授权...");
    Exec(adb_bin.string(), "wait-for-device");

    while (true) {
        auto r = Exec(adb_bin.string(), "shell su -c 'id -u'");
        if (get<1>(r).find('0') != string::npos) {
            Success("Root 授权成功！");
            break;
        }
        Error("请在 KernelSU 中授权后重试");
        WaitEnter();
    }

    LoadingAnimation("正在恢复 SELinux 为强制模式...");
    Exec(adb_bin.string(), "shell su -c 'setenforce 1'");
    auto r2 = Exec(adb_bin.string(), "shell getenforce");
    if (get<1>(r2).find("Enforcing") == string::npos) {
        Error("SELinux 恢复失败！");
        return false;
    }
    Success("SELinux 已恢复为强制模式！");

    Success("\n🎉 KernelSU 安装完成！");
    Info("正在启动 KernelSU 管理器...");
    Exec(adb_bin.string(), "shell am start -S me.weishu.kernelsu");
    system("pause >nul");
    return true;
}

// ===================== 主菜单 =====================
void ShowMainMenu() {
    while (true) {
        system("cls");
        SetColor(Color::CYAN);
        std::println("  _  __                      _ ____        _    ");
        std::println(" | |/ /___ _ __ _ __   ____| / ___| _   _| |__ ");
        std::println(" | ' // _ \\ '__| '_ \\ / _ \\ \\___ \\| | | | '_ \\");
        std::println(" | . \\  __/ |  | | | |  __/ |___) | |_| | |_) |");
        std::println(" |_|\\_\\___|_|  |_| |_|\\___|_|____/ \\__,_|_.__/ \n");

        SetColor(Color::PURPLE);
        std::println("========================================================");
        SetColor(Color::YELLOW);
        std::println("                  KernelSU 一键工具");
        SetColor(Color::PURPLE);
        std::println("========================================================");
        ResetColor();

        std::println("");
        std::println("  [1] 设置手机 SELinux 宽容");
        std::println("  [2] 安装 KernelSU");
        std::println("  [3] 退出程序");
        std::println("");
        SetColor(Color::GRAY);
        std::print("请输入选项 >> ");
        ResetColor();

        std::string input;
        std::getline(std::cin, input);

        if (input == "1") {
            if (CheckPhase1Files()) SetSELinuxPermissive();
            else system("pause");
        } else if (input == "2") {
            if (CheckPhase2Files()) InstallKernelSU();
            else system("pause");
        } else if (input == "3") {
            break;
        } else {
            Error("无效输入，请重新选择");
            std::this_thread::sleep_for(1s);
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("KernelSU 一键安装工具");
    ShowMainMenu();
    return 0;
}
