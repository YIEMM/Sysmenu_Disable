#include <Windows.h>
#include "sysmenu_hook.h"

static WNDPROC g_originalWndProc = nullptr;
static bool g_disableSystemMenu = true;
static HWND g_hookedWindow = nullptr;
static DWORD g_currentPid = 0;
static bool g_allowCaptionOperation = false;
static bool g_buttonPending = false;
static UINT g_buttonTimerId = 0;
static UINT g_buttonArea = 0;

struct EnumWindowsData {
    DWORD pid;
    HWND hwnd;
};

BOOL CALLBACK FindMainWindowCallback(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
    
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    
    if (windowPid == data->pid) {
        data->hwnd = hwnd;
        return FALSE;
    }
    
    return TRUE;
}

HWND FindMainWindow(DWORD pid) {
    EnumWindowsData data = { pid, nullptr };
    EnumWindows(FindMainWindowCallback, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

#define BUTTON_TIMER_ID 1001
#define BUTTON_CLICK_THRESHOLD 250

static void ExecuteButtonAction(HWND hwnd, UINT hitTest) {
    switch (hitTest) {
        case HTMINBUTTON:
            ShowWindow(hwnd, SW_MINIMIZE);
            break;
        case HTMAXBUTTON:
            if (IsZoomed(hwnd)) {
                ShowWindow(hwnd, SW_RESTORE);
            } else {
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
            break;
        case HTCLOSE:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
    }
}

static LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_disableSystemMenu) {
        if (msg == WM_NCRBUTTONDOWN || msg == WM_NCRBUTTONUP || msg == WM_CONTEXTMENU) {
            return 0;
        }
        
        if (msg == WM_NCLBUTTONDOWN && wParam == HTSYSMENU) {
            return 0;
        }
        
        if (msg == WM_NCLBUTTONUP && wParam == HTSYSMENU) {
            return 0;
        }
        
        if (msg == WM_KEYDOWN && wParam == VK_TAB) {
            g_allowCaptionOperation = !g_allowCaptionOperation;
        }
        
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
        
        if (msg == WM_SYSCOMMAND) {
            WPARAM cmd = wParam & 0xFFF0;
            if (cmd == SC_KEYMENU) {
                return 0;
            }
        }

        if (msg == WM_NCLBUTTONDOWN) {
            if (wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE) {
                g_buttonPending = true;
                g_buttonArea = (UINT)wParam;
                g_buttonTimerId = SetTimer(hwnd, BUTTON_TIMER_ID, BUTTON_CLICK_THRESHOLD, nullptr);
                return 0;
            }
        }
        
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
        
        if (msg == WM_TIMER && wParam == BUTTON_TIMER_ID) {
            KillTimer(hwnd, g_buttonTimerId);
            g_buttonTimerId = 0;
            g_buttonPending = false;
            g_buttonArea = 0;
            return 0;
        }
        
        if (msg == WM_NCMOUSEMOVE && g_buttonPending) {
            if (wParam != g_buttonArea) {
                KillTimer(hwnd, g_buttonTimerId);
                g_buttonTimerId = 0;
                g_buttonPending = false;
                g_buttonArea = 0;
            }
        }
    }
    
    return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    HWND mainWnd = nullptr;
    
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            g_currentPid = GetCurrentProcessId();
            mainWnd = FindMainWindow(g_currentPid);
            if (mainWnd) {
                InstallHook(mainWnd);
            }
            break;
        case DLL_PROCESS_DETACH:
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
