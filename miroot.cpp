#include <windows.h>
#include <filesystem>
#include <iostream>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdio>
#include <limits>
#include <algorithm>
#include <sstream>
#include <string>
#include <urlmon.h>
#include <shellapi.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

using namespace std;
namespace fs = std::filesystem;

const fs::path cwd = fs::current_path();
const fs::path ADB_DIR = cwd / "adb";
const fs::path ADB_EXE = ADB_DIR / "adb.exe";
const fs::path FASTBOOT_EXE = ADB_DIR / "fastboot.exe";

const string ADB_URL = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";
const string ZIP_FILE = "platform-tools.zip";

fs::path ksum = cwd / "KernelSU.apk";

// 颜色定义
enum Color {
    RED = 12, GREEN = 10, YELLOW = 14, BLUE = 9,
    PURPLE = 13, CYAN = 11, WHITE = 15, GRAY = 8,
    BLACK = 0
};

// 全局变量用于控制光标位置
HANDLE hOut;
CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

// 设置光标位置
void SetCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hOut, coord);
}

// 设置文字颜色
void SetColor(int color) {
    SetConsoleTextAttribute(hOut, color);
}

void ResetColor() {
    SetConsoleTextAttribute(hOut, 15);
}

// 绘制像素风格边框
void DrawBorder() {
    // 获取窗口大小
    GetConsoleScreenBufferInfo(hOut, &csbiInfo);
    int screenWidth = csbiInfo.srWindow.Right - csbiInfo.srWindow.Left + 1;
    int screenHeight = csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top + 1;

    // 外框大小
    int boxWidth = 60;
    int boxHeight = 15;
    int startX = (screenWidth - boxWidth) / 2;
    int startY = (screenHeight - boxHeight) / 2 - 3; // 上移一点

    SetColor(CYAN);
    // 绘制外框
    for (int i = 0; i < boxWidth; i++) {
        SetCursorPosition(startX + i, startY);
        printf("-");
        SetCursorPosition(startX + i, startY + boxHeight - 1);
        printf("-");
    }
    for (int i = 0; i < boxHeight; i++) {
        SetCursorPosition(startX, startY + i);
        printf("|");
        SetCursorPosition(startX + boxWidth - 1, startY + i);
        printf("|");
    }
    // 绘制四角
    SetCursorPosition(startX, startY); printf("+");
    SetCursorPosition(startX + boxWidth - 1, startY); printf("+");
    SetCursorPosition(startX, startY + boxHeight - 1); printf("+");
    SetCursorPosition(startX + boxWidth - 1, startY + boxHeight - 1); printf("+");
    
    ResetColor();
}

// 绘制带特效的ROOT文字
void DrawRootText() {
    GetConsoleScreenBufferInfo(hOut, &csbiInfo);
    int screenWidth = csbiInfo.srWindow.Right - csbiInfo.srWindow.Left + 1;
    int startX = (screenWidth - 40) / 2; // 文字居中
    int startY = 2;

    // 随机颜色数组
    int colors[] = { CYAN, GREEN, BLUE, WHITE, PURPLE };
    int colorCount = 5;

    // 模拟扫描线/发光效果
    static int tick = 0;
    tick++;
    
    string rootText = "          ██████╗  ██████╗  ██████╗ ████████╗        ";
    string rootText2 = "          ██╔══██╗██╔═══██╗██╔═══██╗╚══██╔══╝        ";
    string rootText3 = "          ██████╔╝██║   ██║██║   ██║   ██║           ";
    string rootText4 = "          ██╔══██╗██║   ██║██║   ██║   ██║           ";
    string rootText5 = "          ██║  ██║╚██████╔╝╚██████╔╝   ██║           ";
    string rootText6 = "          ╚═╝  ╚═╝ ╚═════╝  ╚═════╝    ╚═╝           ";

    // 每帧随机改变颜色
    int c1 = colors[rand() % colorCount];
    int c2 = colors[rand() % colorCount];
    int c3 = colors[rand() % colorCount];

    // 动态闪烁效果
    if (tick % 2 == 0) {
        SetColor(c1); SetCursorPosition(startX, startY); printf("%s\n", rootText.c_str());
        SetColor(c2); SetCursorPosition(startX, startY+1); printf("%s\n", rootText2.c_str());
        SetColor(c3); SetCursorPosition(startX, startY+2); printf("%s\n", rootText3.c_str());
        SetColor(c1); SetCursorPosition(startX, startY+3); printf("%s\n", rootText4.c_str());
        SetColor(c2); SetCursorPosition(startX, startY+4); printf("%s\n", rootText5.c_str());
        SetColor(c3); SetCursorPosition(startX, startY+5); printf("%s\n", rootText6.c_str());
    } else {
        SetColor(WHITE); SetCursorPosition(startX, startY); printf("%s\n", rootText.c_str());
        SetColor(CYAN); SetCursorPosition(startX, startY+1); printf("%s\n", rootText2.c_str());
        SetColor(GREEN); SetCursorPosition(startX, startY+2); printf("%s\n", rootText3.c_str());
        SetColor(WHITE); SetCursorPosition(startX, startY+3); printf("%s\n", rootText4.c_str());
        SetColor(CYAN); SetCursorPosition(startX, startY+4); printf("%s\n", rootText5.c_str());
        SetColor(GREEN); SetCursorPosition(startX, startY+5); printf("%s\n", rootText6.c_str());
    }

    // 副标题
    SetColor(CYAN);
    SetCursorPosition((screenWidth - 20) / 2, startY + 7);
    printf("免解锁BL ROOT工具\n");
    
    ResetColor();
}

