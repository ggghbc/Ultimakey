#include "ui_settings.hpp"
#include "settings.hpp"
#include "autostart.hpp"
#include "snippets.hpp"
#include "sound.hpp"
#include "tray.hpp"
#include "engine.hpp"
#include "hook.hpp"
#include "i18n.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <vector>
#include <algorithm>
#include <cwctype>

namespace Ultimakey {

static HWND g_dialog_hwnd = nullptr;
static HFONT g_dialog_font = nullptr;
static HFONT g_title_font = nullptr;

HWND SettingsDialog::GetHwnd() noexcept {
    return g_dialog_hwnd;
}

struct ControlItem {
    HWND hwnd;
    int tab_index; // -1 = always visible (e.g. Save / Cancel / TabControl)
    StrId str_id;  // StrId::Count = no translation (edit boxes, lists, etc.)
};

static std::vector<ControlItem> g_controls;

// Control IDs
enum CtrlId {
    ID_TAB = 100,
    ID_BTN_SAVE = 101,
    ID_BTN_CANCEL = 102,

    // Tab 0 - General / Основные
    ID_CHK_AUTO = 201,
    ID_CHK_SPACE,
    ID_CHK_ENTER,
    ID_CHK_TAB,
    ID_CHK_ARROWS,
    ID_CHK_TYPO,
    ID_CHK_DOUBLESPACE,
    ID_CHK_CAPSREMAP,
    ID_CHK_SOUND,
    ID_BTN_TEST_SOUND,
    ID_CHK_AUTOSTART,
    ID_CHK_LOG,
    ID_LBL_LANG,
    ID_COMBO_LANG,

    // Tab 1 - Hotkeys / Горячие клавиши
    ID_LBL_HOTKEY = 301,
    ID_COMBO_HOTKEY,
    ID_LBL_MODS,
    ID_CHK_CTRL,
    ID_CHK_ALT,
    ID_CHK_SHIFT,
    ID_CHK_WIN,
    ID_HINT_HOTKEY,

    // Tab 2 - Snippets / Автозамена текста
    ID_GRP_SNIP_LIST = 400,
    ID_LBL_SNIP_LIST = 401,
    ID_LIST_SNIPPETS = 402,
    ID_BTN_DEL_SNIP = 403,
    ID_HINT_DEL_SNIP = 404,
    ID_GRP_SNIP_ADD = 405,
    ID_LBL_SNIP_TRIG = 406,
    ID_EDIT_TRIG = 407,
    ID_LBL_SNIP_EXP = 408,
    ID_EDIT_EXP = 409,
    ID_BTN_ADD_SNIP = 410,
    ID_HINT_SNIP_HOW = 411,

    // Tab 3 - App Exceptions / Исключения программ
    ID_GRP_APP_LIST = 500,
    ID_LBL_APP_LIST = 501,
    ID_LIST_APPS = 502,
    ID_BTN_DEL_APP = 503,
    ID_HINT_DEL_APP = 504,
    ID_GRP_APP_ADD = 505,
    ID_BTN_BROWSE_APP = 506,
    ID_EDIT_APP = 507,
    ID_COMBO_APP_MODE = 508,
    ID_BTN_ADD_APP = 509,
    ID_HINT_APP_MODES = 510,

    // Tab 4 - Word Exceptions / Исключения слов
    ID_GRP_WORD_LIST = 600,
    ID_LBL_WORD_LIST = 601,
    ID_LIST_WORDS = 602,
    ID_BTN_DEL_WORD = 603,
    ID_HINT_DEL_WORD = 604,
    ID_GRP_WORD_ADD = 605,
    ID_LBL_WORD_ADD = 606,
    ID_EDIT_WORD = 607,
    ID_BTN_ADD_WORD = 608,
    ID_HINT_WORD_FOOTNOTE = 609,

    // Tab 5 - About / О программе
    ID_LBL_ABOUT_TITLE = 700,
    ID_LBL_ABOUT_DESC = 701,
    ID_GRP_ABOUT_LINKS = 702,
    ID_LBL_ABOUT_REPO = 703,
    ID_BTN_ABOUT_REPO = 704,
    ID_LBL_ABOUT_DONATE = 705,
    ID_BTN_ABOUT_DONATE = 706
};

static WNDPROC g_default_edit_proc = nullptr;

static LRESULT CALLBACK CustomEditSubclassProc(HWND hed, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN && wp == VK_RETURN) {
        HWND hparent = GetParent(hed);
        int id = GetDlgCtrlID(hed);
        if (id == ID_EDIT_TRIG || id == ID_EDIT_EXP) {
            SendMessageW(hparent, WM_COMMAND, MAKEWPARAM(ID_BTN_ADD_SNIP, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(hparent, ID_BTN_ADD_SNIP)));
            return 0;
        } else if (id == ID_EDIT_APP) {
            SendMessageW(hparent, WM_COMMAND, MAKEWPARAM(ID_BTN_ADD_APP, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(hparent, ID_BTN_ADD_APP)));
            return 0;
        } else if (id == ID_EDIT_WORD) {
            SendMessageW(hparent, WM_COMMAND, MAKEWPARAM(ID_BTN_ADD_WORD, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(hparent, ID_BTN_ADD_WORD)));
            return 0;
        }
    }
    return CallWindowProcW(g_default_edit_proc, hed, msg, wp, lp);
}

