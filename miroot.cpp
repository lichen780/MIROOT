#include <filesystem>
#include <iostream>
#include <print>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdio>
#include <numeric>
#include <algorithm>

#define NOMINMAX
#include <Windows.h>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

const auto cwd = fs::current_path();
fs::path adb_bin = cwd / "adb" / "adb.exe";
fs::path fastboot_bin = cwd / "adb" / "fastboot.exe";
fs::path ksum = cwd / "ksu.apk";
fs::path ksud = cwd / "ksud";

// 控制台颜色
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
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

void ResetColor() {
    SetColor(Color::WHITE);
}

// 加载动画
void Loading(const std::string& text) {
    SetColor(Color::CYAN);
    std::print("{} ", text);
    const char ch[] = "|/-\\";
    for (int i = 0; i < 12; ++i) {
        std::print("\b{}", ch[i % 4]);
        std::flush(std::cout);
        std::this_thread::sleep_for(90ms);
    }
    std::print("\b✓\n");
    ResetColor();
}

// 标题界面
void Title(const std::string& title) {
    system("cls");
    SetColor(Color::PURPLE);
    std::println("========================================================");
    SetColor(Color::CYAN);
    std::println("                   {}", title);
    SetColor(Color::PURPLE);
    std::println("========================================================\n");
    ResetColor();
}

// 信息输出
void OK(const std::string& msg) { SetColor(Color::GREEN); std::println("✅ {}", msg); ResetColor(); }
void ERR(const std::string& msg) { SetColor(Color::RED); std::println("❌ {}", msg); ResetColor(); }
void INFO(const std::string& msg) { SetColor(Color::BLUE); std::println("ℹ️  {}", msg); ResetColor(); }
void WARN(const std::string& msg) { SetColor(Color::YELLOW); std::println("⚠️  {}", msg); ResetColor(); }

// 等待回车
void Wait() {
    SetColor(Color::GRAY);
    std::print("\n按回车键继续...");
    std::cin.ignore();
    ResetColor();
}

// 执行命令
[[maybe_unused]]
static auto Exec(const std::string& bin, const std::string& args) -> std::tuple<int, std::string> {
    std::string cmd = std::format("{} {} 2>&1", bin, args);
    FILE* pipe = _popen(cmd.c_str(), "r");

    if (!pipe) {
        int e = errno;
        return { e, strerror(e) };
    }

    std::string out;
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) out += buf;

    int code = _pclose(pipe);
    return { code, out };
}

// 循环等待设备连接（直到连上才退出）
void WaitForDeviceLoop() {
    INFO("等待设备连接，请确保开启USB调试...\n");
    while (true) {
        auto [code, output] = Exec(adb_bin.string(), "devices");
        if (code == 0 && output.find("device") != std::string::npos && output.find("offline") == std::string::npos) {
            OK("设备已成功连接！");
            break;
        }
        std::this_thread::sleep_for(1s);
    }
}

// 获取手机信息
void ShowDeviceInfo() {
    Loading("正在获取手机信息");
    auto [_, brand] = Exec(adb_bin.string(), "shell getprop ro.product.brand");
    auto [__, model] = Exec(adb_bin.string(), "shell getprop ro.product.model");
    auto [___, android] = Exec(adb_bin.string(), "shell getprop ro.build.version.release");
    auto [____, cpu] = Exec(adb_bin.string(), "shell getprop ro.product.board");

    brand.erase(remove_if(brand.begin(), brand.end(), isspace), brand.end());
    model.erase(remove_if(model.begin(), model.end(), isspace), model.end());
    android.erase(remove_if(android.begin(), android.end(), isspace), android.end());
    cpu.erase(remove_if(cpu.begin(), cpu.end(), isspace), cpu.end());

    SetColor(Color::YELLOW);
    std::println("📱 手机品牌：{}", brand);
    std::println("📱 手机型号：{}", model);
    std::println("🤖 安卓版本：{}", android);
    std::println("⚙️  处理器：{}\n", cpu);
    ResetColor();
}

// ===================== 静默文件检查 =====================
bool Check1() {
    if (!fs::exists(adb_bin)) { ERR("缺少 adb 文件"); return false; }
    if (!fs::exists(fastboot_bin)) { ERR("缺少 fastboot 文件"); return false; }
    return true;
}

bool Check2() {
    if (!fs::exists(adb_bin)) { ERR("缺少 adb 文件"); return false; }
    if (!fs::exists(fastboot_bin)) { ERR("缺少 fastboot 文件"); return false; }
    if (!fs::exists(ksud)) { ERR("缺少 ksud 文件"); return false; }
    if (!fs::exists(ksum)) { ERR("缺少 ksu.apk 文件"); return false; }
    return true;
}

