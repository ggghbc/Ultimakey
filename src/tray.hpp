#pragma once

#include "types.hpp"

namespace Ultimakey {

class TrayIcon {
public:
    static constexpr UINT WM_TRAY_CALLBACK = WM_APP + 101;

    static constexpr UINT IDM_TOGGLE_AUTO  = 1001;
    static constexpr UINT IDM_TOGGLE_PAUSE = 1002;
    static constexpr UINT IDM_SETTINGS     = 1003;
    static constexpr UINT IDM_OPEN_LOG     = 1004;
    static constexpr UINT IDM_EXIT         = 1005;

    static TrayIcon& Instance();

    bool Create(HWND hwnd, HINSTANCE hinstance);
    void Destroy();

    void UpdateState(bool auto_enabled, bool paused);
    void ShowContextMenu(HWND hwnd);

private:
    TrayIcon();
    ~TrayIcon();

    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    NOTIFYICONDATAW nid_ = {};
    HICON icon_on_ = nullptr;
    HICON icon_off_ = nullptr;
    bool created_ = false;
};

} // namespace Ultimakey
