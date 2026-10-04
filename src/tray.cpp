#include "tray.hpp"
#include "resource.h"
#include "settings.hpp"
#include "engine.hpp"
#include "i18n.hpp"

namespace Ultimakey {

TrayIcon& TrayIcon::Instance() {
    static TrayIcon instance;
    return instance;
}

TrayIcon::TrayIcon() = default;

TrayIcon::~TrayIcon() {
    Destroy();
}

bool TrayIcon::Create(HWND hwnd, HINSTANCE hinstance) {
    if (created_) return true;

    hwnd_ = hwnd;
    hinstance_ = hinstance;

    icon_on_ = LoadIconW(hinstance, MAKEINTRESOURCEW(IDI_APP_ICON_ON));
    icon_off_ = LoadIconW(hinstance, MAKEINTRESOURCEW(IDI_APP_ICON_OFF));

    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = hwnd;
    nid_.uID = 1;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAY_CALLBACK;
    nid_.hIcon = icon_on_;
    wcscpy_s(nid_.szTip, Tr(StrId::TrayTitle));

    created_ = Shell_NotifyIconW(NIM_ADD, &nid_);
    return created_;
}

void TrayIcon::Destroy() {
    if (created_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        created_ = false;
    }
}

void TrayIcon::UpdateState(bool auto_enabled, bool paused) {
    if (!created_) return;

    bool is_active = auto_enabled && !paused;
    nid_.hIcon = is_active ? icon_on_ : icon_off_;

    if (paused) {
        wcscpy_s(nid_.szTip, Tr(StrId::TrayTipPaused));
    } else if (!auto_enabled) {
        wcscpy_s(nid_.szTip, Tr(StrId::TrayTipAutoOff));
    } else {
        wcscpy_s(nid_.szTip, Tr(StrId::TrayTipRunning));
    }

    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void TrayIcon::ShowNotification(const std::wstring& title, const std::wstring& message) {
    if (!created_) return;
    NOTIFYICONDATAW nid = nid_;
    nid.uFlags |= NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayIcon::ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hmenu = CreatePopupMenu();
    if (!hmenu) return;

    const auto& s = Settings::Instance();
    bool paused = Engine::Instance().IsPaused();

    AppendMenuW(hmenu, MF_STRING | (s.auto_enabled ? MF_CHECKED : MF_UNCHECKED),
                IDM_TOGGLE_AUTO, Tr(StrId::TrayMenuAuto));
    AppendMenuW(hmenu, MF_STRING | (paused ? MF_CHECKED : MF_UNCHECKED),
                IDM_TOGGLE_PAUSE, Tr(StrId::TrayMenuPause));
    AppendMenuW(hmenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hmenu, MF_STRING, IDM_SETTINGS, Tr(StrId::TrayMenuSettings));
    AppendMenuW(hmenu, MF_STRING, IDM_OPEN_LOG, Tr(StrId::TrayMenuLog));
    AppendMenuW(hmenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hmenu, MF_STRING, IDM_EXIT, Tr(StrId::TrayMenuExit));

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);

    DestroyMenu(hmenu);
}

} // namespace Ultimakey