// 加载动画
void Loading(const string& text) {
    SetColor(CYAN);
    printf("%s ", text.c_str());
    const char ch[] = "|/-\\";
    for (int i = 0; i < 12; i++) {
        printf("\b%c", ch[i % 4]);
        fflush(stdout);
        Sleep(90);
    }
    printf("\b✓\n");
    ResetColor();
}

// 标题界面（包含边框和文字）
void Title(const string& title) {
    system("cls");
    // 重新获取窗口大小
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DrawBorder();
    DrawRootText();
}

void OK(const string& msg) {
    SetColor(GREEN);
    printf("✅ %s\n", msg.c_str());
    ResetColor();
}

void ERR(const string& msg) {
    SetColor(RED);
    printf("❌ %s\n", msg.c_str());
    ResetColor();
}

void INFO(const string& msg) {
    SetColor(BLUE);
    printf("ℹ️  %s\n", msg.c_str());
    ResetColor();
}

void WARN(const string& msg) {
    SetColor(YELLOW);
    printf("⚠️  %s\n", msg.c_str());
    ResetColor();
}

void PressAnyKeyBack() {
    SetColor(GRAY);
    printf("\n执行完成！按回车键返回主菜单...");
    cin.ignore((std::numeric_limits<streamsize>::max)(), '\n');
    cin.get();
    ResetColor();
}

static tuple<int, string> Exec(const string& bin, const string& args) {
    string cmd = format("\"{}\" {} 2>nul", bin, args);
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return { -1, "" };

    char buf[1024] = { 0 };
    string out;
    while (fgets(buf, sizeof(buf), pipe)) out += buf;
    int code = _pclose(pipe);
    return { code, out };
}

bool DownloadADB() {
    INFO("正在下载 ADB 工具包...");
    HRESULT res = URLDownloadToFileA(NULL, ADB_URL.c_str(), ZIP_FILE.c_str(), 0, NULL);
    if (res != S_OK) { ERR("下载失败！请检查网络"); return false; }
    OK("ADB 下载完成！");
    return true;
}

bool ExtractADB() {
    INFO("正在解压至 ADB 文件夹...");

    if (!fs::exists(ADB_DIR)) fs::create_directory(ADB_DIR);

    string cmd = "powershell -NoProfile -Command \"$ProgressPreference = 'SilentlyContinue'; Expand-Archive -Path '" + ZIP_FILE + "' -DestinationPath '" + ADB_DIR.string() + "' -Force\" 2>nul";
    system(cmd.c_str());
    Sleep(6000);

    fs::path extracted = ADB_DIR / "platform-tools";
    if (fs::exists(extracted)) {
        for (auto& f : fs::directory_iterator(extracted)) {
            fs::path dest = ADB_DIR / f.path().filename();
            if (fs::exists(dest)) fs::remove(dest);
            fs::rename(f.path(), dest);
        }
        fs::remove_all(extracted);
    }

    OK("ADB 解压完成！");
    return true;
}

