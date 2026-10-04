#include "types.hpp"
#include "settings.hpp"
#include "engine.hpp"
#include "hook.hpp"
#include "tray.hpp"
#include "ui_settings.hpp"
#include "logger.hpp"
#include <timeapi.h>

using namespace Ultimakey;

static HWND g_main_hwnd = nullptr;
static HANDLE g_single_instance_mutex = nullptr;

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_TIMER: {
            if (wparam == 1001) {
                KillTimer(hwnd, 1001);
                Engine::Instance().OnBoundaryTimer();
                return 0;
            }
            break;
        }

        case TrayIcon::WM_TRAY_CALLBACK: {
            if (lparam == WM_RBUTTONUP) {
                TrayIcon::Instance().ShowContextMenu(hwnd);
            } else if (lparam == WM_LBUTTONDBLCLK) {
                SettingsDialog::Show(hwnd, GetModuleHandleW(nullptr));
            }
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);
            switch (id) {
                case TrayIcon::IDM_TOGGLE_AUTO: {
                    auto& s = Settings::Instance();
                    s.auto_enabled = !s.auto_enabled;
                    s.Save();
                    TrayIcon::Instance().UpdateState(s.auto_enabled, Engine::Instance().IsPaused());
                    break;
                }
                case TrayIcon::IDM_TOGGLE_PAUSE: {
                    bool new_pause = !Engine::Instance().IsPaused();
                    Engine::Instance().SetPaused(new_pause);
                    TrayIcon::Instance().UpdateState(Settings::Instance().auto_enabled, new_pause);
                    break;
                }
                case TrayIcon::IDM_SETTINGS: {
                    SettingsDialog::Show(hwnd, GetModuleHandleW(nullptr));
                    break;
                }
                case TrayIcon::IDM_OPEN_LOG: {
                    ShellExecuteW(nullptr, L"open", Settings::GetAppDataDirectory().c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
                    break;
                }
                case TrayIcon::IDM_EXIT: {
                    PostQuitMessage(0);
                    break;
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nShowCmd*/) {
    // 0. High-resolution timer (1ms scheduler tick)
    timeBeginPeriod(1);

    // 1. Single-Instance Check
    g_single_instance_mutex = CreateMutexW(nullptr, TRUE, L"Ultimakey_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND prev_wnd = FindWindowW(L"Ultimakey_Hidden_Class", nullptr);
        if (prev_wnd) {
            PostMessageW(prev_wnd, WM_COMMAND, MAKEWPARAM(TrayIcon::IDM_SETTINGS, 0), 0);
        }
        if (g_single_instance_mutex) CloseHandle(g_single_instance_mutex);
        timeEndPeriod(1);
        return 0; // Already running, activated existing instance settings
    }

    // 2. Load Settings (zero disk writes on startup)
    Settings::Instance().Load();
    Logger::Instance().Write("Ultimakey: Запуск программы");

    // 3. Initialize Engine & Language Data from Resources
    if (!Engine::Instance().Initialize(hInstance)) {
        MessageBoxW(nullptr, L"Не удалось загрузить встроенные языковые словари!", L"Ошибка Ultimakey", MB_OK | MB_ICONERROR);
        if (g_single_instance_mutex) CloseHandle(g_single_instance_mutex);
        timeEndPeriod(1);
        return 1;
    }

    // 4. Create Hidden Main Window for Tray Messages
    const wchar_t* kMainClass = L"Ultimakey_Hidden_Class";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kMainClass;
    RegisterClassExW(&wc);

    g_main_hwnd = CreateWindowExW(0, kMainClass, L"Ultimakey_Message_Window",
                                 0, 0, 0, 0, 0,
                                 nullptr, nullptr, hInstance, nullptr);

    if (!g_main_hwnd) {
        Logger::Instance().Write("Ultimakey: Ошибка создания окна сообщений");
        if (g_single_instance_mutex) CloseHandle(g_single_instance_mutex);
        timeEndPeriod(1);
        return 1;
    }

    Engine::Instance().SetMessageHwnd(g_main_hwnd);

    // 5. Create Tray Icon
    TrayIcon::Instance().Create(g_main_hwnd, hInstance);
    TrayIcon::Instance().UpdateState(Settings::Instance().auto_enabled, Engine::Instance().IsPaused());

    // 6. Install Low-Level Keyboard and Mouse Hooks
    if (!HookManager::Instance().Install()) {
        MessageBoxW(nullptr, L"Не удалось установить хук клавиатуры!", L"Ошибка Ultimakey", MB_OK | MB_ICONERROR);
        TrayIcon::Instance().Destroy();
        if (g_single_instance_mutex) CloseHandle(g_single_instance_mutex);
        timeEndPeriod(1);
        return 1;
    }

    // 7. Trim cold pages from working set (< 1 MB RAM) and enter Message Loop
    SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        HWND dlg = SettingsDialog::GetHwnd();
        if (dlg && IsDialogMessageW(dlg, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 8. Cleanup & Shutdown
    Logger::Instance().Write("Ultimakey: Завершение работы");
    HookManager::Instance().Uninstall();
    TrayIcon::Instance().Destroy();
    Engine::Instance().Shutdown();

    if (g_single_instance_mutex) {
        ReleaseMutex(g_single_instance_mutex);
        CloseHandle(g_single_instance_mutex);
    }

    timeEndPeriod(1);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nShowCmd);
}
