// A lightweight utility to fix the Windows 11 taskbar auto-hide bug.
// Based on the technical blueprint provided.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include "resource.h"

// --- Global Variables ---
HINSTANCE g_hInstance = NULL;
HWND g_hMessageWindow = NULL;
HHOOK g_hMouseHook = NULL;
HWINEVENTHOOK g_hForegroundHook = NULL;
BOOL g_isFullScreenActive = FALSE;
BOOL g_isFixOnCooldown = FALSE;
BOOL g_isPaused = FALSE;
UINT_PTR g_cooldownTimerId = 1;

// --- Constants ---
const wchar_t CLASS_NAME[] = L"TaskbarAutoHideFixMessageClass";
const wchar_t WINDOW_NAME[] = L"Taskbar Auto-Hide Fix";
const UINT WM_TRAYICON = WM_APP + 1;
const UINT WM_USER_CURSOR_AT_EDGE = WM_APP + 2;
const UINT ID_TRAY_EXIT = 1001;
const UINT ID_TRAY_PAUSE = 1002;
const UINT COOLDOWN_TIMER_ID = 1;
const int COOLDOWN_DURATION_MS = 3000; // 3 seconds

// --- Forward Declarations ---
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon(HWND hWnd);
void UpdateTrayIcon();
void ShowContextMenu(HWND hWnd, POINT pt);
void InstallHooks();
void UninstallHooks();
BOOL CheckIfForegroundIsFullscreen(HWND hwnd);
BOOL IsTaskbarStuckOnMonitor(HMONITOR hMonitor);
void ApplyCorrectionToMonitor(HMONITOR hMonitor);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);


// --- Entry Point ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    g_hMessageWindow = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    if (g_hMessageWindow == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    AddTrayIcon(g_hMessageWindow);
    InstallHooks();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UninstallHooks();
    RemoveTrayIcon(g_hMessageWindow);

    return (int)msg.wParam;
}

// --- Window Procedure ---
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            POINT curPoint;
            GetCursorPos(&curPoint);
            ShowContextMenu(hWnd, curPoint);
        }
        break;

    case WM_USER_CURSOR_AT_EDGE:
        if (!g_isFixOnCooldown && !g_isFullScreenActive && !g_isPaused) {
            EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
        }
        break;

    case WM_TIMER:
        if (wParam == COOLDOWN_TIMER_ID) {
            g_isFixOnCooldown = FALSE;
            KillTimer(hWnd, COOLDOWN_TIMER_ID);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_TRAY_PAUSE:
            g_isPaused = !g_isPaused;
            UpdateTrayIcon();
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// --- Tray Icon Management ---
void AddTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
    wcscpy_s(nid.szTip, L"Taskbar Auto-Hide Fix");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void UpdateTrayIcon() {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = g_hMessageWindow;
    nid.uID = 1;
    nid.uFlags = NIF_TIP;
    if (g_isPaused) {
        wcscpy_s(nid.szTip, L"Taskbar Auto-Hide Fix (Paused)");
    }
    else {
        wcscpy_s(nid.szTip, L"Taskbar Auto-Hide Fix");
    }
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}


void ShowContextMenu(HWND hWnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    UINT pauseFlags = MF_STRING | (g_isPaused ? MF_CHECKED : MF_UNCHECKED);
    InsertMenu(hMenu, -1, pauseFlags, ID_TRAY_PAUSE, L"Pause Detection");
    InsertMenu(hMenu, -1, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

// --- System Hooks ---
void InstallHooks() {
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, g_hInstance, 0);
    g_hForegroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
}

void UninstallHooks() {
    if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
    if (g_hForegroundHook) UnhookWinEvent(g_hForegroundHook);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE) {
        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        HMONITOR hMonitor = MonitorFromPoint(pMouseStruct->pt, MONITOR_DEFAULTTONEAREST);
        if (hMonitor) {
            MONITORINFO mi = { sizeof(mi) };
            if (GetMonitorInfo(hMonitor, &mi)) {
                // Check if cursor is at the very bottom edge of the monitor
                if (pMouseStruct->pt.y >= mi.rcMonitor.bottom - 2) {
                    PostMessage(g_hMessageWindow, WM_USER_CURSOR_AT_EDGE, 0, 0);
                }
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (event == EVENT_SYSTEM_FOREGROUND) {
        g_isFullScreenActive = CheckIfForegroundIsFullscreen(hwnd);
    }
}

// --- Detection & Correction Logic ---
BOOL CheckIfForegroundIsFullscreen(HWND hwnd) {
    if (!hwnd) return FALSE;

    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hMonitor, &mi)) return FALSE;

    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) return FALSE;

    return windowRect.left == mi.rcMonitor.left &&
           windowRect.right == mi.rcMonitor.right &&
           windowRect.top == mi.rcMonitor.top &&
           windowRect.bottom == mi.rcMonitor.bottom;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    if (IsTaskbarStuckOnMonitor(hMonitor)) {
        ApplyCorrectionToMonitor(hMonitor);
        g_isFixOnCooldown = TRUE;
        SetTimer(g_hMessageWindow, COOLDOWN_TIMER_ID, COOLDOWN_DURATION_MS, NULL);
    }
    return TRUE; // Continue enumeration
}

struct EnumWindowCallback_Data {
    HMONITOR hMonitor;
    HWND hTaskbar;
};

BOOL CALLBACK EnumWindowCallback(HWND hwnd, LPARAM lParam) {
    EnumWindowCallback_Data* data = (EnumWindowCallback_Data*)lParam;

    wchar_t className[256];
    if (GetClassName(hwnd, className, 256) && wcscmp(className, L"Shell_TrayWnd") == 0) {
        HMONITOR hWindowMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (hWindowMonitor == data->hMonitor) {
            data->hTaskbar = hwnd;
            return FALSE; // Stop enumeration
        }
    }
    return TRUE; // Continue enumeration
}


BOOL IsTaskbarStuckOnMonitor(HMONITOR hMonitor) {
    EnumWindowCallback_Data data = { hMonitor, NULL };
    EnumWindows(EnumWindowCallback, (LPARAM)&data);
    HWND hTaskbar = data.hTaskbar;

    if (!hTaskbar) return FALSE;

    // 1. User Intent is Confirmed (Auto-hide is on)
    APPBARDATA abd = { sizeof(abd), hTaskbar };
    UINT state = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    if (!(state & ABS_AUTOHIDE)) {
        return FALSE;
    }

    // 2. Taskbar is Physically Hidden
    RECT taskbarRect;
    GetWindowRect(hTaskbar, &taskbarRect);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);

    bool isPhysicallyHidden = false;
    // Assuming taskbar is at the bottom
    if (taskbarRect.top >= mi.rcMonitor.bottom) {
        isPhysicallyHidden = true;
    }
    // Add checks for other positions if necessary (top, left, right)

    if (!isPhysicallyHidden) {
        return FALSE;
    }

    // 3. Activation Trigger is Present (already checked by the mouse hook)
    // 4. No Fullscreen Obstruction is Active (already checked by the event hook)

    return TRUE;
}

void ApplyCorrectionToMonitor(HMONITOR hMonitor) {
    EnumWindowCallback_Data data = { hMonitor, NULL };
    EnumWindows(EnumWindowCallback, (LPARAM)&data);
    HWND hTaskbar = data.hTaskbar;

    if (hTaskbar) {
        // Primary Correction: The UI Refresh Exploit
        SendMessage(hTaskbar, WM_SETTINGCHANGE, SPI_SETLOGICALDPIOVERRIDE, 0);
    }
}