void AutoSetupADB() {
    if (fs::exists(ADB_EXE)) { OK("ADB 工具已存在"); return; }
    WARN("未检测到 ADB 工具，自动部署中...");
    if (DownloadADB() && ExtractADB()) {
        fs::remove(ZIP_FILE);
        OK("ADB 部署完成！");
    }
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
        size_t sp = line.find(" ");
        if (sp == string::npos) continue;
        string serial = line.substr(0, sp);
        string status = line.substr(sp);
        if (serial.size() >= 10 && status.find("device") != string::npos) return true;
    }
    return false;
}

void WaitForDeviceLoop() {
    INFO("等待设备连接，请开启USB调试...");
    while (true) {
        if (CheckDeviceSerial()) { OK("设备已成功连接！"); break; }
        Sleep(3000);
    }
}

void ShowDeviceInfo() {
    Loading("正在获取手机信息");

    auto [_, brand] = Exec(ADB_EXE.string(), "shell getprop ro.product.brand");
    auto [__, model] = Exec(ADB_EXE.string(), "shell getprop ro.product.model");
    auto [___, android] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.release");
    auto [____, cpu] = Exec(ADB_EXE.string(), "shell getprop ro.product.board");
    auto [_____, sdk] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.sdk");
    auto [______, kernel] = Exec(ADB_EXE.string(), "shell uname -r");
    auto [_______, abi] = Exec(ADB_EXE.string(), "shell getprop ro.product.cpu.abi");
    auto [________, device] = Exec(ADB_EXE.string(), "shell getprop ro.product.device");
    auto [_________, patch] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.security_patch");

    brand.erase(remove_if(brand.begin(), brand.end(), ::isspace), brand.end());
    model.erase(remove_if(model.begin(), model.end(), ::isspace), model.end());
    android.erase(remove_if(android.begin(), android.end(), ::isspace), android.end());
    cpu.erase(remove_if(cpu.begin(), cpu.end(), ::isspace), cpu.end());
    sdk.erase(remove_if(sdk.begin(), sdk.end(), ::isspace), sdk.end());
    abi.erase(remove_if(abi.begin(), abi.end(), ::isspace), abi.end());
    device.erase(remove_if(device.begin(), device.end(), ::isspace), device.end());
    patch.erase(remove_if(patch.begin(), patch.end(), ::isspace), patch.end());

    kernel.erase(remove_if(kernel.begin(), kernel.end(), [](int c) {
        return c == '\n' || c == '\r';
    }), kernel.end());

    SetColor(YELLOW);
    printf("📱 手机品牌：%s\n", brand.c_str());
    printf("📱 手机型号：%s\n", model.c_str());
    printf("🔧 产品代号：%s\n", device.c_str());
    printf("🤖 安卓版本：%s (API %s)\n", android.c_str(), sdk.c_str());
    printf("⚙️ 处理器：%s\n", cpu.c_str());
    printf("🧩 CPU 架构：%s\n", abi.c_str());
    printf("🧠 内核版本：%s\n", kernel.c_str());
    printf("🛡️ 安全补丁：%s\n", patch.c_str());
    printf("\n");
    ResetColor();
}

bool IsKsuInstalled() {
    auto [code, _] = Exec(ADB_EXE.string(), "shell pm list packages | findstr me.weishu.kernelsu");
    return code == 0;
}

bool Check1() {
    if (!fs::exists(ADB_EXE)) { ERR("缺少 ADB.exe"); return false; }
    if (!fs::exists(FASTBOOT_EXE)) { ERR("缺少 fastboot.exe"); return false; }
    return true;
}

bool Check2() {
    if (!fs::exists(ADB_EXE)) { ERR("缺少 ADB.exe"); return false; }
    if (!fs::exists(FASTBOOT_EXE)) { ERR("缺少 fastboot.exe"); return false; }
    if (!fs::exists(ksum)) { ERR("缺少 KernelSU.apk 文件"); return false; }
    return true;
}

bool Func1_SetSELinux() {
    Title("免解BL - 设置SELinux宽容模式");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("重启至 Fastboot");
    Exec(ADB_EXE.string(), "reboot bootloader");
    INFO("进入 Fastboot 后按回车");
    PressAnyKeyBack();

    Loading("设置 SELinux 为宽容");
    Exec(FASTBOOT_EXE.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive
