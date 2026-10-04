#include "ui_settings.hpp"
#include "settings.hpp"
#include "autostart.hpp"
#include "snippets.hpp"
#include "sound.hpp"
#include "tray.hpp"
#include "engine.hpp"
#include "hook.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <vector>
#include <algorithm>
#include <cwctype>

namespace Ultimakey {

static HWND g_dialog_hwnd = nullptr;
static HFONT g_dialog_font = nullptr;

HWND SettingsDialog::GetHwnd() noexcept {
    return g_dialog_hwnd;
}

struct ControlItem {
    HWND hwnd;
    int tab_index; // -1 = always visible (e.g. Save / Cancel / TabControl)
};

static std::vector<ControlItem> g_controls;

// Control IDs
enum CtrlId {
    ID_TAB = 100,
    ID_BTN_SAVE = 101,
    ID_BTN_CANCEL = 102,

    // Tab 0 - Основные
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

    // Tab 1 - Горячие клавиши
    ID_LBL_HOTKEY = 301,
    ID_COMBO_HOTKEY,
    ID_CHK_CTRL,
    ID_CHK_ALT,
    ID_CHK_SHIFT,
    ID_CHK_WIN,

    // Tab 2 - Автозамена текста
    ID_GRP_SNIP_LIST = 400,
    ID_LIST_SNIPPETS = 401,
    ID_BTN_DEL_SNIP = 402,
    ID_GRP_SNIP_ADD = 403,
    ID_EDIT_TRIG = 404,
    ID_EDIT_EXP = 405,
    ID_BTN_ADD_SNIP = 406,

    // Tab 3 - Исключения программ
    ID_GRP_APP_LIST = 500,
    ID_LIST_APPS = 501,
    ID_BTN_DEL_APP = 502,
    ID_GRP_APP_ADD = 503,
    ID_EDIT_APP = 504,
    ID_COMBO_APP_MODE = 505,
    ID_BTN_ADD_APP = 506,
    ID_BTN_BROWSE_APP = 507,