static const struct { int vk; const wchar_t* name; } kHotkeys[] = {
    {VK_PAUSE, L"Pause / Break"},
    {VK_CAPITAL, L"Caps Lock"},
    {VK_SCROLL, L"Scroll Lock"},
    {VK_F1, L"F1"}, {VK_F2, L"F2"}, {VK_F3, L"F3"}, {VK_F4, L"F4"},
    {VK_F6, L"F6"}, {VK_F7, L"F7"}, {VK_F8, L"F8"}, {VK_F9, L"F9"},
    {VK_F10, L"F10"}, {VK_F11, L"F11"}, {VK_F12, L"F12"},
    {L'`', L"`"},
    {VK_OEM_3, L"~ (OEM 3)"}
};

static void UpdateDialogTexts(HWND hwnd, const std::string& lang) {
    SetWindowTextW(hwnd, Tr(StrId::SettingsTitle, lang));

    HWND htab = GetDlgItem(hwnd, ID_TAB);
    if (htab) {
        TCITEMW tie = {};
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<LPWSTR>(Tr(StrId::TabGeneral, lang));
        SendMessageW(htab, TCM_SETITEMW, 0, reinterpret_cast<LPARAM>(&tie));
        tie.pszText = const_cast<LPWSTR>(Tr(StrId::TabHotkeys, lang));
        SendMessageW(htab, TCM_SETITEMW, 1, reinterpret_cast<LPARAM>(&tie));
        tie.pszText = const_cast<LPWSTR>(Tr(StrId::TabSnippets, lang));
        SendMessageW(htab, TCM_SETITEMW, 2, reinterpret_cast<LPARAM>(&tie));
        tie.pszText = const_cast<LPWSTR>(Tr(StrId::TabAppExceptions, lang));
        SendMessageW(htab, TCM_SETITEMW, 3, reinterpret_cast<LPARAM>(&tie));
        tie.pszText = const_cast<LPWSTR>(Tr(StrId::TabWordExceptions, lang));
        SendMessageW(htab, TCM_SETITEMW, 4, reinterpret_cast<LPARAM>(&tie));
        tie.pszText = const_cast<LPWSTR>(Tr(StrId::TabAbout, lang));
        SendMessageW(htab, TCM_SETITEMW, 5, reinterpret_cast<LPARAM>(&tie));
    }

    for (const auto& item : g_controls) {
        if (item.str_id != StrId::Count) {
            SetWindowTextW(item.hwnd, Tr(item.str_id, lang));
        }
    }

    HWND happ_mode_combo = GetDlgItem(hwnd, ID_COMBO_APP_MODE);
    if (happ_mode_combo) {
        int cur_sel = static_cast<int>(SendMessageW(happ_mode_combo, CB_GETCURSEL, 0, 0));
        SendMessageW(happ_mode_combo, CB_RESETCONTENT, 0, 0);
        SendMessageW(happ_mode_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Tr(StrId::AppModeSoft, lang)));
        SendMessageW(happ_mode_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Tr(StrId::AppModeOff, lang)));
        SendMessageW(happ_mode_combo, CB_SETCURSEL, cur_sel >= 0 ? cur_sel : 0, 0);
    }
}

