#ifndef SYSMENU_HOOK_H
#define SYSMENU_HOOK_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SYSMENU_HOOK_EXPORTS
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __declspec(dllimport)
#endif

EXPORT_API BOOL InstallHook(HWND hwnd);
EXPORT_API BOOL UninstallHook(HWND hwnd);

#ifdef __cplusplus
}
#endif

#endif
