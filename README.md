Kinda works, still in devlopment
# Windows 11 Taskbar Auto-Hide Fix

A lightweight, background utility to automatically detect and correct the Windows 11 taskbar auto-hide bug.

## About the Application

This utility runs silently in the system tray and monitors for the specific conditions where the Windows 11 taskbar, when set to "auto-hide," fails to reappear when summoned by the user. It is designed to be fully autonomous, operate with minimal system overhead, and apply its corrective measures without requiring a disruptive restart of the Windows Explorer shell process.

## Features

-   **Automatic Detection:** A hybrid event-driven model accurately detects when the taskbar is "stuck" in a hidden state.
-   **Lightweight Operation:** The application consumes virtually no CPU resources while idle, only performing checks when necessary.
-   **Non-Disruptive Correction:** It forces the taskbar to refresh and resynchronize its state without restarting any system processes.
-   **Multi-Monitor Support:** Correctly identifies and fixes the taskbar on any connected display.
-   **DPI-Aware:** Works reliably on systems with high-DPI displays and custom scaling factors.
-   **No Admin Rights Required:** Runs entirely within the standard user's security context.
-   **System Tray Icon:** Provides easy access to pause or exit the utility.

## How to Build

This application is built using the C++ Win32 API and requires the Microsoft C++ toolchain (available with Visual Studio or the Visual Studio Build Tools).

### Build Steps

1.  **Install Dependencies:**
    *   Install Visual Studio with the "Desktop development with C++" workload, or install the standalone [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022).

2.  **Open Developer Command Prompt:**
    *   Open the "Developer Command Prompt for VS 2022" (or your installed version) from the Start Menu. This provides an environment where the necessary build tools (like `cl.exe`, `link.exe`, `rc.exe`) are available in the PATH.

3.  **Navigate to Project Directory:**
    *   In the Developer Command Prompt, change to the drive where the source code is located. For example:
        ```cmd
        cd /d K:
        ```
    *   Navigate to the project folder:
        ```cmd
        cd \Projects_2\Kode\TaskBarFix\WinTaskBarAutoHideFix
        ```

4.  **Run the Build Script:**
    *   Execute the PowerShell build script from within the project's root directory:
        ```powershell
        powershell -ExecutionPolicy Bypass -File .\build.ps1
        ```

5.  **Find the Executable:**
    *   If the build is successful, the compiled executable (`WinTaskBarAutoHideFix.exe`) will be located in the `build` subdirectory.

## How It Works

The utility is based on the detailed technical blueprint provided in the project. It uses a combination of a low-level mouse hook (`WH_MOUSE_LL`) and a `SetWinEventHook` to create an efficient detection mechanism.

1.  A **mouse hook** detects when the cursor moves to the bottom edge of any screen, posting a message to the application.
2.  A **WinEvent hook** listens for foreground window changes to determine if a fullscreen application is running, which would normally and correctly hide the taskbar.
3.  When triggered by the mouse hook, the application runs a **multi-factor heuristic check** on each monitor to confirm that the taskbar is genuinely stuck and not just hidden for a valid reason (like a fullscreen app).
4.  If a taskbar is confirmed to be stuck, the utility sends it a `WM_SETTINGCHANGE` message with the `SPI_SETLOGICALDPIOVERRIDE` parameter. This undocumented but effective technique forces the Windows Shell to perform a comprehensive UI refresh, causing the taskbar to re-evaluate its state and become visible again.
