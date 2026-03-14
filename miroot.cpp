#include <filesystem>
#include <iostream>
#include <print>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdio>
#include <limits>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <windows.h>
#include <string>
#include <urlmon.h>
#include <shellapi.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

const fs::path cwd = fs::current_path();
const fs::path ADB_DIR = cwd / "adb";
const fs::path ADB_EXE = ADB_DIR / "adb.exe";
const fs::path FASTBOOT_EXE = ADB_DIR / "fastboot.exe";

const string ADB_URL = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";
const string ZIP_FILE = "platform-tools.zip";
const string TOOL_DIR = "platform-tools";

fs::path ksum = cwd / "ksu.apk";
fs::path ksud = cwd / "ksud";

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

void Loading(const string& text) {
    SetColor(Color::CYAN);
    cout << text << " ";
    const char ch[] = "|/-\\";
    for (int i = 0; i < 12; ++i) {
        cout << "\b" << ch[i % 4];
        cout.flush();
        this_thread::sleep_for(90ms);
    }
    cout << "\b✓\n";
    ResetColor();
}

void Title(const string& title) {
    system("cls");
    SetColor(Color::PURPLE);
    println("========================================================");
    SetColor(Color::CYAN);
    println("                   {}", title);
    SetColor(Color::PURPLE);
    println("========================================================\n");
    ResetColor();
}

void OK(const string& msg) { SetColor(Color::GREEN); println("✅ {}", msg); ResetColor(); }
void ERR(const string& msg) { SetColor(Color::RED); println("❌ {}", msg); ResetColor(); }
void INFO(const string& msg) { SetColor(Color::BLUE); println("ℹ️  {}", msg); ResetColor(); }
void WARN(const string& msg) { SetColor(Color::YELLOW); println("⚠️  {}", msg); ResetColor(); }

void PressAnyKeyBack() {
    SetColor(Color::GRAY);
    cout << "\n执行完成！按回车键返回主菜单...";
    cin.ignore(100000, '\n');
    cin.get();
    ResetColor();
}

static auto Exec(const string& bin, const string& args) -> tuple<int, string> {
    string cmd = format("\"{}\" {} 2>&1", bin, args);
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        int e = errno;
        return { e, strerror(e) };
    }
    string out;
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) out += buf;
    int code = _pclose(pipe);
    return { code, out };
}

bool DownloadADB() {
    INFO("正在下载 ADB 工具包...");
    HRESULT res = URLDownloadToFileA(NULL, ADB_URL.c_str(), ZIP_FILE.c_str(), 0, NULL);
    if (res != S_OK) {
        ERR("下载失败！请检查网络");
        return false;
    }
    OK("ADB 下载完成！");
    return true;
}

bool ExtractADB() {
    INFO("正在解压至 adb 文件夹...");
    if (!fs::exists(ADB_DIR)) fs::create_directories(ADB_DIR);
    system(format("powershell -Command \"Expand-Archive -Path {} -DestinationPath {} -Force\" >nul 2>&1", ZIP_FILE, ADB_DIR.string()).c_str());
    this_thread::sleep_for(6s);

    fs::path extracted = ADB_DIR / TOOL_DIR;
    if (fs::exists(extracted)) {
        for (const auto& f : fs::directory_iterator(extracted)) {
            fs::path dst = ADB_DIR / f.path().filename();
            if (fs::exists(dst)) fs::remove(dst);
            fs::rename(f.path(), dst);
        }
        fs::remove_all(extracted);
    }
    OK("ADB 解压完成！");
    return true;
}

void AutoSetupADB() {
    if (fs::exists(ADB_EXE)) {
        OK("ADB 工具已存在，跳过下载");
        return;
    }

    WARN("未检测到 adb 文件夹，开始自动部署...");
    if (!DownloadADB()) return;
    if (!ExtractADB()) return;
    fs::remove(ZIP_FILE);
    OK("ADB 全自动部署完成！");
}

void KillAdbFastboot() {
    if (fs::exists(ADB_EXE)) {
        system(format("\"{}\" kill-server >nul 2>&1", ADB_EXE.string()).c_str());
    }
    system("taskkill /f /im adb.exe >nul 2>&1");
    system("taskkill /f /im fastboot.exe >nul 2>&1");
}

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        KillAdbFastboot();
        return TRUE;
    }
    return FALSE;
}

bool CheckDeviceSerial() {
    auto [code, output] = Exec(ADB_EXE.string(), "devices");
    istringstream iss(output);
    string line;
    while (getline(iss, line)) {
        if (line.find("List of devices") != string::npos) continue;
        if (line.empty()) continue;
        size_t space_pos = line.find(" ");
        if (space_pos == string::npos) continue;
        string serial = line.substr(0, space_pos);
        string status = line.substr(space_pos);
        if (serial.length() >= 10 && status.find("device") != string::npos) {
            return true;
        }
    }
    return false;
}