    // Tab 4 - Исключения слов
    ID_GRP_WORD_LIST = 600,
    ID_LIST_WORDS = 601,
    ID_BTN_DEL_WORD = 602,
    ID_GRP_WORD_ADD = 603,
    ID_EDIT_WORD = 604,
    ID_BTN_ADD_WORD = 605
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
    {L'`', L"` (Тильда)"},
    {VK_OEM_3, L"~ (OEM 3)"}
};

static void SwitchTab(int tab_idx) {
    for (const auto& item : g_controls) {
        if (item.tab_index >= 0) {
            ShowWindow(item.hwnd, (item.tab_index == tab_idx) ? SW_SHOW : SW_HIDE);
        }
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
            std::wstring mode_desc = (mode == L"off") ? L"[Отключено]" : L"[Мягкий режим]";
            std::wstring line = app + L"  " + mode_desc;
            SendMessageW(happs, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
        SendMessageW(happs, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(happs, nullptr, TRUE);
    }

    HWND happ_mode_combo = GetDlgItem(hwnd, ID_COMBO_APP_MODE);
    if (happ_mode_combo) {
        SendMessageW(happ_mode_combo, CB_RESETCONTENT, 0, 0);
        SendMessageW(happ_mode_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Мягкий режим"));
        SendMessageW(happ_mode_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Отключено"));
        SendMessageW(happ_mode_combo, CB_SETCURSEL, 0, 0);
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
    TrayIcon::Instance().ShowNotification(L"Ultimakey", L"Настройки сохранены");
}

LRESULT CALLBACK SettingsDialog::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE hinst = reinterpret_cast<LPCREATESTRUCT>(lparam)->hInstance;

            // PerMonitorV2 DPI
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
            HFONT hfont = g_dialog_font;

            EnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            g_controls.clear();

            // Tab Control
            HWND htab = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
                                        s(12), s(10), s(645), s(450), hwnd, reinterpret_cast<HMENU>(ID_TAB), hinst, nullptr);
            SendMessageW(htab, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), TRUE);
            SetWindowPos(htab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            TCITEMW tie = {};
            tie.mask = TCIF_TEXT;
            tie.pszText = const_cast<LPWSTR>(L"Основные");
            SendMessageW(htab, TCM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tie));
            tie.pszText = const_cast<LPWSTR>(L"Горячие клавиши");
            SendMessageW(htab, TCM_INSERTITEMW, 1, reinterpret_cast<LPARAM>(&tie));
            tie.pszText = const_cast<LPWSTR>(L"Автозамена текста");
            SendMessageW(htab, TCM_INSERTITEMW, 2, reinterpret_cast<LPARAM>(&tie));
            tie.pszText = const_cast<LPWSTR>(L"Исключения программ");
            SendMessageW(htab, TCM_INSERTITEMW, 3, reinterpret_cast<LPARAM>(&tie));
            tie.pszText = const_cast<LPWSTR>(L"Исключения слов");
            SendMessageW(htab, TCM_INSERTITEMW, 4, reinterpret_cast<LPARAM>(&tie));

            auto add_ctrl = [&](HWND h, int tab_idx) {
                SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), TRUE);
                g_controls.push_back({h, tab_idx});
                return h;
            };

            auto hook_edit = [&](HWND hed) {
                if (!g_default_edit_proc) {
                    g_default_edit_proc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hed, GWLP_WNDPROC));
                }
                SetWindowLongPtrW(hed, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CustomEditSubclassProc));
                return hed;
            };

            // TAB 0 Controls
            int y = s(42);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Автоматически исправлять неверную раскладку", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_AUTO), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"  • При нажатии Пробела", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(45), y, s(560), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SPACE), hinst, nullptr), 0); y += s(20);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"  • При нажатии Enter", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(45), y, s(560), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ENTER), hinst, nullptr), 0); y += s(20);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"  • При нажатии Tab", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(45), y, s(560), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_TAB), hinst, nullptr), 0); y += s(23);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Отменять исправление при перемещении стрелками", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ARROWS), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Автоматически исправлять частые опечатки", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_TYPO), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Двойной пробел заменять на точку и пробел (. )", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_DOUBLESPACE), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Клавиша CapsLock переключает раскладку (без фиксации)", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_CAPSREMAP), hinst, nullptr), 0); y += s(22);

            // Sound: checkbox + compact play icon button
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Звуковой щелчок при исправлении слова", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(350), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SOUND), hinst, nullptr), 0);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"▶", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, s(380), y - s(1), s(30), s(22), hwnd, reinterpret_cast<HMENU>(ID_BTN_TEST_SOUND), hinst, nullptr), 0);
            y += s(24);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Запускать Ultimakey при входе в Windows", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_AUTOSTART), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Вести журнал работы программы для отладки", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), y, s(580), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_LOG), hinst, nullptr), 0);

            // TAB 1 Controls (Hotkeys)
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Клавиша для смены языка слова или выделенного текста:", WS_CHILD, s(25), s(50), s(600), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_HOTKEY), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, s(25), s(75), s(260), s(200), hwnd, reinterpret_cast<HMENU>(ID_COMBO_HOTKEY), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Дополнительные клавиши-модификаторы:", WS_CHILD, s(25), s(115), s(600), s(20), hwnd, nullptr, hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Ctrl", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(25), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_CTRL), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Alt", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(120), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ALT), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Shift", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(215), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SHIFT), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Win", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, s(310), s(140), s(80), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_WIN), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Совет: если выделить фрагмент текста и нажать горячую клавишу,\nUltimakey изменит раскладку всего выделенного фрагмента.", WS_CHILD, s(25), s(185), s(600), s(40), hwnd, nullptr, hinst, nullptr), 1);

            // TAB 2 Controls (Snippets / Text Replacement) - Crystal Clear GroupBoxes
            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Сохранённые правила автозамены ", WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS, s(22), s(42), s(625), s(200), hwnd, reinterpret_cast<HMENU>(ID_GRP_SNIP_LIST), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Список активных правил (выберите правило для просмотра или удаления):", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(62), s(470), s(18), hwnd, nullptr, hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(35), s(82), s(470), s(148), hwnd, reinterpret_cast<HMENU>(ID_LIST_SNIPPETS), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Удалить", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(520), s(82), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_SNIP), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Выберите правило\nв списке слева,\nчтобы удалить его", WS_CHILD | WS_CLIPSIBLINGS, s(520), s(120), s(115), s(45), hwnd, nullptr, hinst, nullptr), 2);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Добавить или изменить правило ", WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS, s(22), s(252), s(625), s(175), hwnd, reinterpret_cast<HMENU>(ID_GRP_SNIP_ADD), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Что вводите (сокращение):", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(272), s(225), s(18), hwnd, nullptr, hinst, nullptr), 2);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(35), s(292), s(225), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_TRIG), hinst, nullptr)), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"На что заменять (полный текст):", WS_CHILD | WS_CLIPSIBLINGS, s(275), s(272), s(230), s(18), hwnd, nullptr, hinst, nullptr), 2);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(275), s(292), s(230), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_EXP), hinst, nullptr)), 2);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Добавить", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(520), s(290), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_SNIP), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Как это работает: при вводе сокращения (например, 'спс') и нажатии пробела\nпрограмма мгновенно заменит его на полный текст ('Спасибо большое!').", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(330), s(600), s(36), hwnd, nullptr, hinst, nullptr), 2);

            // TAB 3 Controls (Apps Exclusions) - Crystal Clear GroupBoxes
            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Программы с особым режимом работы ", WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS, s(22), s(42), s(625), s(200), hwnd, reinterpret_cast<HMENU>(ID_GRP_APP_LIST), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Список программ-исключений (выберите для просмотра или удаления):", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(62), s(470), s(18), hwnd, nullptr, hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(35), s(82), s(470), s(148), hwnd, reinterpret_cast<HMENU>(ID_LIST_APPS), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Удалить", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(520), s(82), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_APP), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Выберите программу\nв списке слева,\nчтобы удалить её", WS_CHILD | WS_CLIPSIBLINGS, s(520), s(120), s(115), s(45), hwnd, nullptr, hinst, nullptr), 3);

            // TAB 3 Controls (Apps Exclusions - Add Box)
            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Добавить программу в исключения ", WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS, s(22), s(252), s(625), s(175), hwnd, reinterpret_cast<HMENU>(ID_GRP_APP_ADD), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Выбрать .exe файл...", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(35), s(280), s(165), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_BROWSE_APP), hinst, nullptr), 3);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(210), s(283), s(155), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_APP), hinst, nullptr)), 3);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(375), s(283), s(130), s(120), hwnd, reinterpret_cast<HMENU>(ID_COMBO_APP_MODE), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Добавить", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(520), s(280), s(115), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_APP), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Нажмите «Выбрать .exe файл...» для выбора программы через проводник Windows.\n• Мягкий режим: исправляются только длинные слова (от 4 букв).\n• Отключено: автоисправление полностью выключено в этой программе.", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(325), s(600), s(45), hwnd, nullptr, hinst, nullptr), 3);

            // TAB 4 Controls (Words & Extensions Exclusions)
            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Слова и форматы файлов, которые никогда не исправляются ", WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS, s(22), s(42), s(625), s(200), hwnd, reinterpret_cast<HMENU>(ID_GRP_WORD_LIST), hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Список слов-исключений (exe, dll, txt, png, github и др.):", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(62), s(470), s(18), hwnd, nullptr, hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(35), s(82), s(470), s(148), hwnd, reinterpret_cast<HMENU>(ID_LIST_WORDS), hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Удалить", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(520), s(82), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_WORD), hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Выберите слово\nв списке слева,\nчтобы удалить его", WS_CHILD | WS_CLIPSIBLINGS, s(520), s(120), s(115), s(45), hwnd, nullptr, hinst, nullptr), 4);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Добавить слово или расширение в список ", WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS, s(22), s(252), s(625), s(175), hwnd, reinterpret_cast<HMENU>(ID_GRP_WORD_ADD), hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Слово или расширение файла (например, exe или torrent):", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(272), s(470), s(18), hwnd, nullptr, hinst, nullptr), 4);
            add_ctrl(hook_edit(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS, s(35), s(292), s(470), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_WORD), hinst, nullptr)), 4);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Добавить", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_CLIPSIBLINGS, s(520), s(290), s(115), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_WORD), hinst, nullptr), 4);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Любые слова и форматы файлов из этого списка программа никогда не будет\nавтоматически переводить на другую раскладку клавиатуры.", WS_CHILD | WS_CLIPSIBLINGS, s(35), s(330), s(600), s(36), hwnd, nullptr, hinst, nullptr), 4);

            // Bottom Buttons (always visible)
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Сохранить", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, s(445), s(472), s(105), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_SAVE), hinst, nullptr), -1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, s(558), s(472), s(105), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_CANCEL), hinst, nullptr), -1);

            PopulateDialog(hwnd);
            SwitchTab(0);
            return 0;
        }

        case WM_NOTIFY: {
            auto nm = reinterpret_cast<LPNMHDR>(lparam);
            if (nm->idFrom == ID_TAB && nm->code == TCN_SELCHANGE) {
                int cur_tab = static_cast<int>(SendMessageW(nm->hwndFrom, TCM_GETCURSEL, 0, 0));
                SwitchTab(cur_tab);
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);
            int code = HIWORD(wparam);

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
                                bool is_off = (full.find(L"[Отключено]") != std::wstring_view::npos);
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
                    // Immediately activate snippet in store and save to disk
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
                ofn.lpstrFilter = L"Программы (*.exe)\0*.exe\0Все файлы (*.*)\0*.*\0";
                ofn.lpstrFile = file_path;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
                if (GetOpenFileNameW(&ofn)) {
                    std::wstring full_path(file_path);
                    size_t last_slash = full_path.find_last_of(L"\\/");
                    std::wstring file_name = (last_slash != std::wstring::npos) ? full_path.substr(last_slash + 1) : full_path;
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

    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
    DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;

    RECT rc = { 0, 0, MulDiv(680, static_cast<int>(dpi), 96), MulDiv(530, static_cast<int>(dpi), 96) };
    AdjustWindowRectEx(&rc, style, FALSE, ex_style);
    int win_w = rc.right - rc.left;
    int win_h = rc.bottom - rc.top;

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    g_dialog_hwnd = CreateWindowExW(ex_style,
                                   kClassName, L"Настройки Ultimakey",
                                   style | WS_VISIBLE,
                                   win_x, win_y, win_w, win_h,
                                   nullptr, nullptr, hinstance, nullptr);

    SetForegroundWindow(g_dialog_hwnd);
}

} // namespace Ultimakey