static void SwitchTab(int tab_idx) {
    HWND htab = GetDlgItem(g_dialog_hwnd, ID_TAB);
    for (const auto& item : g_controls) {
        if (item.tab_index >= 0) {
            if (item.tab_index == tab_idx) {
                ShowWindow(item.hwnd, SW_SHOW);
                BringWindowToTop(item.hwnd);
            } else {
                ShowWindow(item.hwnd, SW_HIDE);
            }
        }
    }
    if (htab) {
        SetWindowPos(htab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (g_dialog_hwnd) {
        RedrawWindow(g_dialog_hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

static void PopulateDialog(HWND hwnd) {
    auto& s = Settings::Instance();
    s.autostart = Autostart::IsEnabled();

    // Tab 0 checkboxes
    CheckDlgButton(hwnd, ID_CHK_AUTO, s.auto_enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_SPACE, s.trigger_space ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_ENTER, s.trigger_enter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_TAB, s.trigger_tab ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_ARROWS, s.arrows_cancel ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_TYPO, s.typofix_enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_DOUBLESPACE, s.double_space_period ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_CAPSREMAP, s.caps_remap_enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_SOUND, s.sound_enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_AUTOSTART, s.autostart ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_LOG, s.write_log ? BST_CHECKED : BST_UNCHECKED);

    // Tab 0 language combo
    HWND hlang = GetDlgItem(hwnd, ID_COMBO_LANG);
    if (hlang) {
        SendMessageW(hlang, CB_RESETCONTENT, 0, 0);
        SendMessageW(hlang, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"English"));
        SendMessageW(hlang, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Русский"));
        SendMessageW(hlang, CB_SETCURSEL, (s.language == "ru") ? 1 : 0, 0);
    }

    // Tab 1 hotkeys
    HWND hcombo = GetDlgItem(hwnd, ID_COMBO_HOTKEY);
    SendMessageW(hcombo, CB_RESETCONTENT, 0, 0);
    int sel_idx = 0;
    for (size_t i = 0; i < sizeof(kHotkeys)/sizeof(kHotkeys[0]); ++i) {
        SendMessageW(hcombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kHotkeys[i].name));
        if (kHotkeys[i].vk == s.hotkey_vk) {
            sel_idx = static_cast<int>(i);
        }
    }
    SendMessageW(hcombo, CB_SETCURSEL, sel_idx, 0);

    CheckDlgButton(hwnd, ID_CHK_CTRL, (s.hotkey_mods & Settings::ModCtrl) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_ALT, (s.hotkey_mods & Settings::ModAlt) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_SHIFT, (s.hotkey_mods & Settings::ModShift) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, ID_CHK_WIN, (s.hotkey_mods & Settings::ModWin) ? BST_CHECKED : BST_UNCHECKED);

    // Tab 2 snippets
    HWND hsnip = GetDlgItem(hwnd, ID_LIST_SNIPPETS);
    if (hsnip) {
        SendMessageW(hsnip, WM_SETREDRAW, FALSE, 0);
        SendMessageW(hsnip, LB_RESETCONTENT, 0, 0);
        for (const auto& [trig, exp] : s.snippets) {
            std::wstring line = trig + L"  ->  " + exp;
            SendMessageW(hsnip, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
        SendMessageW(hsnip, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hsnip, nullptr, TRUE);
    }

    // Tab 3 apps
    HWND happs = GetDlgItem(hwnd, ID_LIST_APPS);
    if (happs) {
        SendMessageW(happs, WM_SETREDRAW, FALSE, 0);
        SendMessageW(happs, LB_RESETCONTENT, 0, 0);
        for (const auto& [app, mode] : s.app_modes) {
            std::wstring mode_desc = (mode == L"off") ? L"  [Off]" : L"  [Soft]";
            std::wstring line = app + mode_desc;
            SendMessageW(happs, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
        SendMessageW(happs, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(happs, nullptr, TRUE);
    }

    // Tab 4 words
    HWND hwords = GetDlgItem(hwnd, ID_LIST_WORDS);
    if (hwords) {
        SendMessageW(hwords, WM_SETREDRAW, FALSE, 0);
        SendMessageW(hwords, LB_RESETCONTENT, 0, 0);
        std::vector<std::wstring> sorted_words(s.ignored_words.begin(), s.ignored_words.end());
        std::sort(sorted_words.begin(), sorted_words.end());
        for (const auto& w : sorted_words) {
            SendMessageW(hwords, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(w.c_str()));
        }
        SendMessageW(hwords, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hwords, nullptr, TRUE);
    }

    UpdateDialogTexts(hwnd, s.language);
}

static void SaveDialog(HWND hwnd) {
    auto& s = Settings::Instance();

    s.auto_enabled = (IsDlgButtonChecked(hwnd, ID_CHK_AUTO) == BST_CHECKED);
    s.trigger_space = (IsDlgButtonChecked(hwnd, ID_CHK_SPACE) == BST_CHECKED);
    s.trigger_enter = (IsDlgButtonChecked(hwnd, ID_CHK_ENTER) == BST_CHECKED);
    s.trigger_tab = (IsDlgButtonChecked(hwnd, ID_CHK_TAB) == BST_CHECKED);
    s.arrows_cancel = (IsDlgButtonChecked(hwnd, ID_CHK_ARROWS) == BST_CHECKED);
    s.typofix_enabled = (IsDlgButtonChecked(hwnd, ID_CHK_TYPO) == BST_CHECKED);
    s.double_space_period = (IsDlgButtonChecked(hwnd, ID_CHK_DOUBLESPACE) == BST_CHECKED);
    s.caps_remap_enabled = (IsDlgButtonChecked(hwnd, ID_CHK_CAPSREMAP) == BST_CHECKED);
    s.sound_enabled = (IsDlgButtonChecked(hwnd, ID_CHK_SOUND) == BST_CHECKED);
    s.autostart = (IsDlgButtonChecked(hwnd, ID_CHK_AUTOSTART) == BST_CHECKED);
    s.write_log = (IsDlgButtonChecked(hwnd, ID_CHK_LOG) == BST_CHECKED);

    HWND hlang = GetDlgItem(hwnd, ID_COMBO_LANG);
    if (hlang) {
        int sel = static_cast<int>(SendMessageW(hlang, CB_GETCURSEL, 0, 0));
        s.language = (sel == 1) ? "ru" : "en";
    }

    HWND hcombo = GetDlgItem(hwnd, ID_COMBO_HOTKEY);
    int cur_sel = static_cast<int>(SendMessageW(hcombo, CB_GETCURSEL, 0, 0));
    if (cur_sel >= 0 && cur_sel < static_cast<int>(sizeof(kHotkeys)/sizeof(kHotkeys[0]))) {
        s.hotkey_vk = kHotkeys[cur_sel].vk;
    }

    s.hotkey_mods = 0;
    if (IsDlgButtonChecked(hwnd, ID_CHK_CTRL) == BST_CHECKED) s.hotkey_mods |= Settings::ModCtrl;
    if (IsDlgButtonChecked(hwnd, ID_CHK_ALT) == BST_CHECKED) s.hotkey_mods |= Settings::ModAlt;
    if (IsDlgButtonChecked(hwnd, ID_CHK_SHIFT) == BST_CHECKED) s.hotkey_mods |= Settings::ModShift;
    if (IsDlgButtonChecked(hwnd, ID_CHK_WIN) == BST_CHECKED) s.hotkey_mods |= Settings::ModWin;

    Autostart::SetEnabled(s.autostart);
    SoundEffect::Instance().SetEnabled(s.sound_enabled);
    SnippetStore::Instance().SetSnippets(s.snippets);

    s.Save();
    TrayIcon::Instance().UpdateState(s.auto_enabled, Engine::Instance().IsPaused());
    TrayIcon::Instance().ShowNotification(L"Ultimakey", Tr(StrId::NotifSettingsSaved));
}

LRESULT CALLBACK SettingsDialog::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE hinst = reinterpret_cast<LPCREATESTRUCT>(lparam)->hInstance;

            // DPI awareness
            UINT dpi = 96;
            HMODULE u32 = GetModuleHandleW(L"user32.dll");
            auto pGetDpiForWindow = reinterpret_cast<UINT(WINAPI*)(HWND)>(reinterpret_cast<void*>(GetProcAddress(u32, "GetDpiForWindow")));
            if (pGetDpiForWindow) {
                dpi = pGetDpiForWindow(hwnd);
            } else {
                HDC hdc = GetDC(hwnd);
                dpi = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(hwnd, hdc);
            }
            if (dpi == 0) dpi = 96;

            auto s = [dpi](int val) { return MulDiv(val, static_cast<int>(dpi), 96); };

            if (!g_dialog_font) {
                g_dialog_font = CreateFontW(s(-12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                if (!g_dialog_font) {
                    g_dialog_font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                }
            }
            if (!g_title_font) {
                g_title_font = CreateFontW(s(-16), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }
            HFONT hfont = g_dialog_font;

            EnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            g_controls.clear();

            // Tab Control
            HWND htab = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
                                        s(12), s(10), s(715), s(455), hwnd, reinterpret_cast<HMENU>(ID_TAB), hinst, nullptr);
            SendMessageW(htab, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), TRUE);
            SetWindowPos(htab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            TCITEMW tie = {};
            tie.mask = TCIF_TEXT;
            tie.pszText = const_cast<LPWSTR>(L"");
            for (int i = 0; i < 6; ++i) {
                SendMessageW(htab, TCM_INSERTITEMW, i, reinterpret_cast<LPARAM>(&tie));
            }

            auto add_ctrl = [&](HWND h, int tab_idx, StrId str_id = StrId::Count) {
                SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), TRUE);
                g_controls.push_back({h, tab_idx, str_id});
                return h;
            };

            auto hook_edit = [&](HWND hed) {
                if (!g_default_edit_proc) {
                    g_default_edit_proc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hed, GWLP_WNDPROC));
                }
                SetWindowLongPtrW(hed, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CustomEditSubclassProc));
                return hed;
            };

            // ==========================================
            // TAB 0 Controls: General / Основные
            // ==========================================
            int y = s(38);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_AUTO), hinst, nullptr), 0, StrId::OptAutoEnabled); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(45), y, s(650), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SPACE), hinst, nullptr), 0, StrId::OptTriggerSpace); y += s(20);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(45), y, s(650), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ENTER), hinst, nullptr), 0, StrId::OptTriggerEnter); y += s(20);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(45), y, s(650), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_TAB), hinst, nullptr), 0, StrId::OptTriggerTab); y += s(23);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ARROWS), hinst, nullptr), 0, StrId::OptArrowsCancel); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_TYPO), hinst, nullptr), 0, StrId::OptTypofix); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_DOUBLESPACE), hinst, nullptr), 0, StrId::OptDoubleSpace); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_CAPSREMAP), hinst, nullptr), 0, StrId::OptCapsRemap); y += s(22);

            // Sound: Checkbox + Play button
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(400), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SOUND), hinst, nullptr), 0, StrId::OptSound);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"▶", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(430), y - s(1), s(32), s(22), hwnd, reinterpret_cast<HMENU>(ID_BTN_TEST_SOUND), hinst, nullptr), 0, StrId::BtnListen);
            y += s(24);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_AUTOSTART), hinst, nullptr), 0, StrId::OptStartup); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_LOG), hinst, nullptr), 0, StrId::OptLog); y += s(28);

            // Interface Language Selector
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(25), y + s(2), s(140), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_LANG), hinst, nullptr), 0, StrId::LblLanguage);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, s(170), y, s(160), s(100), hwnd, reinterpret_cast<HMENU>(ID_COMBO_LANG), hinst, nullptr), 0);

            // ==========================================
            // TAB 1 Controls: Hotkeys / Горячие клавиши
            // ==========================================
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(25), s(50), s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_HOTKEY), hinst, nullptr), 1, StrId::LblHotkeySwitch);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, s(25), s(75), s(280), s(200), hwnd, reinterpret_cast<HMENU>(ID_COMBO_HOTKEY), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(25), s(115), s(670), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_MODS), hinst, nullptr), 1, StrId::LblHotkeyMods);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Ctrl", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_CTRL), hinst, nullptr), 1, StrId::ModCtrl);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Alt", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(120), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ALT), hinst, nullptr), 1, StrId::ModAlt);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Shift", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(215), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SHIFT), hinst, nullptr), 1, StrId::ModShift);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Win", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(310), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_WIN), hinst, nullptr), 1, StrId::ModWin);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(25), s(185), s(670), s(45), hwnd, reinterpret_cast<HMENU>(ID_HINT_HOTKEY), hinst, nullptr), 1, StrId::HintHotkeySelection);

            // ==========================================
            // TAB 2 Controls: Snippets / Автозамена текста
            // ==========================================
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(40), s(695), s(195), hwnd, reinterpret_cast<HMENU>(ID_GRP_SNIP_LIST), hinst, nullptr), 2, StrId::GrpSnippetsList);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(58), s(535), s(18), hwnd, reinterpret_cast<HMENU>(ID_LBL_SNIP_LIST), hinst, nullptr), 2, StrId::LblSnippetsList);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP, s(35), s(78), s(535), s(145), hwnd, reinterpret_cast<HMENU>(ID_LIST_SNIPPETS), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(590), s(78), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_SNIP), hinst, nullptr), 2, StrId::BtnDelete);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(590), s(115), s(115), s(45), hwnd, reinterpret_cast<HMENU>(ID_HINT_DEL_SNIP), hinst, nullptr), 2, StrId::HintDeleteSnippet);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(248), s(695), s(195), hwnd, reinterpret_cast<HMENU>(ID_GRP_SNIP_ADD), hinst, nullptr), 2, StrId::GrpSnippetsAdd);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(268), s(255), s(18), hwnd, reinterpret_cast<HMENU>(ID_LBL_SNIP_TRIG), hinst, nullptr), 2, StrId::LblSnippetTrigger);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, s(35), s(288), s(255), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_TRIG), hinst, nullptr)), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(305), s(268), s(265), s(18), hwnd, reinterpret_cast<HMENU>(ID_LBL_SNIP_EXP), hinst, nullptr), 2, StrId::LblSnippetReplacement);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, s(305), s(288), s(265), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_EXP), hinst, nullptr)), 2);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(590), s(286), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_SNIP), hinst, nullptr), 2, StrId::BtnAdd);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(325), s(670), s(36), hwnd, reinterpret_cast<HMENU>(ID_HINT_SNIP_HOW), hinst, nullptr), 2, StrId::HintSnippetsHow);

            // ==========================================
            // TAB 3 Controls: App Exceptions / Исключения программ
            // ==========================================
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(40), s(695), s(195), hwnd, reinterpret_cast<HMENU>(ID_GRP_APP_LIST), hinst, nullptr), 3, StrId::GrpAppList);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(58), s(535), s(18), hwnd, reinterpret_cast<HMENU>(ID_LBL_APP_LIST), hinst, nullptr), 3, StrId::LblAppList);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP, s(35), s(78), s(535), s(145), hwnd, reinterpret_cast<HMENU>(ID_LIST_APPS), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(590), s(78), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_APP), hinst, nullptr), 3, StrId::BtnDelete);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(590), s(115), s(115), s(45), hwnd, reinterpret_cast<HMENU>(ID_HINT_DEL_APP), hinst, nullptr), 3, StrId::HintDeleteApp);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(248), s(695), s(195), hwnd, reinterpret_cast<HMENU>(ID_GRP_APP_ADD), hinst, nullptr), 3, StrId::GrpAppAdd);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(35), s(278), s(175), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_BROWSE_APP), hinst, nullptr), 3, StrId::BtnBrowse);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, s(220), s(281), s(190), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_APP), hinst, nullptr)), 3);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, s(420), s(281), s(150), s(120), hwnd, reinterpret_cast<HMENU>(ID_COMBO_APP_MODE), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(590), s(278), s(115), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_APP), hinst, nullptr), 3, StrId::BtnAdd);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(322), s(670), s(45), hwnd, reinterpret_cast<HMENU>(ID_HINT_APP_MODES), hinst, nullptr), 3, StrId::HintAppModes);

            // ==========================================
            // TAB 4 Controls: Word Exceptions / Слова, которые не исправляются
            // ==========================================
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(40), s(695), s(195), hwnd, reinterpret_cast<HMENU>(ID_GRP_WORD_LIST), hinst, nullptr), 4, StrId::GrpWordList);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(58), s(535), s(18), hwnd, reinterpret_cast<HMENU>(ID_LBL_WORD_LIST), hinst, nullptr), 4, StrId::LblWordList);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP, s(35), s(78), s(535), s(145), hwnd, reinterpret_cast<HMENU>(ID_LIST_WORDS), hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(590), s(78), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_WORD), hinst, nullptr), 4, StrId::BtnDelete);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(590), s(115), s(115), s(45), hwnd, reinterpret_cast<HMENU>(ID_HINT_DEL_WORD), hinst, nullptr), 4, StrId::HintDeleteWord);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(248), s(695), s(195), hwnd, reinterpret_cast<HMENU>(ID_GRP_WORD_ADD), hinst, nullptr), 4, StrId::GrpWordAdd);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(268), s(535), s(18), hwnd, reinterpret_cast<HMENU>(ID_LBL_WORD_ADD), hinst, nullptr), 4, StrId::LblWordAdd);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, s(35), s(288), s(535), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_WORD), hinst, nullptr)), 4);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(590), s(286), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_WORD), hinst, nullptr), 4, StrId::BtnAdd);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(325), s(670), s(36), hwnd, reinterpret_cast<HMENU>(ID_HINT_WORD_FOOTNOTE), hinst, nullptr), 4, StrId::HintWordFootnote);

            // ==========================================
            // TAB 5 Controls: About / О программе
            // ==========================================
            HWND htitle = add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(45), s(670), s(28), hwnd, reinterpret_cast<HMENU>(ID_LBL_ABOUT_TITLE), hinst, nullptr), 5, StrId::AboutTitle);
            if (g_title_font) {
                SendMessageW(htitle, WM_SETFONT, reinterpret_cast<WPARAM>(g_title_font), TRUE);
            }
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(35), s(78), s(670), s(36), hwnd, reinterpret_cast<HMENU>(ID_LBL_ABOUT_DESC), hinst, nullptr), 5, StrId::AboutDesc);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_GROUPBOX, s(22), s(125), s(695), s(318), hwnd, reinterpret_cast<HMENU>(ID_GRP_ABOUT_LINKS), hinst, nullptr), 5, StrId::GrpAboutLinks);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(40), s(155), s(650), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_ABOUT_REPO), hinst, nullptr), 5, StrId::LblAboutRepo);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(40), s(180), s(420), s(36), hwnd, reinterpret_cast<HMENU>(ID_BTN_ABOUT_REPO), hinst, nullptr), 5, StrId::BtnAboutRepo);

            add_ctrl(CreateWindowExW(0, L"STATIC", L"", WS_CHILD, s(40), s(235), s(650), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_ABOUT_DONATE), hinst, nullptr), 5, StrId::LblAboutDonate);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(40), s(260), s(420), s(36), hwnd, reinterpret_cast<HMENU>(ID_BTN_ABOUT_DONATE), hinst, nullptr), 5, StrId::BtnAboutDonate);

            // ==========================================
            // Bottom Buttons (always visible)
            // ==========================================
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, s(505), s(477), s(105), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_SAVE), hinst, nullptr), -1, StrId::BtnSave);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, s(620), s(477), s(105), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_CANCEL), hinst, nullptr), -1, StrId::BtnCancel);

            PopulateDialog(hwnd);
            SwitchTab(0);
            return 0;
        }

        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));
            return 1;
        }

        case WM_CTLCOLORDLG:
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));

        case WM_CTLCOLORSTATIC: {
            HDC hdc_static = reinterpret_cast<HDC>(wparam);
            SetBkMode(hdc_static, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        case WM_NOTIFY: {
            auto nm = reinterpret_cast<LPNMHDR>(lparam);
            if (nm->idFrom == ID_TAB && nm->code == TCN_SELCHANGE) {
                int cur_tab = static_cast<int>(SendMessageW(nm->hwndFrom, TCM_GETCURSEL, 0, 0));
                SwitchTab(cur_tab);
                return 0;
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);
            int code = HIWORD(wparam);

            if (id == ID_COMBO_LANG && code == CBN_SELCHANGE) {
                HWND hcombo = GetDlgItem(hwnd, ID_COMBO_LANG);
                int sel = static_cast<int>(SendMessageW(hcombo, CB_GETCURSEL, 0, 0));
                std::string new_lang = (sel == 1) ? "ru" : "en";
                UpdateDialogTexts(hwnd, new_lang);
                RedrawWindow(hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                return 0;
            }

            if (code == LBN_SELCHANGE) {
                if (id == ID_LIST_SNIPPETS) {
                    HWND hsnip = GetDlgItem(hwnd, ID_LIST_SNIPPETS);
                    int sel = static_cast<int>(SendMessageW(hsnip, LB_GETCURSEL, 0, 0));
                    if (sel >= 0 && sel < static_cast<int>(Settings::Instance().snippets.size())) {
                        const auto& [trig, exp] = Settings::Instance().snippets[sel];
                        SetDlgItemTextW(hwnd, ID_EDIT_TRIG, trig.c_str());
                        SetDlgItemTextW(hwnd, ID_EDIT_EXP, exp.c_str());
                    }
                    return 0;
                } else if (id == ID_LIST_APPS) {
                    HWND happs = GetDlgItem(hwnd, ID_LIST_APPS);
                    int sel = static_cast<int>(SendMessageW(happs, LB_GETCURSEL, 0, 0));
                    if (sel >= 0) {
                        wchar_t text[256] = {};
                        if (SendMessageW(happs, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(text)) != LB_ERR) {
                            std::wstring_view full(text);
                            size_t sp = full.rfind(L"  [");
                            if (sp != std::wstring_view::npos) {
                                std::wstring app_key(full.substr(0, sp));
                                SetDlgItemTextW(hwnd, ID_EDIT_APP, app_key.c_str());
                                bool is_off = (full.find(L"[Off]") != std::wstring_view::npos || full.find(L"[Отключено]") != std::wstring_view::npos);
                                SendMessageW(GetDlgItem(hwnd, ID_COMBO_APP_MODE), CB_SETCURSEL, is_off ? 1 : 0, 0);
                            }
                        }
                    }
                    return 0;
                } else if (id == ID_LIST_WORDS) {
                    HWND hwords = GetDlgItem(hwnd, ID_LIST_WORDS);
                    int sel = static_cast<int>(SendMessageW(hwords, LB_GETCURSEL, 0, 0));
                    if (sel >= 0) {
                        wchar_t text[256] = {};
                        if (SendMessageW(hwords, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(text)) != LB_ERR) {
                            SetDlgItemTextW(hwnd, ID_EDIT_WORD, text);
                        }
                    }
                    return 0;
                }
            } else if (code == LBN_DBLCLK && id == ID_LIST_APPS) {
                HWND happs = GetDlgItem(hwnd, ID_LIST_APPS);
                int sel = static_cast<int>(SendMessageW(happs, LB_GETCURSEL, 0, 0));
                if (sel >= 0) {
                    wchar_t text[256] = {};
                    if (SendMessageW(happs, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(text)) != LB_ERR) {
                        std::wstring_view full(text);
                        size_t sp = full.rfind(L"  [");
                        if (sp != std::wstring_view::npos) {
                            std::wstring app_key = ToLower(full.substr(0, sp));
                            auto it = Settings::Instance().app_modes.find(app_key);
                            if (it != Settings::Instance().app_modes.end()) {
                                it->second = (it->second == L"off") ? L"soft" : L"off";
                                Settings::Instance().Save();
                                PopulateDialog(hwnd);
                                SendMessageW(happs, LB_SETCURSEL, sel, 0);
                            }
                        }
                    }
                }
                return 0;
            }

            if (id == ID_BTN_SAVE) {
                SaveDialog(hwnd);
                DestroyWindow(hwnd);
                return 0;
            } else if (id == ID_BTN_CANCEL || id == 2 /*IDCANCEL*/) {
                DestroyWindow(hwnd);
                return 0;
            } else if (id == ID_BTN_TEST_SOUND) {
                SoundEffect::Instance().PlayTestSound();
                return 0;
            } else if (id == ID_BTN_ABOUT_REPO) {
                ShellExecuteW(nullptr, L"open", L"https://github.com/ggghbc/Ultimakey", nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            } else if (id == ID_BTN_ABOUT_DONATE) {
                ShellExecuteW(nullptr, L"open", L"https://boosty.to/ggghbc", nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            } else if (id == ID_BTN_ADD_SNIP) {
                wchar_t trig[128] = {}, exp[2048] = {};
                GetDlgItemTextW(hwnd, ID_EDIT_TRIG, trig, 128);
                GetDlgItemTextW(hwnd, ID_EDIT_EXP, exp, 2048);
                std::wstring trig_str(trig);
                while (!trig_str.empty() && iswspace(trig_str.front())) trig_str.erase(0, 1);
                while (!trig_str.empty() && iswspace(trig_str.back())) trig_str.pop_back();
                if (!trig_str.empty() && wcslen(exp) > 0) {
                    bool found = false;
                    for (auto& [t, e] : Settings::Instance().snippets) {
                        if (t == trig_str) {
                            e = exp;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        Settings::Instance().snippets.emplace_back(trig_str, exp);
                    }
                    SnippetStore::Instance().SetSnippets(Settings::Instance().snippets);
                    Settings::Instance().Save();

                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_TRIG, L"");
                    SetDlgItemTextW(hwnd, ID_EDIT_EXP, L"");
                    SetFocus(GetDlgItem(hwnd, ID_EDIT_TRIG));
                }
                return 0;
            } else if (id == ID_BTN_DEL_SNIP) {
                HWND hsnip = GetDlgItem(hwnd, ID_LIST_SNIPPETS);
                int sel = static_cast<int>(SendMessageW(hsnip, LB_GETCURSEL, 0, 0));
                if (sel >= 0 && sel < static_cast<int>(Settings::Instance().snippets.size())) {
                    Settings::Instance().snippets.erase(Settings::Instance().snippets.begin() + sel);
                    SnippetStore::Instance().SetSnippets(Settings::Instance().snippets);
                    Settings::Instance().Save();

                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_TRIG, L"");
                    SetDlgItemTextW(hwnd, ID_EDIT_EXP, L"");
                }
                return 0;
            } else if (id == ID_BTN_BROWSE_APP) {
                wchar_t file_path[MAX_PATH] = {};
                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Programs (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = file_path;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
                if (GetOpenFileNameW(&ofn)) {
                    std::wstring full_path(file_path);
                    size_t last_slash = full_path.find_last_of(L"\\/");
                    std::wstring file_name = (last_slash != std::wstring_view::npos) ? full_path.substr(last_slash + 1) : full_path;
                    if (file_name.size() > 4 && ToLower(file_name.substr(file_name.size() - 4)) == L".exe") {
                        file_name = file_name.substr(0, file_name.size() - 4);
                    }
                    SetDlgItemTextW(hwnd, ID_EDIT_APP, file_name.c_str());
                }
                return 0;
            } else if (id == ID_BTN_ADD_APP) {
                wchar_t app_name[128] = {};
                GetDlgItemTextW(hwnd, ID_EDIT_APP, app_name, 128);
                HWND happ_mode_combo = GetDlgItem(hwnd, ID_COMBO_APP_MODE);
                int cur_mode_sel = static_cast<int>(SendMessageW(happ_mode_combo, CB_GETCURSEL, 0, 0));
                const wchar_t* mode_code = (cur_mode_sel == 1) ? L"off" : L"soft";
                std::wstring name_str(app_name);
                while (!name_str.empty() && iswspace(name_str.front())) name_str.erase(0, 1);
                while (!name_str.empty() && iswspace(name_str.back())) name_str.pop_back();
                if (name_str.size() > 4 && ToLower(name_str.substr(name_str.size() - 4)) == L".exe") {
                    name_str = name_str.substr(0, name_str.size() - 4);
                }
                if (!name_str.empty()) {
                    Settings::Instance().app_modes[ToLower(name_str)] = mode_code;
                    Settings::Instance().Save();
                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_APP, L"");
                    SetFocus(GetDlgItem(hwnd, ID_EDIT_APP));
                }
                return 0;
            } else if (id == ID_BTN_DEL_APP) {
                HWND happs = GetDlgItem(hwnd, ID_LIST_APPS);
                int sel = static_cast<int>(SendMessageW(happs, LB_GETCURSEL, 0, 0));
                if (sel >= 0) {
                    int len = static_cast<int>(SendMessageW(happs, LB_GETTEXTLEN, sel, 0));
                    std::vector<wchar_t> text(len + 1);
                    SendMessageW(happs, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(text.data()));
                    std::wstring full(text.data());
                    size_t sp = full.rfind(L"  [");
                    if (sp != std::wstring::npos) {
                        std::wstring app_key = full.substr(0, sp);
                        Settings::Instance().app_modes.erase(ToLower(app_key));
                        Settings::Instance().Save();
                        PopulateDialog(hwnd);
                        SetDlgItemTextW(hwnd, ID_EDIT_APP, L"");
                    }
                }
                return 0;
            } else if (id == ID_BTN_ADD_WORD) {
                wchar_t word[128] = {};
                GetDlgItemTextW(hwnd, ID_EDIT_WORD, word, 128);
                std::wstring w(word);
                while (!w.empty() && iswspace(w.front())) w.erase(0, 1);
                while (!w.empty() && iswspace(w.back())) w.pop_back();
                if (!w.empty() && w.front() == L'.') {
                    w.erase(0, 1);
                }
                if (!w.empty()) {
                    Settings::Instance().ignored_words.insert(ToLower(w));
                    Settings::Instance().Save();
                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_WORD, L"");
                    SetFocus(GetDlgItem(hwnd, ID_EDIT_WORD));
                }
                return 0;
            } else if (id == ID_BTN_DEL_WORD) {
                HWND hwords = GetDlgItem(hwnd, ID_LIST_WORDS);
                int sel = static_cast<int>(SendMessageW(hwords, LB_GETCURSEL, 0, 0));
                if (sel >= 0) {
                    int len = static_cast<int>(SendMessageW(hwords, LB_GETTEXTLEN, sel, 0));
                    std::vector<wchar_t> text(len + 1);
                    SendMessageW(hwords, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(text.data()));
                    std::wstring word(text.data());
                    Settings::Instance().ignored_words.erase(ToLower(word));
                    Settings::Instance().Save();
                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_WORD, L"");
                }
                return 0;
            }
            break;
        }

        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            HookManager::Instance().ResumeMouseHook();
            g_dialog_hwnd = nullptr;
            g_default_edit_proc = nullptr;
            if (g_dialog_font) {
                DeleteObject(g_dialog_font);
                g_dialog_font = nullptr;
            }
            if (g_title_font) {
                DeleteObject(g_title_font);
                g_title_font = nullptr;
            }
            g_controls.clear();
            SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void SettingsDialog::Show(HWND parent_hwnd, HINSTANCE hinstance) {
    if (g_dialog_hwnd) {
        SetForegroundWindow(g_dialog_hwnd);
        return;
    }

    HookManager::Instance().SuspendMouseHook();

    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_TAB_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t* kClassName = L"Ultimakey_Settings_Class";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hinstance;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);

    UINT dpi = 96;
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    auto pGetDpiForSystem = reinterpret_cast<UINT(WINAPI*)()>(reinterpret_cast<void*>(GetProcAddress(u32, "GetDpiForSystem")));
    if (pGetDpiForSystem) {
        dpi = pGetDpiForSystem();
    } else {
        HDC hdc = GetDC(nullptr);
        dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(nullptr, hdc);
    }
    if (dpi == 0) dpi = 96;

    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;

    RECT rc = { 0, 0, MulDiv(750, static_cast<int>(dpi), 96), MulDiv(535, static_cast<int>(dpi), 96) };
    AdjustWindowRectEx(&rc, style, FALSE, ex_style);
    int win_w = rc.right - rc.left;
    int win_h = rc.bottom - rc.top;

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    g_dialog_hwnd = CreateWindowExW(ex_style,
                                   kClassName, Tr(StrId::SettingsTitle),
                                   style | WS_VISIBLE,
                                   win_x, win_y, win_w, win_h,
                                   nullptr, nullptr, hinstance, nullptr);

    SetForegroundWindow(g_dialog_hwnd);
}

} // namespace Ultimakey