// ===================== 功能1：设置SELinux宽容 =====================
bool Func1_SetSELinux() {
    Title("免解BL - 设置SELinux宽容模式");

    // 循环检测设备，连上才继续
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("重启至 Fastboot");
    auto [c2, o2] = Exec(adb_bin.string(), "reboot bootloader");
    if (c2 != 0) return false;

    INFO("请确认手机已进入 Fastboot 模式");
    Wait();

    Loading("设置 SELinux 为宽容");
    auto [c3, o3] = Exec(fastboot_bin.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");
    if (c3 != 0) return false;

    Loading("开机");
    Exec(fastboot_bin.string(), "continue");

    INFO("请等待手机完全开机");
    Wait();
    Loading("等待设备连接");
    Exec(adb_bin.string(), "wait-for-device");

    Loading("检查SELinux状态");
    auto [c5, o5] = Exec(adb_bin.string(), "shell getenforce");
    if (c5 != 0 || o5.find("Permissive") == std::string::npos) {
        ERR("SELinux 设置失败！");
        return false;
    }

    OK("SELinux 已设置为宽容模式");
    system("pause >nul");
    return true;
}

// ===================== 功能2：安装ROOT权限 =====================
bool Func2_InstallRoot() {
    Title("免解BL - 安装ROOT权限");

    // 循环检测设备，连上才继续
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("推送ROOT组件");
    Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksud", ksud.string()));
    Exec(adb_bin.string(), "shell chmod 755 /data/local/tmp/ksud");

    Loading("启动ROOT守护进程");
    Exec(adb_bin.string(), "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 '/data/local/tmp/ksud' i32 1 s16 'late-load' s16 '/data/local/tmp/ksud-log.txt' i32 60");
    std::this_thread::sleep_for(3s);

    auto [c4, o4] = Exec(adb_bin.string(), "shell grep 'kernelsu' /proc/modules");
    if (c4 != 0 || o4.find("kernelsu") == std::string::npos) {
        ERR("ROOT模块加载失败！");
        return false;
    }
    OK("内核ROOT模块加载成功");

    Loading("卸载旧版管理器");
    Exec(adb_bin.string(), "shell pm uninstall me.weishu.kernelsu");

    Loading("安装ROOT管理器");
    Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksu.apk", ksum.string()));
    Exec(adb_bin.string(), "shell pm install -r /data/local/tmp/ksu.apk");

    WARN("请打开 KernelSU → 超级用户 → 允许 Shell ROOT权限");
    Wait();

    while (true) {
        Loading("检查ROOT授权");
        auto [r, o] = Exec(adb_bin.string(), "shell su -c 'id -u'");
        if (o.find('0') != std::string::npos) {
            OK("ROOT授权成功！");
            break;
        }
        ERR("未授权，请在KernelSU允许后重试");
        Wait();
    }

    Loading("恢复SELinux为安全模式");
    Exec(adb_bin.string(), "shell su -c 'setenforce 1'");
    auto [rr, oo] = Exec(adb_bin.string(), "shell getenforce");
    if (oo.find("Enforcing") == std::string::npos) {
        ERR("SELinux恢复失败");
        return false;
    }
    OK("SELinux 已恢复为强制模式");

    OK("\n🎉 免解BL ROOT安装全部完成！");
    INFO("正在启动ROOT管理器...");
    Exec(adb_bin.string(), "shell am start -S me.weishu.kernelsu");
    system("pause >nul");
    return true;
}

// ===================== 主菜单 =====================
void Menu() {
    while (true) {
        system("cls");
        SetColor(Color::CYAN);

        std::println("    __  ___                       __  ______  ____");
        std::println("   /  |/  /___  ____  ____ ______/  |/  / _ \\/ __/");
        std::println("  / /|_/ / __ \\/ __ \\/ __ `/ ___/ /|_/ / , _/ /    ");
        std::println(" / /  / / /_/ / /_/ / /_/ / /  / /  / / /| / /___ ");
        std::println("/_/  /_/\\____/\\____/\\__,_/_/  /_/  /_/_/ |_/_____/  ");

        SetColor(Color::PURPLE);
        std::println("========================================================");
        SetColor(Color::YELLOW);
        std::println("                 免解BL ROOT 工具");
        SetColor(Color::PURPLE);
        std::println("========================================================");
        ResetColor();

        std::println("");
        std::println("  [1] 设置SELinux宽容");
        std::println("  [2] 安装ROOT权限");
        std::println("  [3] 退出");
        std::println("");
        SetColor(Color::GRAY);
        std::print("请选择功能 >> ");
        ResetColor();

        std::string s;
        std::getline(std::cin, s);

        if (s == "1") { if (Check1()) Func1_SetSELinux(); else system("pause"); }
        if (s == "2") { if (Check2()) Func2_InstallRoot(); else system("pause"); }
        if (s == "3") break;

        std::this_thread::sleep_for(300ms);
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("免解BL ROOT工具 | 无需解锁BL直接ROOT");
    Menu();
    return 0;
}