void WaitForDeviceLoop() {
    INFO("等待设备连接，请开启USB调试...\n");
    while (true) {
        if (CheckDeviceSerial()) {
            OK("设备已成功连接！");
            break;
        }
        this_thread::sleep_for(3s);
    }
}

void ShowDeviceInfo() {
    Loading("正在获取手机信息");
    auto [_, brand] = Exec(ADB_EXE.string(), "shell getprop ro.product.brand");
    auto [__, model] = Exec(ADB_EXE.string(), "shell getprop ro.product.model");
    auto [___, android] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.release");
    auto [____, cpu] = Exec(ADB_EXE.string(), "shell getprop ro.product.board");

    brand.erase(remove_if(brand.begin(), brand.end(), ::isspace), brand.end());
    model.erase(remove_if(model.begin(), model.end(), ::isspace), model.end());
    android.erase(remove_if(android.begin(), android.end(), ::isspace), android.end());
    cpu.erase(remove_if(cpu.begin(), cpu.end(), ::isspace), cpu.end());

    SetColor(Color::YELLOW);
    println("📱 手机品牌：{}", brand);
    println("📱 手机型号：{}", model);
    println("🤖 安卓版本：{}", android);
    println("⚙️  处理器：{}\n", cpu);
    ResetColor();
}

bool Check1() {
    if (!fs::exists(ADB_EXE)) { ERR("缺少 adb.exe"); return false; }
    if (!fs::exists(FASTBOOT_EXE)) { ERR("缺少 fastboot.exe"); return false; }
    return true;
}

bool Check2() {
    if (!fs::exists(ADB_EXE)) { ERR("缺少 adb.exe"); return false; }
    if (!fs::exists(FASTBOOT_EXE)) { ERR("缺少 fastboot.exe"); return false; }
    if (!fs::exists(ksud)) { ERR("缺少 ksud 文件"); return false; }
    if (!fs::exists(ksum)) { ERR("缺少 ksu.apk 文件"); return false; }
    return true;
}

bool Func1_SetSELinux() {
    Title("免解BL - 设置SELinux宽容模式");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("重启至 Fastboot");
    Exec(ADB_EXE.string(), "reboot bootloader");
    INFO("请确认进入 Fastboot 后按回车");
    PressAnyKeyBack();

    Loading("设置 SELinux 为宽容");
    Exec(FASTBOOT_EXE.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");

    Loading("重启系统");
    Exec(FASTBOOT_EXE.string(), "continue");
    INFO("请等待开机后按回车");
    PressAnyKeyBack();

    OK("SELinux 设置完成！");
    PressAnyKeyBack();
    return true;
}

bool Func2_InstallRoot() {
    Title("免解BL - 安装ROOT权限");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("推送ROOT组件");
    Exec(ADB_EXE.string(), format("push {} /data/local/tmp/ksud", ksud.string()));
    Exec(ADB_EXE.string(), "shell chmod 755 /data/local/tmp/ksud");

    Loading("启动ROOT服务");
    Exec(ADB_EXE.string(), "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 '/data/local/tmp/ksud' i32 1 s16 'late-load' i32 60");
    this_thread::sleep_for(2s);

    Loading("安装 KernelSU 管理器");
    Exec(ADB_EXE.string(), format("push {} /data/local/tmp/ksu.apk", ksum.string()));
    Exec(ADB_EXE.string(), "shell pm install -r /data/local/tmp/ksu.apk");

    OK("ROOT 安装完成！请打开 KernelSU 授权");
    PressAnyKeyBack();
    return true;
}

void Menu() {
    while (true) {
        system("cls");
        SetColor(Color::CYAN);
        println("  _   _  _   _    ____   _      ___  _   _  ");
        println(" | \\ | || | | |  |  _ \\ | |    |_ _|| \\ | | ");
        println(" |  \\| || | | |  | |_) || |     | | |  \\| | ");
        println(" | |\\  || |_| |  |  __/ | |___  | | | |\\  | ");
        println(" |_| \\_| \\___/   |_|    |_____| |___||_| \\_| ");
        println("                ___   ___  ___   ");
        println("               | _ \\ | _ \\| _ \\  ");
        println("               |  _/ |  _/|  _/  ");
        println("               |_|   |_|  |_|    ");

        SetColor(Color::PURPLE);
        println("========================================================");
        println("                 免解BL ROOT 工具");
        println("========================================================");
        ResetColor();

        println("");
        println("  [1] 设置SELinux宽容");
        println("  [2] 安装ROOT权限");
        println("  [3] 退出程序");
        println("");
        SetColor(Color::GRAY);
        cout << "请选择功能 >> ";
        ResetColor();

        string s;
        getline(cin, s);

        if (s == "1") { if (Check1()) Func1_SetSELinux(); }
        if (s == "2") { if (Check2()) Func2_InstallRoot(); }
        if (s == "3") { KillAdbFastboot(); break; }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("免解BL ROOT工具 | 无需解锁BL直接ROOT");
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    AutoSetupADB();
    Menu();
    return 0;
}
