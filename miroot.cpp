#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <atomic>
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
const string GBL_EFI_URL = "https://gh-proxy.org/https://github.com/lichen780/MIROOT/raw/main/gbl_efi_unlock.efi";
const string KSU_URL = "https://gh-proxy.org/https://github.com/lichen780/MIROOT/raw/main/KernelSU.apk";

fs::path ksum = cwd / "KernelSU.apk";

const fs::path UNLOCK_8E5_DIR = cwd / "8e5-unlock";
fs::path gbl_efi = UNLOCK_8E5_DIR / "gbl_efi_unlock.efi";
const fs::path UNLOCK_8E_DIR = cwd / "8e-unlock";
const fs::path UNLOCK_8G3_DIR = cwd / "8g3-unlock";
const fs::path ENNEA_IMG = UNLOCK_8G3_DIR / "8gen3-Ennea.img";

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
    printf("      ╔═════════════════════════════════════════════════════════╗\n");
    SetColor(CYAN);
    printf("      ║                 MI ROOT  小米解锁工具                  ║\n");
    SetColor(PURPLE);
    printf("      ╚═════════════════════════════════════════════════════════╝\n");
    ResetColor();
    SetColor(YELLOW);
    printf("\n      >> %s\n\n", title.c_str());
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
    printf("\n执行完成！按回车键返回...");
    fflush(stdin);
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

bool DownloadGBLEFI() {
    INFO("正在下载 gbl_efi_unlock.efi 文件...");
    SetColor(CYAN);
    printf("下载源：gh-proxy.org\n");
    ResetColor();

    HRESULT res = URLDownloadToFileA(NULL, GBL_EFI_URL.c_str(), gbl_efi.string().c_str(), 0, NULL);
    if (res != S_OK) {
        ERR("下载失败！请检查网络连接");
        return false;
    }

    if (!fs::exists(gbl_efi) || fs::file_size(gbl_efi) == 0) {
        ERR("下载的文件无效或为空！");
        return false;
    }

    OK("gbl_efi_unlock.efi 下载完成！");
    return true;
}

bool DownloadKernelSU() {
    INFO("正在下载 KernelSU.apk 文件...");
    SetColor(CYAN);
    printf("下载源：gh-proxy.org\n");
    ResetColor();

    HRESULT res = URLDownloadToFileA(NULL, KSU_URL.c_str(), ksum.string().c_str(), 0, NULL);
    if (res != S_OK) {
        ERR("下载失败！请检查网络连接");
        return false;
    }

    if (!fs::exists(ksum) || fs::file_size(ksum) == 0) {
        ERR("下载的文件无效或为空！");
        return false;
    }

    OK("KernelSU.apk 下载完成！");
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
    system(format("\"{}\" devices >nul 2>&1", ADB_EXE.string()).c_str());
    auto [code, output] = Exec(ADB_EXE.string(), "devices");

    istringstream iss(output);
    string line;
    while (getline(iss, line)) {
        if (line.find("List of devices") != string::npos) continue;
        if (line.empty()) continue;
        if (line.find("device") != string::npos && line.find("offline") == string::npos) {
            return true;
        }
    }
    return false;
}

void WaitForDeviceLoop() {
    INFO("等待设备连接，请开启 USB 调试...");
    while (true) {
        if (CheckDeviceSerial()) {
            OK("设备已成功连接！");
            break;
        }
        Sleep(1000);
    }
}

void ShowDeviceInfo() {
    Loading("正在获取手机信息");

    auto [_c1, marketname] = Exec(ADB_EXE.string(), "shell getprop ro.product.marketname");
    auto [_c2, model] = Exec(ADB_EXE.string(), "shell getprop ro.product.model");
    auto [_c3, android] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.release");
    auto [_c4, sdk] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.sdk");
    auto [_c5, patch] = Exec(ADB_EXE.string(), "shell getprop ro.build.version.security_patch");
    auto [_c6, osVersion] = Exec(ADB_EXE.string(), "shell getprop ro.mi.os.version.incremental");
    auto [_c7, socModel] = Exec(ADB_EXE.string(), "shell getprop ro.soc.model");

    marketname.erase(remove_if(marketname.begin(), marketname.end(), ::isspace), marketname.end());
    model.erase(remove_if(model.begin(), model.end(), ::isspace), model.end());
    android.erase(remove_if(android.begin(), android.end(), ::isspace), android.end());
    sdk.erase(remove_if(sdk.begin(), sdk.end(), ::isspace), sdk.end());
    patch.erase(remove_if(patch.begin(), patch.end(), ::isspace), patch.end());
    osVersion.erase(remove_if(osVersion.begin(), osVersion.end(), ::isspace), osVersion.end());
    socModel.erase(remove_if(socModel.begin(), socModel.end(), ::isspace), socModel.end());

    SetColor(YELLOW);
    printf("📱 市场名称：%s\n", marketname.c_str());
    printf("📱 手机型号：%s\n", model.c_str());
    printf("⚙️ CPU 型号：%s\n", socModel.c_str());
    printf("🤖 安卓版本：%s\n", android.c_str());
    printf("🔧 SDK 版本：%s\n", sdk.c_str());
    printf("🛡️ 安全补丁：%s\n", patch.c_str());
    printf("📲 OS 版本：%s\n", osVersion.c_str());
    printf("\n");
    ResetColor();
}

