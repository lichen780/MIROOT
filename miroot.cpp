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
#include <cstdlib>
#include <ctime>

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

enum Color {
    RED = 12, GREEN = 10, YELLOW = 14, BLUE = 9,
    PURPLE = 13, CYAN = 11, WHITE = 15, GRAY = 8
};

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void SetColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void ResetColor() {
    SetConsoleTextAttribute(hConsole, 15);
}

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

void Title(const string& title) {
    system("cls");
    SetColor(PURPLE);
    printf("========================================================\n");
    SetColor(CYAN);
    printf("                  免解 BL ROOT 工具\n");
    SetColor(PURPLE);
    printf("========================================================\n\n");
    ResetColor();
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
        // 跳过无关行
        if (line.find("List of devices") != string::npos) continue;
        if (line.empty()) continue;
        
        // 只要检测到在线设备就返回真
        if (line.find("device") != string::npos && 
            line.find("offline") == string::npos) {
            return true;
        }
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

// 核心修改：友好提示 KernelSU 缺失，不再直接报错
bool Check2() {
    if (!fs::exists(ADB_EXE)) { ERR("缺少 ADB.exe"); return false; }
    if (!fs::exists(FASTBOOT_EXE)) { ERR("缺少 fastboot.exe"); return false; }
    
    // 找不到 KernelSU.apk，提示用户手动下载
    if (!fs::exists(ksum)) {
        Title("文件缺失提示");
        WARN("未检测到 KernelSU.apk 文件！");
        cout << endl;
        INFO("请自行下载 KernelSU.apk，并保存到软件当前目录");
        INFO("文件名必须为：KernelSU.apk");
        cout << endl;
        INFO("下载地址：https://github.com/tiann/KernelSU/releases");
        cout << endl;
        PressAnyKeyBack();
        return false;
    }
    return true;
}

bool Func1_SetSELinux() {
    Title("免解BL - 设置SELinux宽容模式");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("重启至 Fastboot 模式");
    // 重启到bootloader（Fastboot）
    Exec(ADB_EXE.string(), "reboot bootloader");
    
    // 【关键】等待用户确认手机已经进入Fastboot界面，再继续
    INFO("请等待手机完全进入 Fastboot 模式");
    INFO("确认进入后 → 按回车键继续执行命令");
    PressAnyKeyBack();

    // ==============================
    // 【核心修复】真正执行Fastboot命令
    // ==============================
    Loading("正在设置 SELinux 为宽容模式 (permissive)");
    // 执行你要的命令：正确完整命令
    Exec(FASTBOOT_EXE.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");

    Loading("正在重启手机系统");
    // 正确重启命令：fastboot continue
    Exec(FASTBOOT_EXE.string(), "continue");

    INFO("手机正在开机，开机完成后按回车");
    PressAnyKeyBack();

    OK("SELinux 宽容模式设置完成！");
    PressAnyKeyBack();
    return true;
}

bool Func2_InstallKernelSU() {
    Title("免解BL - 安装 KernelSU 管理器");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    if (IsKsuInstalled()) {
        WARN("检测到手机已安装 KernelSU 管理器！");
        SetColor(CYAN);
        printf("\n是否覆盖安装？[Y] 覆盖 / [N] 取消：");
        ResetColor();

        string choice;
        getline(cin, choice);
        if (choice != "Y" && choice != "y") {
            INFO("已取消安装");
            PressAnyKeyBack();
            return true;
        }
        INFO("准备覆盖安装 KernelSU 管理器");
    }

    Loading("安装 KernelSU 管理器");
    Exec(ADB_EXE.string(), format("push {} /data/local/tmp/KernelSU.apk", ksum.string()));
    Exec(ADB_EXE.string(), "shell pm install -r /data/local/tmp/KernelSU.apk");

    OK("KernelSU 管理器安装完成！请打开应用授权");
    PressAnyKeyBack();
    return true;
}

void DrawAnimatedMenu() {
    system("cls");
    srand((unsigned)time(NULL));

    int colors[] = { BLUE, GREEN, CYAN, PURPLE, WHITE };
    int c1 = colors[rand() % 5];
    int c2 = colors[rand() % 5];
    int c3 = colors[rand() % 5];

    SetColor(c1);
    printf("      ╔════════════════════════════════════════════════════╗\n");
    SetColor(c2);
    printf("      ║                                                    ║\n");
    SetColor(c3);
    printf("      ║        ██████╗  ██████╗  ██████╗ ████████╗         ║\n");
    SetColor(c1);
    printf("      ║        ██╔══██╗██╔═══██╗██╔═══██╗╚══██╔══╝         ║\n");
    SetColor(c2);
    printf("      ║        ██████╔╝██║   ██║██║   ██║   ██║            ║\n");
    SetColor(c3);
    printf("      ║        ██╔══██╗██║   ██║██║   ██║   ██║            ║\n");
    SetColor(c1);
    printf("      ║        ██║  ██║╚██████╔╝╚██████╔╝   ██║            ║\n");
    SetColor(c2);
    printf("      ║        ╚═╝  ╚═╝ ╚═════╝  ╚═════╝    ╚═╝            ║\n");
    SetColor(c3);
    printf("      ║                                                    ║\n");
    SetColor(c1);
    printf("      ║              免解锁BL ROOT工具                     ║\n");
    SetColor(c2);
    printf("      ║                                                    ║\n");
    SetColor(c3);
    printf("      ╚════════════════════════════════════════════════════╝\n");
    ResetColor();

    printf("\n");
    SetColor(WHITE);
    printf("      +------------------------------------------------------+\n");
    printf("      |                                                      |\n");
    SetColor(GREEN);
    printf("      |   [1]  设置 SELinux 宽容模式                         |\n");
    SetColor(YELLOW);
    printf("      |   [2]  安装 KernelSU 管理器                          |\n");
    SetColor(RED);
    printf("      |   [3]  退出程序                                      |\n");
    SetColor(WHITE);
    printf("      |                                                      |\n");
    printf("      +------------------------------------------------------+\n");
    printf("\n");

    SetColor(CYAN);
    printf("      > 请输入选项 [1-3]: ");
    ResetColor();
}

void Menu() {
    while (true) {
        DrawAnimatedMenu();
        string s;
        getline(cin, s);

        if (s == "1") { if (Check1()) Func1_SetSELinux(); }
        if (s == "2") { if (Check2()) Func2_InstallKernelSU(); }
        if (s == "3") { KillAdbFastboot(); break; }
    }
}

int main() {
    system("chcp 65001 >nul");
    SetConsoleTitleW(L"\u514D\u89E3BL ROOT \u5DE5\u5177");
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    AutoSetupADB();
    Menu();
    return 0;
}
