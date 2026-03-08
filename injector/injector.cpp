#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "psapi.lib")

struct ProcessInfo {
    DWORD pid;
    std::string name;
    std::string title;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    std::vector<ProcessInfo>* processes = reinterpret_cast<std::vector<ProcessInfo>*>(lParam);
    
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    
    if (pid == 0) {
        return TRUE;
    }
    
    char title[256];
    if (GetWindowTextA(hwnd, title, sizeof(title)) == 0) {
        return TRUE;
    }
    
    char processName[MAX_PATH];
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL) {
        if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName)) != 0) {
            ProcessInfo info;
            info.pid = pid;
            info.name = processName;
            info.title = title;
            processes->push_back(info);
        }
        CloseHandle(hProcess);
    }
    
    return TRUE;
}

std::vector<ProcessInfo> FindProcesses(const std::string& processName, const std::string& windowTitle) {
    std::vector<ProcessInfo> processes;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&processes));
    
    std::vector<ProcessInfo> filtered;
    for (const auto& proc : processes) {
        bool match = true;
        
        if (!processName.empty()) {
            if (proc.name.find(processName) == std::string::npos) {
                match = false;
            }
        }
        
        if (!windowTitle.empty()) {
            if (proc.title.find(windowTitle) == std::string::npos) {
                match = false;
            }
        }
        
        if (match) {
            filtered.push_back(proc);
        }
    }
    
    return filtered;
}

bool IsProcessWow64(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        return false;
    }
    
    BOOL isWow64 = FALSE;
    IsWow64Process(hProcess, &isWow64);
    
    CloseHandle(hProcess);
    return isWow64 == TRUE;
}

bool IsInjectorWow64() {
    BOOL isWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &isWow64);
    return isWow64 == TRUE;
}

bool InjectDLL(DWORD pid, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "[错误] 无法打开进程: " << GetLastError() << std::endl;
        return false;
    }
    
    size_t pathLen = dllPath.length() + 1;
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT, PAGE_READWRITE);
    if (pRemoteMemory == NULL) {
        std::cerr << "[错误] 无法分配内存: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    
    if (!WriteProcessMemory(hProcess, pRemoteMemory, dllPath.c_str(), pathLen, NULL)) {
        std::cerr << "[错误] 无法写入内存: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == NULL) {
        std::cerr << "[错误] 无法获取 kernel32.dll 模块句柄" << std::endl;
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
    if (pLoadLibrary == NULL) {
        std::cerr << "[错误] 无法获取 LoadLibraryA 地址" << std::endl;
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteMemory, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "[错误] 无法创建远程线程: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    WaitForSingleObject(hThread, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    if (exitCode == 0) {
        std::cerr << "[错误] LoadLibraryA 返回 NULL，DLL 可能加载失败" << std::endl;
        return false;
    }
    
    return true;
}

void PrintUsage() {
    std::cout << "用法: injector.exe [选项] <DLL路径>" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -p <进程名>  按进程名查找目标进程" << std::endl;
    std::cout << "  -w <窗口标题> 按窗口标题查找目标进程" << std::endl;
    std::cout << "  -h           显示帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  injector.exe -p notepad.exe C:\\path\\to\\hook.dll" << std::endl;
    std::cout << "  injector.exe -w \"记事本\" C:\\path\\to\\hook.dll" << std::endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    std::cout << "=== DLL 注入器 ===" << std::endl;
    std::cout << "注入器架构: " << (IsInjectorWow64() ? "x86 (32位)" : "x64 (64位)") << std::endl;
    std::cout << std::endl;
    
    if (argc < 2) {
        PrintUsage();
        return 1;
    }
    
    std::string processName;
    std::string windowTitle;
    std::string dllPath;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            PrintUsage();
            return 0;
        } else if (arg == "-p" && i + 1 < argc) {
            processName = argv[++i];
        } else if (arg == "-w" && i + 1 < argc) {
            windowTitle = argv[++i];
        } else if (arg[0] != '-') {
            dllPath = arg;
        }
    }
    
    if (dllPath.empty()) {
        std::cerr << "[错误] 未指定 DLL 路径" << std::endl;
        PrintUsage();
        return 1;
    }
    
    if (processName.empty() && windowTitle.empty()) {
        std::cerr << "[错误] 必须指定进程名 (-p) 或窗口标题 (-w)" << std::endl;
        PrintUsage();
        return 1;
    }
    
    std::cout << "[信息] 搜索目标进程..." << std::endl;
    std::cout << "[信息] 进程名: " << (processName.empty() ? "(未指定)" : processName) << std::endl;
    std::cout << "[信息] 窗口标题: " << (windowTitle.empty() ? "(未指定)" : windowTitle) << std::endl;
    std::cout << std::endl;
    
    std::vector<ProcessInfo> processes = FindProcesses(processName, windowTitle);
    
    if (processes.empty()) {
        std::cerr << "[错误] 未找到匹配的进程" << std::endl;
        return 1;
    }
    
    std::cout << "[信息] 找到 " << processes.size() << " 个匹配的进程:" << std::endl;
    for (size_t i = 0; i < processes.size(); i++) {
        std::cout << "  [" << i + 1 << "] PID: " << processes[i].pid 
                  << ", 名称: " << processes[i].name 
                  << ", 标题: " << processes[i].title << std::endl;
    }
    std::cout << std::endl;
    
    if (processes.size() > 1) {
        std::cout << "[提示] 找到多个进程，将注入到第一个进程" << std::endl;
    }
    
    DWORD targetPid = processes[0].pid;
    bool targetWow64 = IsProcessWow64(targetPid);
    bool injectorWow64 = IsInjectorWow64();
    
    std::cout << "[信息] 目标进程架构: " << (targetWow64 ? "x86 (32位)" : "x64 (64位)") << std::endl;
    
    if (targetWow64 != injectorWow64) {
        std::cerr << "[错误] 架构不匹配！" << std::endl;
        std::cerr << "[错误] 注入器是 " << (injectorWow64 ? "x86" : "x64") 
                  << "，但目标进程是 " << (targetWow64 ? "x86" : "x64") << std::endl;
        std::cerr << "[错误] 请使用匹配架构的注入器" << std::endl;
        return 1;
    }
    
    std::cout << "[信息] 架构匹配，开始注入..." << std::endl;
    std::cout << "[信息] DLL 路径: " << dllPath << std::endl;
    std::cout << std::endl;
    
    if (InjectDLL(targetPid, dllPath)) {
        std::cout << "[成功] DLL 注入成功！" << std::endl;
        return 0;
    } else {
        std::cerr << "[失败] DLL 注入失败" << std::endl;
        return 1;
    }
}