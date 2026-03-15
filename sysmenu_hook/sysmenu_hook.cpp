#include <Windows.h>
#include "sysmenu_hook.h"

// 全局变量
static WNDPROC g_originalWndProc = nullptr;  // 原始窗口过程
static bool g_disableSystemMenu = true;      // 是否禁用系统菜单
static HWND g_hookedWindow = nullptr;        // 已挂钩的窗口
static DWORD g_currentPid = 0;               // 当前进程ID
static bool g_allowCaptionOperation = false; // 是否允许标题栏操作
static bool g_buttonPending = false;         // 按钮操作是否待定
static UINT g_buttonTimerId = 0;             // 按钮计时器ID
static UINT g_buttonArea = 0;                // 按钮区域

// 枚举窗口数据结构
struct EnumWindowsData {
    DWORD pid;  // 进程ID
    HWND hwnd;  // 窗口句柄
};

// 枚举窗口回调函数
BOOL CALLBACK FindMainWindowCallback(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
    
    // 只处理可见窗口
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    
    // 找到目标进程的窗口
    if (windowPid == data->pid) {
        data->hwnd = hwnd;
        return FALSE;
    }
    
    return TRUE;
}

// 查找进程的主窗口
HWND FindMainWindow(DWORD pid) {
    EnumWindowsData data = { pid, nullptr };
    EnumWindows(FindMainWindowCallback, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

// 按钮计时器ID
#define BUTTON_TIMER_ID 1001
// 按钮点击阈值（毫秒）
#define BUTTON_CLICK_THRESHOLD 250

// 执行按钮操作
static void ExecuteButtonAction(HWND hwnd, UINT hitTest) {
    switch (hitTest) {
        case HTMINBUTTON:  // 最小化按钮
            ShowWindow(hwnd, SW_MINIMIZE);
            break;
        case HTMAXBUTTON:  // 最大化/还原按钮
            if (IsZoomed(hwnd)) {
                ShowWindow(hwnd, SW_RESTORE);
            } else {
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
            break;
        case HTCLOSE:  // 关闭按钮
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
    }
}

// 自定义窗口过程
static LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_disableSystemMenu) {
        // 禁用右键菜单相关消息
        if (msg == WM_NCRBUTTONDOWN || msg == WM_NCRBUTTONUP || msg == WM_CONTEXTMENU) {
            return 0;
        }
        
        // 禁用系统菜单按钮
        if (msg == WM_NCLBUTTONDOWN && wParam == HTSYSMENU) {
            return 0;
        }
        
        if (msg == WM_NCLBUTTONUP && wParam == HTSYSMENU) {
            return 0;
        }
        
        // 切换标题栏操作权限（Tab键）
        if (msg == WM_KEYDOWN && wParam == VK_TAB) {
            g_allowCaptionOperation = !g_allowCaptionOperation;
        }
        
        // 处理标题栏操作
        if (msg == WM_NCLBUTTONDOWN && wParam == HTCAPTION) {
            if (g_allowCaptionOperation) {
                return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
            } else {
                return 0;
            }
        }
        
        if (msg == WM_NCLBUTTONUP && wParam == HTCAPTION) {
            if (g_allowCaptionOperation) {
                return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
            } else {
                return 0;
            }
        }
        
        // 禁用键盘系统菜单
        if (msg == WM_SYSCOMMAND) {
            WPARAM cmd = wParam & 0xFFF0;
            if (cmd == SC_KEYMENU) {
                return 0;
            }
        }

        // 处理按钮按下事件
        if (msg == WM_NCLBUTTONDOWN) {
            if (wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE) {
                g_buttonPending = true;
                g_buttonArea = (UINT)wParam;
                g_buttonTimerId = SetTimer(hwnd, BUTTON_TIMER_ID, BUTTON_CLICK_THRESHOLD, nullptr);
                return 0;
            }
        }
        
        // 处理按钮释放事件
        if (msg == WM_NCLBUTTONUP) {
            if (g_buttonPending && wParam == g_buttonArea) {
                KillTimer(hwnd, g_buttonTimerId);
                g_buttonTimerId = 0;
                g_buttonPending = false;
                ExecuteButtonAction(hwnd, g_buttonArea);
                g_buttonArea = 0;
                return 0;
            }
        }
        
        // 处理计时器事件（长按超时）
        if (msg == WM_TIMER && wParam == BUTTON_TIMER_ID) {
            KillTimer(hwnd, g_buttonTimerId);
            g_buttonTimerId = 0;
            g_buttonPending = false;
            g_buttonArea = 0;
            return 0;
        }
        
        // 处理鼠标移动事件（移出按钮区域）
        if (msg == WM_NCMOUSEMOVE && g_buttonPending) {
            if (wParam != g_buttonArea) {
                KillTimer(hwnd, g_buttonTimerId);
                g_buttonTimerId = 0;
                g_buttonPending = false;
                g_buttonArea = 0;
            }
        }
    }
    
    // 调用原始窗口过程
    return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}

// DLL入口函数
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    HWND mainWnd = nullptr;
    
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:  // DLL加载时
            DisableThreadLibraryCalls(hinstDLL);
            g_currentPid = GetCurrentProcessId();
            mainWnd = FindMainWindow(g_currentPid);
            if (mainWnd) {
                InstallHook(mainWnd);
            }
            break;
        case DLL_PROCESS_DETACH:  // DLL卸载时
            if (g_hookedWindow && g_originalWndProc) {
                if (g_buttonTimerId) {
                    KillTimer(g_hookedWindow, g_buttonTimerId);
                    g_buttonTimerId = 0;
                }
                SetWindowLongPtr(g_hookedWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
            }
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

// 安装钩子
BOOL InstallHook(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return FALSE;
    }
    
    if (g_hookedWindow) {
        return FALSE;
    }
    
    g_hookedWindow = hwnd;
    g_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
    
    if (!g_originalWndProc) {
        g_hookedWindow = nullptr;
        return FALSE;
    }
    
    return TRUE;
}

// 卸载钩子
BOOL UninstallHook(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return FALSE;
    }
    
    if (g_hookedWindow != hwnd) {
        return FALSE;
    }
    
    if (g_originalWndProc) {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
    }
    
    g_hookedWindow = nullptr;
    g_originalWndProc = nullptr;
    
    return TRUE;
}