bool IsKsuInstalled() {
    auto [code, _out] = Exec(ADB_EXE.string(), "shell pm list packages | findstr me.weishu.kernelsu");
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

    if (!fs::exists(ksum)) {
        Title("文件缺失提示");
        WARN("未检测到 KernelSU.apk 文件！");
        cout << endl;
        INFO("正在尝试自动下载...");
        cout << endl;

        if (DownloadKernelSU()) {
            OK("文件已准备就绪！");
            Sleep(1000);
            return true;
        } else {
            cout << endl;
            WARN("自动下载失败，请手动下载！");
            cout << endl;
            INFO("下载地址：");
            SetColor(CYAN);
            printf("https://github.com/tiann/KernelSU/releases\n");
            ResetColor();
            cout << endl;
            INFO("下载后请将文件保存到软件当前目录");
            INFO("文件名必须为：KernelSU.apk");
            cout << endl;
            PressAnyKeyBack();
            return false;
        }
    }
    return true;
}

bool Check3() {
    if (!fs::exists(ADB_EXE)) { ERR("缺少 ADB.exe"); return false; }
    if (!fs::exists(FASTBOOT_EXE)) { ERR("缺少 fastboot.exe"); return false; }

    if (!fs::exists(gbl_efi)) {
        Title("文件缺失提示");
        WARN("未检测到 gbl_efi_unlock.efi 文件！");
        cout << endl;
        INFO("正在尝试自动下载...");
        cout << endl;

        if (DownloadGBLEFI()) {
            OK("文件已准备就绪！");
            Sleep(1000);
            return true;
        } else {
            cout << endl;
            WARN("自动下载失败，请手动下载！");
            cout << endl;
            INFO("下载地址：");
            SetColor(CYAN);
            printf("https://github.com/lichen780/MIROOT/raw/main/gbl_efi_unlock.efi\n");
            ResetColor();
            cout << endl;
            INFO("下载后请将文件保存到 8e5-unlock 文件夹内");
            INFO("文件名必须为：gbl_efi_unlock.efi");
            cout << endl;
            PressAnyKeyBack();
            return false;
        }
    }
    return true;
}

bool WaitDeviceOnline(int timeoutSec) {
    INFO("等待设备重新连接... (最长等待 " + to_string(timeoutSec) + " 秒)");
    auto start = chrono::steady_clock::now();
    while (true) {
        auto now = chrono::steady_clock::now();
        auto sec = chrono::duration_cast<chrono::seconds>(now - start).count();
        if (sec >= timeoutSec) {
            ERR("⏰ 等待设备超时 (" + to_string(timeoutSec) + " 秒)！");
            return false;
        }
        if (CheckDeviceSerial()) {
            OK("设备已重新上线！");
            Sleep(800);
            return true;
        }
        Sleep(800);
    }
}

bool WaitFastbootReady(int retries) {
    for (int i = 0; i < retries; i++) {
        auto [code, out] = Exec(FASTBOOT_EXE.string(), "devices");
        if (out.find("fastboot") != string::npos) return true;
        Sleep(1000);
        SetColor(GRAY);
        printf("Fastboot 等待中... [%d/%d]\r", i + 1, retries);
        ResetColor();
    }
    printf("\n");
    return false;
}

bool FlashAblViaMqsas(const fs::path& ablPath) {
    INFO("正在通过系统服务刷写 ABL 分区...");

    string pushCmd = format("push {} /data/local/tmp/abl", ablPath.string());
    auto [pushCode, pushOut] = Exec(ADB_EXE.string(), pushCmd);
    if (pushCode != 0) {
        ERR("推送 ABL 文件失败！");
        return false;
    }
    OK("ABL 文件推送完成！");

    Loading("刷写 abl_a 分区");
    auto [c1, o1] = Exec(ADB_EXE.string(),
        "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 \"dd\" i32 1 s16 'if=/data/local/tmp/abl of=/dev/block/by-name/abl_a' s16 '/data/mqsas/log.txt' i32 60");

    Loading("刷写 abl_b 分区");
    auto [c2, o2] = Exec(ADB_EXE.string(),
        "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 \"dd\" i32 1 s16 'if=/data/local/tmp/abl of=/dev/block/by-name/abl_b' s16 '/data/mqsas/log.txt' i32 60");

    OK("ABL 分区刷写完成！");
    return true;
}

bool RunBatScript(const fs::path& batPath, const string& label) {
    INFO("正在执行 " + label + "...");
    SetColor(GRAY);
    printf("脚本路径：%s\n", batPath.string().c_str());
    ResetColor();

    string cmd = format("cmd /c \"cd /d {} && {}\"", batPath.parent_path().string(), batPath.string());
    int ret = system(cmd.c_str());

    if (ret != 0) {
        WARN(label + " 执行返回码：" + to_string(ret));
        return false;
    }
    OK(label + " 执行完成！");
    return true;
}

// 带超时的 fastboot 命令执行（防止某些 USB 驱动下命令无限挂起）
static atomic<int> g_fbCounter{0};

