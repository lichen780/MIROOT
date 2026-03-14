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
const string ADB_URL = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";
const string ZIP_FILE = "platform-tools.zip";
const string TOOL_DIR = "platform-tools";

fs::path adb_bin = cwd / "adb.exe";
fs::path fastboot_bin = cwd / "fastboot.exe";
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

// 标题界面
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

// 信息输出
void OK(const string& msg) { SetColor(Color::GREEN); println("✅ {}", msg); ResetColor(); }
void ERR(const string& msg) { SetColor(Color::RED); println("❌ {}", msg); ResetColor(); }
void INFO(const string& msg) { SetColor(Color::BLUE); println("ℹ️  {}", msg); ResetColor(); }
void WARN(const string& msg) { SetColor(Color::YELLOW); println("⚠️  {}", msg); ResetColor(); }

// 按任意键返回菜单
void PressAnyKeyBack() {
    SetColor(Color::GRAY);
    cout << "\n执行完成！按 回车键 返回主菜单...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
    ResetColor();
}

// 执行命令
static auto Exec(const string& bin, const string& args) -> tuple<int, string> {
    string cmd = format("{} {} 2>&1", bin, args);
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

// 自动下载 ADB
bool DownloadADB() {
    INFO("正在下载 ADB 工具...");
    HRESULT res = URLDownloadToFileA(NULL, ADB_URL.c_str(), ZIP_FILE.c_str(), 0, NULL);
    if (res != S_OK) {
        ERR("下载失败！请检查网络");
        return false;
    }
    OK("ADB 下载完成！");
    return true;
}

// 自动解压
bool ExtractZIP() {
    INFO("正在解压 ADB 工具...");
    system("powershell -Command \"Expand-Archive -Path platform-tools.zip -DestinationPath ./ -Force\" >nul 2>&1");
    this_thread::sleep_for(5s);
    OK("解压完成！");
    return true;
}

// 自动部署 ADB
void AutoSetupADB() {
    if (fs::exists(adb_bin)) return;

    WARN("未检测到 ADB 工具，正在自动下载部署...");
    if (!DownloadADB()) return;
    if (!ExtractZIP()) return;

    fs::path tool_path = cwd / TOOL_DIR;
    
    if (fs::exists(tool_path / "adb.exe"))
        fs::copy(tool_path / "adb.exe", cwd / "adb.exe", fs::copy_options::overwrite_existing);
    if (fs::exists(tool_path / "fastboot.exe"))
        fs::copy(tool_path / "fastboot.exe", cwd / "fastboot.exe", fs::copy_options::overwrite_existing);
    if (fs::exists(tool_path / "AdbWinApi.dll"))
        fs::copy(tool_path / "AdbWinApi.dll", cwd / "AdbWinApi.dll", fs::copy_options::overwrite_existing);
    if (fs::exists(tool_path / "AdbWinUsbApi.dll"))
        fs::copy(tool_path / "AdbWinUsbApi.dll", cwd / "AdbWinUsbApi.dll", fs::copy_options::overwrite_existing);

    fs::remove(ZIP_FILE);
    fs::remove_all(tool_path);
    OK("ADB 部署完成！");
}

// 清理进程
void KillAdbFastboot() {
    system("adb kill-server >nul 2>&1");
    system("taskkill /f /im adb.exe >nul 2>&1");
    system("taskkill /f /im fastboot.exe >nul 2>&1");
}

// 关闭窗口钩子
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        KillAdbFastboot();
        return TRUE;
    }
    return FALSE;
}

