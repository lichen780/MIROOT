#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <filesystem>
#include <tuple>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

using namespace std;
namespace fs = std::filesystem;

const fs::path cwd = fs::current_path();
const fs::path ADB_DIR = cwd / "adb";
const fs::path ADB_EXE = ADB_DIR / "adb.exe";
const fs::path FASTBOOT_EXE = ADB_DIR / "fastboot.exe";

const char* ADB_URL = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";
const char* ZIP_FILE = "platform-tools.zip";

fs::path ksum = cwd / "ksu.apk";
fs::path ksud = cwd / "ksud";

void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void OK(const char* msg) {
    SetColor(10);
    printf("✅ %s\n", msg);
    SetColor(15);
}

void INFO(const char* msg) {
    SetColor(9);
    printf("ℹ️  %s\n", msg);
    SetColor(15);
}

void WARN(const char* msg) {
    SetColor(14);
    printf("⚠️  %s\n", msg);
    SetColor(15);
}

void ERR(const char* msg) {
    SetColor(12);
    printf("❌ %s\n", msg);
    SetColor(15);
}

void PressAnyKeyBack() {
    printf("\n执行完成！按回车键返回主菜单...");
    while (getchar() != '\n');
    getchar();
}

void DownloadADB() {
    INFO("正在下载 ADB 工具包...");
    HRESULT r = URLDownloadToFileA(NULL, ADB_URL, ZIP_FILE, 0, NULL);
    if (r == S_OK) OK("ADB 下载完成！");
    else ERR("下载失败！");
}

void ExtractADB() {
    INFO("正在解压至 adb 文件夹...");
    if (!fs::exists(ADB_DIR)) fs::create_directory(ADB_DIR);
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\" >nul 2>&1",
        ZIP_FILE, ADB_DIR.string().c_str());
    system(cmd);
    Sleep(5000);

    fs::path src = ADB_DIR / "platform-tools";
    if (fs::exists(src)) {
        for (auto& f : fs::directory_iterator(src)) {
            fs::path d = ADB_DIR / f.path().filename();
            if (fs::exists(d)) fs::remove(d);
            fs::rename(f.path(), d);
        }
        fs::remove_all(src);
    }
    OK("ADB 解压完成！");
}

void InitADB() {
    if (fs::exists(ADB_EXE)) {
        OK("adb 工具已存在");
        return;
    }
    WARN("未检测到 adb 文件夹，自动部署中...");
    DownloadADB();
    ExtractADB();
    if (fs::exists(ZIP_FILE)) fs::remove(ZIP_FILE);
    OK("ADB 部署完成！");
}

bool CheckDevice() {
    char buf[2048];
    FILE* f = _popen(("\"" + ADB_EXE.string() + "\" devices 2>&1").c_str(), "r");
    if (!f) return false;
    bool ok = false;
    while (fgets(buf, sizeof(buf), f)) {
        string s = buf;
        if (s.find("device") != string::npos && s.size() > 20) ok = true;
    }
    _pclose(f);
    return ok;
}

void WaitDevice() {
    INFO("等待设备连接，请开启USB调试...");
    while (!CheckDevice()) Sleep(1000);
    OK("设备已连接！");
}

void Menu();

void Func1() {
    system("cls");
    INFO("=== 设置 SELinux ===");
    WaitDevice();
    system(("\"" + ADB_EXE.string() + "\" reboot bootloader").c_str());
    INFO("重启到 Fastboot，请按回车继续");
    getchar();
    system(("\"" + FASTBOOT_EXE.string() + "\" oem set-gpu-preemption 0 androidboot.selinux=permissive").c_str());
    system(("\"" + FASTBOOT_EXE.string() + "\" continue").c_str());
    OK("设置完成！");
    PressAnyKeyBack();
    Menu();
}

void Func2() {
    system("cls");
    INFO("=== 安装 ROOT ===");
    WaitDevice();
    system(("\"" + ADB_EXE.string() + "\" push \"" + ksud.string() + "\" /data/local/tmp/ksud").c_str());
    system(("\"" + ADB_EXE.string() + "\" shell chmod 755 /data/local/tmp/ksud").c_str());
    system(("\"" + ADB_EXE.string() + "\" shell service call miui.mqsas.IMQSNative 21 i32 1 s16 '/data/local/tmp/ksud' i32 1 s16 'late-load' i32 60").c_str());
    system(("\"" + ADB_EXE.string() + "\" push \"" + ksum.string() + "\" /data/local/tmp/ksu.apk").c_str());
    system(("\"" + ADB_EXE.string() + "\" shell pm install -r /data/local/tmp/ksu.apk").c_str());
    OK("ROOT 安装完成！");
    PressAnyKeyBack();
    Menu();
}

void Menu() {
    system("cls");
    printf("=========================================\n");
    printf("          免解BL ROOT 工具\n");
    printf("=========================================\n\n");
    printf("  1. 设置 SELinux 宽容\n");
    printf("  2. 安装 ROOT 权限\n");
    printf("  3. 退出\n\n");
    printf("请选择：");
    
    char c = getchar();
    if (c == '1') Func1();
    if (c == '2') Func2();
    if (c == '3') exit(0);
}

BOOL WINAPI Handler(DWORD s) {
    if (s == CTRL_CLOSE_EVENT) {
        system(("taskkill /f /im adb.exe fastboot.exe >nul 2>&1"));
    }
    return TRUE;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("免解BL ROOT 工具");  // ✅ 正常中文标题，不乱码
    SetConsoleCtrlHandler(Handler, TRUE);
    InitADB();
    Menu();
    return 0;
}