static tuple<int, string> ExecFastboot(const string& args, int timeoutSec = 15) {
    string fastbootPath = FASTBOOT_EXE.string();
    string cmdline = format("\"{}\" {}", fastbootPath, args);

    int counter = g_fbCounter++;
    string tmpFile = format("_fb_{}_{}.tmp", GetCurrentProcessId(), counter);
    string wrapper = format("cmd /c \"{}\" > {} 2>&1", cmdline, tmpFile);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessA(NULL, wrapper.data(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        remove(tmpFile.c_str());
        return { -1, "" };
    }

    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutSec * 1000);

    if (waitResult == WAIT_TIMEOUT) {
        // 用 taskkill /T 杀掉整个进程树（包括孙进程 fastboot.exe）
        string killCmd = format("taskkill /T /F /PID {} >nul 2>&1", pi.dwProcessId);
        system(killCmd.c_str());
        WaitForSingleObject(pi.hProcess, 2000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        string output;
        FILE* f = fopen(tmpFile.c_str(), "r");
        if (f) { char buf[1024]; while (fgets(buf, sizeof(buf), f)) output += buf; fclose(f); }
        remove(tmpFile.c_str());
        return { -2, output };
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    string output;
    FILE* f = fopen(tmpFile.c_str(), "r");
    if (f) { char buf[1024]; while (fgets(buf, sizeof(buf), f)) output += buf; fclose(f); }
    remove(tmpFile.c_str());

    return { (int)exitCode, output };
}

// 带实时反馈的 fastboot 执行（解决用户看不到进度的问题）
static tuple<int, string> ExecFastbootWithFeedback(const string& args, int timeoutSec = 15) {
    SetColor(GRAY);
    printf("      ");
    ResetColor();

    // 启动 fastboot 子线程
    atomic<bool> done(false);
    atomic<int> resultCode(0);
    string resultOutput;

    thread worker([&]() {
        auto [code, out] = ExecFastboot(args, timeoutSec);
        resultCode = code;
        resultOutput = out;
        done = true;
    });

    // 主线程显示旋转动画
    const char spinner[] = "|/-\\";
    int i = 0;
    while (!done) {
        SetColor(CYAN);
        printf("\r      %c  正在执行...", spinner[i % 4]);
        ResetColor();
        fflush(stdout);
        Sleep(200);
        i++;
    }
    worker.join();

    // 清除旋转动画行
    printf("\r                                       \r");
    fflush(stdout);

    return { resultCode.load(), resultOutput };
}

// ==================== 设备自动识别 ====================

string GetDeviceCodename() {
    auto [code, output] = Exec(ADB_EXE.string(), "shell getprop ro.product.device");
    output.erase(remove_if(output.begin(), output.end(), ::isspace), output.end());
    return output;
}

string GetDeviceMarketName() {
    auto [code, output] = Exec(ADB_EXE.string(), "shell getprop ro.product.marketname");
    output.erase(remove_if(output.begin(), output.end(), ::isspace), output.end());
    return output;
}

// ==================== 设备机型定义 ====================

struct DeviceModel_8E {
    const char* name;
    const char* folder;
    const char* boot_folder;
    const char* codename;
};

const DeviceModel_8E MODELS_8E[] = {
    {"Redmi K80 Pro",       "Redmik80pro",    "Phone",   "vermeer_apollo"},
    {"Redmi K90",           "Redmik90",       "Phone",   "dada"},
    {"Xiaomi 15",           "Xiaomi15",       "Phone",   "dada"},
    {"Xiaomi 15 Pro",       "Xiaomi15pro",    "Phone",   "haotian"},
    {"Xiaomi 15 Ultra",     "Xiaomi15ultra",  "Phone",   "shennong"},
    {"Xiaomi Pad 8 Pro",    "Xiaomipad8pro",  "Tablet",  "pandora"},
};
const int MODELS_8E_COUNT = sizeof(MODELS_8E) / sizeof(MODELS_8E[0]);

struct DeviceModel_8G3 {
    const char* name;
    const char* folder;
    const char* codename;
};

const DeviceModel_8G3 MODELS_8G3[] = {
    {"Redmi K70 Pro",       "Redmik70pro",     "shennong"},
    {"Redmi K80",           "Redmik80",        "zhenniao"},
    {"Xiaomi 14",           "Xiaomi14",        "houji"},
    {"Xiaomi 14 Pro",       "Xiaomi14pro",     "shennong"},
    {"Xiaomi 14 Ultra",     "Xiaomi14ultra",   "aurora"},
    {"Xiaomi MIX Flip",     "Xiaomimixflip",   "ruyi"},
    {"Xiaomi MIX Fold4",    "Xiaomimixfold4",  "goku"},
};
const int MODELS_8G3_COUNT = sizeof(MODELS_8G3) / sizeof(MODELS_8G3[0]);

int AutoDetectModel_8E() {
    string codename = GetDeviceCodename();
    string marketname = GetDeviceMarketName();
    if (codename.empty()) return -1;

    // 先收集所有匹配 codename 的候选
    vector<int> candidates;
    for (int i = 0; i < MODELS_8E_COUNT; i++) {
        if (codename == MODELS_8E[i].codename) candidates.push_back(i);
    }
    if (candidates.empty()) return -1;
    if (candidates.size() == 1) return candidates[0];

    // 多个候选时用 marketname 辅助判断
    string mLower = marketname;
    transform(mLower.begin(), mLower.end(), mLower.begin(), ::tolower);
    for (int idx : candidates) {
        string name = MODELS_8E[idx].name;
        transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (mLower.find(name) != string::npos) return idx;
    }
    return candidates[0]; // 无法区分时返回第一个
}

int AutoDetectModel_8G3() {
    string codename = GetDeviceCodename();
    string marketname = GetDeviceMarketName();
    if (codename.empty()) return -1;

    vector<int> candidates;
    for (int i = 0; i < MODELS_8G3_COUNT; i++) {
        if (codename == MODELS_8G3[i].codename) candidates.push_back(i);
    }
    if (candidates.empty()) return -1;
    if (candidates.size() == 1) return candidates[0];

    string mLower = marketname;
    transform(mLower.begin(), mLower.end(), mLower.begin(), ::tolower);
    for (int idx : candidates) {
        string name = MODELS_8G3[idx].name;
        transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (mLower.find(name) != string::npos) return idx;
    }
    return candidates[0];
}

// ==================== 功能 1: 免解 BL ROOT ====================

bool Func1_SetSELinux() {
    Title("免解 BL - 设置 SELinux 宽容模式");

    bool alreadyInFastboot = false;
    auto [fbCode, fbOut] = Exec(FASTBOOT_EXE.string(), "devices");
    if (fbOut.find("fastboot") != string::npos) {
        alreadyInFastboot = true;
        OK("检测到手机已处于 Fastboot 模式，跳过重启！");
        Sleep(1500);
    }

    if (!alreadyInFastboot) {
        WaitForDeviceLoop();
        ShowDeviceInfo();

        Loading("重启至 Fastboot 模式");
        Exec(ADB_EXE.string(), "reboot bootloader");

        INFO("请等待手机完全进入 Fastboot 模式（米兔/机器人界面）");
        INFO("确认进入后 → 按回车键直接执行命令！");
        cin.get();
    }

    Loading("正在设置 SELinux 为宽容模式");
    auto [fbSetCode, fbSetOut] = ExecFastbootWithFeedback("oem set-gpu-preemption 0 androidboot.selinux=permissive", 15);
    if (fbSetCode == -2) {
        ERR("fastboot 命令超时！尝试 continue 回退...");
    }

    Loading("正在重启手机系统");
    auto [fbContCode, fbContOut] = ExecFastbootWithFeedback("continue", 10);
    if (fbContCode == -2) {
        WARN("fastboot continue 超时，尝试 reboot...");
        ExecFastbootWithFeedback("reboot", 10);
    }

    if (WaitDeviceOnline(30)) {
        Loading("正在检测 SELinux 模式");
        auto [_sc, selinux] = Exec(ADB_EXE.string(), "shell getenforce");

        selinux.erase(remove_if(selinux.begin(), selinux.end(), [](char c) {
            return c == '\n' || c == '\r' || c == ' ';
        }), selinux.end());

        if (selinux == "Permissive" || selinux == "permissive") {
            OK("✅ SELinux 已成功设置为宽容模式！");
        } else {
            WARN("⚠️ 当前 SELinux：" + selinux);
            ERR("❌ 设置未生效，请重试！");
        }
    }

    PressAnyKeyBack();
    return true;
}

bool Func2_InstallKernelSU() {
    Title("免解 BL - 安装 KernelSU 管理器");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    if (IsKsuInstalled()) {
        WARN("检测到手机已安装 KernelSU 管理器！");
        SetColor(CYAN);
        printf("\n是否覆盖安装？[Y] 覆盖 / [N] 取消：");
        ResetColor();

        string choice;
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
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

// ==================== 功能 2: 骁龙 8E5 解 BL 锁 ====================

bool Func3_UnlockBL_8E5() {
    Title("骁龙 8E5 - 解 BL 锁");

    INFO("本功能适用于骁龙 8E5 设备解锁 BL");
    INFO("操作前请确保：");
    SetColor(YELLOW);
    printf("  1. 手机 USB 调试已打开\n");
    printf("  2. 已始终允许该电脑使用 ADB\n");
    printf("  3. gbl_efi_unlock.efi 文件已放置在 8e5-unlock 文件夹内\n");
    ResetColor();
    cout << endl;
    INFO("确认以上条件后，按回车键开始操作...");
    cin.get();

    WaitForDeviceLoop();
    Loading("重启至 Fastboot 模式");
    Exec(ADB_EXE.string(), "reboot bootloader");

    INFO("请等待手机完全进入 Fastboot 模式（米兔/机器人界面）");
    INFO("确认进入后 → 按回车键继续！");
    cin.get();

    Loading("正在设置 SELinux 为宽容模式");
    auto [fbCode, fbOut] = ExecFastbootWithFeedback("oem set-gpu-preemption-value 0 androidboot.selinux=permissive", 15);
    if (fbCode == -2) {
        ERR("fastboot 命令超时！尝试 continue 回退...");
    } else if (fbCode != 0) {
        ERR("设置 SELinux 失败！");
        PressAnyKeyBack();
        return false;
    }
    OK("SELinux 设置完成！");

    Loading("正在重启手机系统");
    auto [fbContCode, fbContOut] = ExecFastbootWithFeedback("continue", 10);
    if (fbContCode == -2) {
        WARN("fastboot continue 超时，尝试 reboot...");
        ExecFastbootWithFeedback("reboot", 10);
    }

    if (!WaitDeviceOnline(30)) {
        ERR("设备未重新连接，无法继续操作！");
        PressAnyKeyBack();
        return false;
    }

    INFO("开始推送解锁文件到设备...");
    Loading("推送 gbl_efi_unlock.efi");
    auto [pushCode, pushOut] = Exec(ADB_EXE.string(), format("push {} /data/local/tmp/gbl_efi_unlock.efi", gbl_efi.string()));
    if (pushCode != 0) {
        ERR("推送文件失败！");
        PressAnyKeyBack();
        return false;
    }
    OK("文件推送完成！");

    INFO("正在执行解锁命令...");
    Loading("调用系统服务解锁 BL");
    auto [svcCode, svcOut] = Exec(ADB_EXE.string(),
        "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 \"dd\" i32 1 s16 'if=/data/local/tmp/gbl_efi_unlock.efi of=/dev/block/by-name/efisp' s16 '/data/mqsas/log.txt' i32 60");

    SetColor(WHITE);
    printf("\n命令执行结果：\n");
    printf("%s\n", svcOut.c_str());
    ResetColor();

    if (svcOut.find("Result: Parcel") == string::npos) {
        WARN("⚠️  返回结果异常！");
        ERR("解锁可能失败，请上报此问题！");
        cout << endl;
        INFO("按任意键继续...");
        cin.get();
    } else {
        OK("解锁命令执行成功！");
    }

    INFO("手机即将重启并进入 Fastboot 模式检查解锁状态...");
    Loading("重启至 Fastboot 模式");
    Exec(ADB_EXE.string(), "reboot bootloader");

    INFO("请等待手机进入 Fastboot 模式后按任意键检查 BL 状态...");
    cin.get();

    Loading("检查 BL 解锁状态");
    auto [unlockCode1, unlockOut1] = ExecFastbootWithFeedback("getvar unlocked", 10);
    auto [unlockCode2, unlockOut2] = ExecFastbootWithFeedback("getvar unlocked", 10);

    SetColor(WHITE);
    printf("\n");
    printf("解锁状态检查结果：\n");
    printf("%s\n", unlockOut1.c_str());
    printf("%s\n", unlockOut2.c_str());
    ResetColor();

    if (unlockOut1.find("unlocked") != string::npos || unlockOut2.find("unlocked") != string::npos) {
        OK("✅ BL 已成功解锁！");
    } else {
        WARN("⚠️  可能未解锁成功，请检查上方输出！");
    }

    PressAnyKeyBack();
    return true;
}

// ==================== 功能 3: 骁龙 8E 解 BL 锁 ====================

bool Func4_UnlockBL_8E() {
    Title("骁龙 8E - 解 BL 锁");

    INFO("正在自动识别设备...");
    WaitForDeviceLoop();

    int detected = AutoDetectModel_8E();
    string marketName = GetDeviceMarketName();
    int selected = -1;

    if (detected >= 0) {
        printf("\n");
        SetColor(WHITE);
        printf("      ┌─────────────────────────────────────────────────────┐\n");
        printf("      │                                                     │\n");
        SetColor(GREEN);
        printf("      │   已识别到设备: %-36s│\n", marketName.c_str());
        printf("      │   匹配机型:     %-36s│\n", MODELS_8E[detected].name);
        SetColor(WHITE);
        printf("      │                                                     │\n");
        printf("      └─────────────────────────────────────────────────────┘\n\n");

        SetColor(CYAN);
        printf("      确认使用此机型？ [Y] 确认 / [N] 手动选择 / [0] 返回: ");
        ResetColor();
        string c;
        cin >> c;
        cin.ignore();
        if (c == "0") return true;
        if (c == "Y" || c == "y") {
            selected = detected;
        }
    }

    if (selected < 0) {
        printf("\n");
        SetColor(WHITE);
        printf("      ┌─────────────────────────────────────────────────────┐\n");
        printf("      │                                                     │\n");
        SetColor(CYAN);
        printf("      │          请选择你的设备型号                          │\n");
        SetColor(WHITE);
        printf("      │                                                     │\n");
        printf("      │─────────────────────────────────────────────────────│\n");
        for (int i = 0; i < MODELS_8E_COUNT; i++) {
            if (i == detected) {
                SetColor(GREEN);
                printf("      │   [%d]  %-43s │\n", i + 1, MODELS_8E[i].name);
            } else {
                SetColor(YELLOW);
                printf("      │   [%d]  %-43s │\n", i + 1, MODELS_8E[i].name);
            }
        }
        SetColor(RED);
        printf("      │   [0]  返回主菜单                                  │\n");
        SetColor(WHITE);
        printf("      │                                                     │\n");
        printf("      └─────────────────────────────────────────────────────┘\n\n");

        SetColor(CYAN);
        printf("      > 请输入选项 [0-%d]: ", MODELS_8E_COUNT);
        ResetColor();

        string choiceStr;
        cin >> choiceStr;
        cin.ignore();

        int choice = -1;
        try { choice = stoi(choiceStr); } catch (...) {}

        if (choice == 0) return true;
        if (choice < 1 || choice > MODELS_8E_COUNT) {
            ERR("无效选项！");
            Sleep(1000);
            return false;
        }
        selected = choice - 1;
    }

    const DeviceModel_8E& model = MODELS_8E[selected];
    fs::path modelDir = UNLOCK_8E_DIR / "Xiaobao" / model.folder;
    fs::path ablPath = modelDir / "images" / "abl.elf";
    fs::path flashAllBat = modelDir / "flash_all.bat";
    fs::path flashAll1Bat = modelDir / "flash_all_1.bat";
    fs::path bootImg = UNLOCK_8E_DIR / "items" / model.boot_folder / "boot.img";
    fs::path gptBoth4 = UNLOCK_8E_DIR / "items" / model.boot_folder / "gpt_both4.bin";

    // 检查必要文件
    if (!fs::exists(ablPath)) { ERR("缺少 ABL 文件：" + ablPath.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(flashAllBat)) { ERR("缺少 flash_all.bat：" + flashAllBat.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(bootImg)) { ERR("缺少 boot.img：" + bootImg.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(gptBoth4)) { ERR("缺少 gpt_both4.bin：" + gptBoth4.string()); PressAnyKeyBack(); return false; }

    // 二次确认
    Title("骁龙 8E - 解 BL 锁");
    WARN("⚠️  解 BL 锁将清除所有数据且失去官方保修！");
    WARN("⚠️  手机变砖、数据丢失，脚本作者概不负责！");
    cout << endl;
    SetColor(YELLOW);
    printf("  选择的设备：%s\n", model.name);
    printf("  ABL 文件：%s\n", ablPath.string().c_str());
    ResetColor();
    cout << endl;

    SetColor(CYAN);
    printf("是否继续？(输入 Y 确认，其他取消): ");
    ResetColor();
    string confirm;
    cin >> confirm;
    cin.ignore();
    if (confirm != "Y" && confirm != "y") {
        INFO("已取消操作");
        PressAnyKeyBack();
        return true;
    }

    // 步骤 1: 等待 ADB 设备
    Title("骁龙 8E - 解 BL 锁 - 步骤 1/6");
    INFO("请连接手机并开启 USB 调试...");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    // 步骤 2: 重启至 Fastboot
    Title("骁龙 8E - 解 BL 锁 - 步骤 2/6");
    Loading("重启至 Fastboot 模式");
    Exec(ADB_EXE.string(), "reboot bootloader");
    Sleep(5000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 步骤 3: 设置 SELinux 宽容模式
    Title("骁龙 8E - 解 BL 锁 - 步骤 3/6");
    Loading("设置 SELinux 为宽容模式");
    ExecFastbootWithFeedback("oem set-gpu-preemption 0 androidboot.selinux=permissive", 15);
    auto [fbContCode_8e, _fb8e] = ExecFastbootWithFeedback("continue", 10);
    if (fbContCode_8e == -2) {
        WARN("fastboot continue 超时，尝试 reboot...");
        ExecFastbootWithFeedback("reboot", 10);
    }

    if (!WaitDeviceOnline(30)) {
        ERR("设备未重新连接！");
        PressAnyKeyBack();
        return false;
    }

    // 检查 SELinux
    auto [_sc, selinux] = Exec(ADB_EXE.string(), "shell getenforce");
    selinux.erase(remove_if(selinux.begin(), selinux.end(), ::isspace), selinux.end());
    if (selinux != "Permissive" && selinux != "permissive") {
        ERR("SELinux 未设置为宽容模式，当前：" + selinux);
        ERR("请重试！");
        PressAnyKeyBack();
        return false;
    }
    OK("SELinux 已确认为宽容模式！");

    // 步骤 4: 刷写 ABL
    Title("骁龙 8E - 解 BL 锁 - 步骤 4/6");
    if (!FlashAblViaMqsas(ablPath)) {
        PressAnyKeyBack();
        return false;
    }

    Exec(ADB_EXE.string(), "reboot bootloader");
    Sleep(5000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 步骤 5: 刷写固件
    Title("骁龙 8E - 解 BL 锁 - 步骤 5/6");
    INFO("正在刷写设备固件（flash_all.bat）...");
    if (!RunBatScript(flashAllBat, "固件刷写 (flash_all.bat)")) {
        WARN("刷写可能未完全成功，继续尝试...");
    }

    ExecFastbootWithFeedback("reboot bootloader", 15);
    Sleep(5000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 刷写 GPT 分区表 + 启动修改镜像
    INFO("正在刷写 GPT 分区表...");
    string gptCmd = format("flash partition:4 {}", gptBoth4.string());
    ExecFastbootWithFeedback(gptCmd, 30);

    Sleep(1000);
    INFO("正在启动修改镜像...");
    string bootCmd = format("boot {}", bootImg.string());
    ExecFastbootWithFeedback(bootCmd, 30);

    // 步骤 6: 恢复分区表 + 刷写剩余固件
    Title("骁龙 8E - 解 BL 锁 - 步骤 6/6");

    if (fs::exists(flashAll1Bat)) {
        INFO("正在恢复分区表并刷写剩余固件...");
        ExecFastbootWithFeedback("reboot bootloader", 15);
        Sleep(5000);

        if (!WaitFastbootReady(10)) {
            ERR("未检测到 Fastboot 设备！");
            PressAnyKeyBack();
            return false;
        }

        if (!RunBatScript(flashAll1Bat, "恢复刷写 (flash_all_1.bat)")) {
            WARN("恢复刷写可能未完全成功！");
        }
    }

    // 完成提示
    Title("骁龙 8E - 解 BL 锁 - 完成");
    OK("🎉 解锁流程已完成！");
    cout << endl;
    INFO("请到 www.xiaomirom.com 下载官方线刷包（请选择稳定版）");
    INFO("将官方线刷包内的 flash_all.bat 拖到此处执行完整刷机");
    INFO("或使用「小米官方刷机工具」等工具刷入官方包");
    cout << endl;

    SetColor(CYAN);
    printf("脚本执行完毕，当前已停留在 Fastboot 模式。\n");
    ResetColor();

    PressAnyKeyBack();
    return true;
}

// ==================== 功能 4: 骁龙 8G3 解 BL 锁 ====================

bool Func5_UnlockBL_8G3() {
    Title("骁龙 8G3 - 解 BL 锁");

    INFO("正在自动识别设备...");
    WaitForDeviceLoop();

    int detected = AutoDetectModel_8G3();
    string marketName = GetDeviceMarketName();
    int selected = -1;

    if (detected >= 0) {
        printf("\n");
        SetColor(WHITE);
        printf("      ┌─────────────────────────────────────────────────────┐\n");
        printf("      │                                                     │\n");
        SetColor(GREEN);
        printf("      │   已识别到设备: %-36s│\n", marketName.c_str());
        printf("      │   匹配机型:     %-36s│\n", MODELS_8G3[detected].name);
        SetColor(WHITE);
        printf("      │                                                     │\n");
        printf("      └─────────────────────────────────────────────────────┘\n\n");

        SetColor(CYAN);
        printf("      确认使用此机型？ [Y] 确认 / [N] 手动选择 / [0] 返回: ");
        ResetColor();
        string c;
        cin >> c;
        cin.ignore();
        if (c == "0") return true;
        if (c == "Y" || c == "y") {
            selected = detected;
        }
    }

    if (selected < 0) {
        printf("\n");
        SetColor(WHITE);
        printf("      ┌─────────────────────────────────────────────────────┐\n");
        printf("      │                                                     │\n");
        SetColor(CYAN);
        printf("      │          请选择你的设备型号                          │\n");
        SetColor(WHITE);
        printf("      │                                                     │\n");
        printf("      │─────────────────────────────────────────────────────│\n");
        for (int i = 0; i < MODELS_8G3_COUNT; i++) {
            if (i == detected) {
                SetColor(GREEN);
                printf("      │   [%d]  %-43s │\n", i + 1, MODELS_8G3[i].name);
            } else {
                SetColor(YELLOW);
                printf("      │   [%d]  %-43s │\n", i + 1, MODELS_8G3[i].name);
            }
        }
        SetColor(RED);
        printf("      │   [0]  返回主菜单                                  │\n");
        SetColor(WHITE);
        printf("      │                                                     │\n");
        printf("      └─────────────────────────────────────────────────────┘\n\n");

        SetColor(CYAN);
        printf("      > 请输入选项 [0-%d]: ", MODELS_8G3_COUNT);
        ResetColor();

        string choiceStr;
        cin >> choiceStr;
        cin.ignore();

        int choice = -1;
        try { choice = stoi(choiceStr); } catch (...) {}

        if (choice == 0) return true;
        if (choice < 1 || choice > MODELS_8G3_COUNT) {
            ERR("无效选项！");
            Sleep(1000);
            return false;
        }
        selected = choice - 1;
    }

    const DeviceModel_8G3& model = MODELS_8G3[selected];
    fs::path modelDir = UNLOCK_8G3_DIR / "Xiaobao" / model.folder;
    fs::path ablPath = modelDir / "images" / "abl.elf";
    fs::path flashAllBat = modelDir / "flash_all.bat";
    fs::path gptBoth4 = UNLOCK_8G3_DIR / "items" / model.folder / "gpt_both4.bin";
    fs::path blgptBoth4 = UNLOCK_8G3_DIR / "items" / model.folder / "blgpt_both4.bin";

    // 检查必要文件
    if (!fs::exists(ablPath)) { ERR("缺少 ABL 文件：" + ablPath.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(flashAllBat)) { ERR("缺少 flash_all.bat：" + flashAllBat.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(gptBoth4)) { ERR("缺少 gpt_both4.bin：" + gptBoth4.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(blgptBoth4)) { ERR("缺少 blgpt_both4.bin：" + blgptBoth4.string()); PressAnyKeyBack(); return false; }
    if (!fs::exists(ENNEA_IMG)) { ERR("缺少 8gen3-Ennea.img：" + ENNEA_IMG.string()); PressAnyKeyBack(); return false; }

    // 二次确认
    Title("骁龙 8G3 - 解 BL 锁");
    WARN("⚠️  解 BL 锁将清除所有数据且失去官方保修！");
    WARN("⚠️  手机变砖、数据丢失，脚本作者概不负责！");
    cout << endl;
    SetColor(YELLOW);
    printf("  选择的设备：%s\n", model.name);
    printf("  ABL 文件：%s\n", ablPath.string().c_str());
    ResetColor();
    cout << endl;

    SetColor(CYAN);
    printf("是否继续？(输入 Y 确认，其他取消): ");
    ResetColor();
    string confirm;
    cin >> confirm;
    cin.ignore();
    if (confirm != "Y" && confirm != "y") {
        INFO("已取消操作");
        PressAnyKeyBack();
        return true;
    }

    // 步骤 1: 等待 ADB 设备
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 1/8");
    INFO("请连接手机并开启 USB 调试...");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    // 步骤 2: 重启至 Fastboot
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 2/8");
    Loading("重启至 Fastboot 模式");
    Exec(ADB_EXE.string(), "reboot bootloader");
    Sleep(2000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 步骤 3: 设置 SELinux 宽容模式
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 3/8");
    Loading("设置 SELinux 为宽容模式");
    ExecFastbootWithFeedback("oem set-gpu-preemption 0 androidboot.selinux=permissive", 15);
    auto [fbContCode_8g3, _fb8g3] = ExecFastbootWithFeedback("continue", 10);
    if (fbContCode_8g3 == -2) {
        WARN("fastboot continue 超时，尝试 reboot...");
        ExecFastbootWithFeedback("reboot", 10);
    }

    if (!WaitDeviceOnline(30)) {
        ERR("设备未重新连接！");
        PressAnyKeyBack();
        return false;
    }

    // 检查 SELinux
    auto [_sc, selinux] = Exec(ADB_EXE.string(), "shell getenforce");
    selinux.erase(remove_if(selinux.begin(), selinux.end(), ::isspace), selinux.end());
    if (selinux != "Permissive" && selinux != "permissive") {
        ERR("SELinux 未设置为宽容模式，当前：" + selinux);
        PressAnyKeyBack();
        return false;
    }
    OK("SELinux 已确认为宽容模式！");

    // 步骤 4: 刷写 ABL
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 4/8");
    if (!FlashAblViaMqsas(ablPath)) {
        PressAnyKeyBack();
        return false;
    }

    Exec(ADB_EXE.string(), "reboot bootloader");
    Sleep(2000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 步骤 5: 刷写固件 (flash_all.bat)
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 5/8");
    INFO("正在刷写设备固件（flash_all.bat）...");
    if (!RunBatScript(flashAllBat, "固件刷写 (flash_all.bat)")) {
        WARN("刷写可能未完全成功，继续尝试...");
    }

    ExecFastbootWithFeedback("reboot bootloader", 15);
    Sleep(2000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 步骤 6: 刷写修改版 GPT 分区表
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 6/8");
    INFO("正在刷写修改版 GPT 分区表...");
    string blgptCmd = format("flash partition:4 {}", blgptBoth4.string());
    ExecFastbootWithFeedback(blgptCmd, 30);

    Sleep(1000);

    // 步骤 7: 启动修改镜像
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 7/8");
    INFO("正在启动修改镜像...");
    string enneaCmd = format("boot {}", ENNEA_IMG.string());
    ExecFastbootWithFeedback(enneaCmd, 30);

    Sleep(2000);

    ExecFastbootWithFeedback("reboot bootloader", 15);
    Sleep(2000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 步骤 8: 恢复原始 GPT 分区表
    Title("骁龙 8G3 - 解 BL 锁 - 步骤 8/8");
    INFO("正在恢复原始 GPT 分区表...");
    string gptCmd = format("flash partition:4 {}", gptBoth4.string());
    ExecFastbootWithFeedback(gptCmd, 30);

    ExecFastbootWithFeedback("reboot bootloader", 15);
    Sleep(2000);

    if (!WaitFastbootReady(10)) {
        ERR("未检测到 Fastboot 设备！");
        PressAnyKeyBack();
        return false;
    }

    // 检查解锁状态
    Title("骁龙 8G3 - 解 BL 锁 - 检查结果");
    Loading("检查 BL 解锁状态");

    auto [infoCode, infoOut] = ExecFastbootWithFeedback("oem device-info", 15);
    SetColor(WHITE);
    printf("\n%s\n", infoOut.c_str());
    ResetColor();

    bool blUnlocked = (infoOut.find("Device unlocked: true") != string::npos);
    if (blUnlocked) {
        OK("✅ BL 已成功解锁！");
    } else {
        WARN("⚠️  BL 可能未解锁，请检查上方输出！");
    }

    // 检查 FRP
    Loading("检查 FRP 状态");
    auto [frpCode, frpOut] = ExecFastbootWithFeedback("erase frp", 15);
    SetColor(WHITE);
    printf("%s\n", frpOut.c_str());
    ResetColor();

    bool frpOk = (frpOut.find("FAILED") == string::npos);
    if (frpOk) {
        OK("FRP 已擦除！");
    }

    if (blUnlocked && frpOk) {
        cout << endl;
        OK("🎉 解锁完全成功！");
    }

    // 完成提示
    cout << endl;
    INFO("请到 www.xiaomirom.com 下载官方线刷包（请选择稳定版）");
    INFO("将官方线刷包内的 flash_all.bat 拖到此处执行完整刷机");
    INFO("或使用「小米官方刷机工具」等工具刷入官方包");
    cout << endl;

    SetColor(CYAN);
    printf("脚本执行完毕，当前已停留在 Fastboot 模式。\n");
    ResetColor();

    PressAnyKeyBack();
    return true;
}

// ==================== 主菜单 ====================

void DrawMenuHeader() {
    SetColor(PURPLE);
    printf("      ╔═════════════════════════════════════════════════════════╗\n");
    SetColor(CYAN);
    printf("      ║                 MI ROOT  小米解锁工具                  ║\n");
    SetColor(PURPLE);
    printf("      ╚═════════════════════════════════════════════════════════╝\n");
    ResetColor();
}

void DrawAnimatedMenu() {
    system("cls");
    DrawMenuHeader();
    printf("\n");
    SetColor(WHITE);
    printf("      ┌─────────────────────────────────────────────────────┐\n");
    printf("      │                                                     │\n");
    SetColor(GREEN);
    printf("      │     [1]   免解 BL ROOT                              │\n");
    SetColor(YELLOW);
    printf("      │     [2]   骁龙 8E5 解 BL 锁                         │\n");
    SetColor(CYAN);
    printf("      │     [3]   骁龙 8E  解 BL 锁                         │\n");
    SetColor(PURPLE);
    printf("      │     [4]   骁龙 8G3 解 BL 锁                         │\n");
    printf("      │                                                     │\n");
    SetColor(RED);
    printf("      │     [0]   退出程序                                  │\n");
    SetColor(WHITE);
    printf("      │                                                     │\n");
    printf("      └─────────────────────────────────────────────────────┘\n");
    printf("\n");
    SetColor(CYAN);
    printf("      > 请输入选项: ");
    ResetColor();
}

void DrawSubmenu_NoUnlock() {
    system("cls");
    DrawMenuHeader();
    printf("\n");
    SetColor(WHITE);
    printf("      ┌─────────────────────────────────────────────────────┐\n");
    printf("      │                                                     │\n");
    SetColor(CYAN);
    printf("      │          免解 BL ROOT                               │\n");
    SetColor(WHITE);
    printf("      │─────────────────────────────────────────────────────│\n");
    printf("      │                                                     │\n");
    SetColor(GREEN);
    printf("      │     [1]   设置 SELinux 宽容模式                     │\n");
    SetColor(YELLOW);
    printf("      │     [2]   安装 KernelSU 管理器                      │\n");
    printf("      │                                                     │\n");
    SetColor(RED);
    printf("      │     [0]   返回主菜单                                │\n");
    SetColor(WHITE);
    printf("      │                                                     │\n");
    printf("      └─────────────────────────────────────────────────────┘\n");
    printf("\n");
    SetColor(CYAN);
    printf("      > 请输入选项: ");
    ResetColor();
}

void Submenu_NoUnlock() {
    while (true) {
        DrawSubmenu_NoUnlock();
        string s;
        cin >> s;
        cin.ignore();

        if (s == "1") { if (Check1()) Func1_SetSELinux(); }
        if (s == "2") { if (Check2()) Func2_InstallKernelSU(); }
        if (s == "0") { break; }
    }
}

void Menu() {
    while (true) {
        DrawAnimatedMenu();
        string s;
        cin >> s;
        cin.ignore();

        if (s == "1") { Submenu_NoUnlock(); }
        if (s == "2") { if (Check3()) Func3_UnlockBL_8E5(); }
        if (s == "3") { if (Check1()) Func4_UnlockBL_8E(); }
        if (s == "4") { if (Check1()) Func5_UnlockBL_8G3(); }
        if (s == "0") { KillAdbFastboot(); break; }
    }
}

int main() {
    system("chcp 65001 >nul");
    SetConsoleTitleW(L"\u5C0F\u7C73\u89E3\u9501 BL ROOT \u5DE5\u5177");
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    AutoSetupADB();
    Menu();
    return 0;
}