// 精准检测设备
bool CheckDeviceSerial() {
    auto [code, output] = Exec(adb_bin.string(), "devices");
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

// 等待设备（3秒）
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

// 获取手机信息
void ShowDeviceInfo() {
    Loading("正在获取手机信息");
    auto [_, brand] = Exec(adb_bin.string(), "shell getprop ro.product.brand");
    auto [__, model] = Exec(adb_bin.string(), "shell getprop ro.product.model");
    auto [___, android] = Exec(adb_bin.string(), "shell getprop ro.build.version.release");
    auto [____, cpu] = Exec(adb_bin.string(), "shell getprop ro.product.board");

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

// 文件检查
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

// ===================== 功能1 =====================
bool Func1_SetSELinux() {
    Title("免解BL - 设置SELinux宽容模式");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("重启至 Fastboot");
    auto [c2, o2] = Exec(adb_bin.string(), "reboot bootloader");
    if (c2 != 0) return false;

    INFO("请确认手机已进入 Fastboot 模式");
    PressAnyKeyBack();

    Loading("设置 SELinux 为宽容");
    auto [c3, o3] = Exec(fastboot_bin.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");
    if (c3 != 0) return false;

    Loading("开机");
    Exec(fastboot_bin.string(), "continue");

    INFO("请等待手机完全开机");
    PressAnyKeyBack();
    Loading("等待设备连接");
    Exec(adb_bin.string(), "wait-for-device");

    Loading("检查SELinux状态");
    auto [c5, o5] = Exec(adb_bin.string(), "shell getenforce");
    if (c5 != 0 || o5.find("Permissive") == string::npos) {
        ERR("SELinux 设置失败！");
        PressAnyKeyBack();
        return false;
    }

    OK("SELinux 已设置为宽容模式");
    PressAnyKeyBack(); // 必须按回车才返回
    return true;
}

// ===================== 功能2 =====================
bool Func2_InstallRoot() {
    Title("免解BL - 安装ROOT权限");
    WaitForDeviceLoop();
    ShowDeviceInfo();

    Loading("推送ROOT组件");
    Exec(adb_bin.string(), format("push {} /data/local/tmp/ksud", ksud.string()));
    Exec(adb_bin.string(), "shell chmod 755 /data/local/tmp/ksud");

    Loading("启动ROOT守护进程");
    Exec(adb_bin.string(), "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 '/data/local/tmp/ksud' i32 1 s16 'late-load' s16 '/data/local/tmp/ksud-log.txt' i32 60");
    this_thread::sleep_for(3s);

    auto [c4, o4] = Exec(adb_bin.string(), "shell grep 'kernelsu' /proc/modules");
    if (c4 != 0 || o4.find("kernelsu") == string::npos) {
        ERR("ROOT模块加载失败！");
        PressAnyKeyBack();
        return false;
    }
    OK("内核ROOT模块加载成功");

    Loading("卸载旧版管理器");
    Exec(adb_bin.string(), "shell pm uninstall me.weishu.kernelsu");

    Loading("安装ROOT管理器");
    Exec(adb_bin.string(), format("push {} /data/local/tmp/ksu.apk", ksum.string()));
    Exec(adb_bin.string(), "shell pm install -r /data/local/tmp/ksu.apk");

    WARN("请打开 KernelSU → 超级用户 → 允许 Shell ROOT权限");
    PressAnyKeyBack();

    while (true) {
        Loading("检查ROOT授权");
        auto [r, o] = Exec(adb_bin.string(), "shell su -c 'id -u'");
        if (o.find('0') != string::npos) {
            OK("ROOT授权成功！");
            break;
        }
        ERR("未授权，请在KernelSU允许后重试");
        PressAnyKeyBack();
    }

    Loading("恢复SELinux为安全模式");
    Exec(adb_bin.string(), "shell su -c 'setenforce 1'");
    auto [rr, oo] = Exec(adb_bin.string(), "shell getenforce");
    if (oo.find("Enforcing") == string::npos) {
        ERR("SELinux恢复失败");
        PressAnyKeyBack();
        return false;
    }
    OK("SELinux 已恢复为强制模式");

    OK("\n🎉 免解BL ROOT安装全部完成！");
    Exec(adb_bin.string(), "shell am start -S me.weishu.kernelsu");
    
    PressAnyKeyBack(); // 必须按回车才返回主菜单
    return true;
}

// ===================== 主菜单 =====================
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

        if (s == "1") { if (Check1()) Func1_SetSELinux(); else system("pause >nul"); }
        if (s == "2") { if (Check2()) Func2_InstallRoot(); else system("pause >nul"); }
        if (s == "3") {
            KillAdbFastboot();
            break;
        }

        this_thread::sleep_for(300ms);
    }
}

// ===================== 主函数 =====================
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("免解BL ROOT工具 | 无需解锁BL直接ROOT");
    
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    AutoSetupADB();
    Menu();
    
    return 0;
}
