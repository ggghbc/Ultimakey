#include "ui_settings.hpp"
#include "settings.hpp"
#include "autostart.hpp"
#include "snippets.hpp"
#include "sound.hpp"
#include "tray.hpp"
#include "engine.hpp"
#include <commctrl.h>
#include <uxtheme.h>
#include <vector>

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
    ID_BTN_ADD_APP = 506
};

static const struct { int vk; const wchar_t* name; } kHotkeys[] = {
    {VK_PAUSE, L"Pause / Break"},
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
    SendMessageW(hsnip, LB_RESETCONTENT, 0, 0);
    for (const auto& [trig, exp] : s.snippets) {
        std::wstring line = trig + L"  ->  " + exp;
        SendMessageW(hsnip, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }

    // Tab 3 apps
    HWND happs = GetDlgItem(hwnd, ID_LIST_APPS);
    SendMessageW(happs, LB_RESETCONTENT, 0, 0);
    for (const auto& [app, mode] : s.app_modes) {
        std::wstring mode_desc = (mode == L"off") ? L"[Отключено]" : L"[Мягкий режим]";
        std::wstring line = app + L"  " + mode_desc;
        SendMessageW(happs, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }

    HWND happ_mode_combo = GetDlgItem(hwnd, ID_COMBO_APP_MODE);
    SendMessageW(happ_mode_combo, CB_RESETCONTENT, 0, 0);
    SendMessageW(happ_mode_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Мягкий режим"));
    SendMessageW(happ_mode_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Отключено"));
    SendMessageW(happ_mode_combo, CB_SETCURSEL, 0, 0);
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
                                        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                        s(10), s(10), s(525), s(440), hwnd, reinterpret_cast<HMENU>(ID_TAB), hinst, nullptr);
            SendMessageW(htab, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), TRUE);

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

            auto add_ctrl = [&](HWND h, int tab_idx) {
                SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), TRUE);
                g_controls.push_back({h, tab_idx});
                return h;
            };

            // TAB 0 Controls
            int y = s(42);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Автоматически исправлять неверную раскладку", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_AUTO), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"  • При нажатии Пробела", WS_CHILD | BS_AUTOCHECKBOX, s(45), y, s(400), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SPACE), hinst, nullptr), 0); y += s(20);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"  • При нажатии Enter", WS_CHILD | BS_AUTOCHECKBOX, s(45), y, s(400), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ENTER), hinst, nullptr), 0); y += s(20);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"  • При нажатии Tab", WS_CHILD | BS_AUTOCHECKBOX, s(45), y, s(400), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_TAB), hinst, nullptr), 0); y += s(23);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Отменять исправление при перемещении стрелками", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ARROWS), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Автоматически исправлять частые опечатки", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_TYPO), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Двойной пробел заменять на точку и пробел (. )", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_DOUBLESPACE), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Клавиша CapsLock переключает раскладку (без фиксации)", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_CAPSREMAP), hinst, nullptr), 0); y += s(22);

            // Sound: checkbox + compact play icon button
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Звуковой щелчок при исправлении слова", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(315), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SOUND), hinst, nullptr), 0);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"▶", WS_CHILD | BS_PUSHBUTTON, s(345), y - s(1), s(30), s(22), hwnd, reinterpret_cast<HMENU>(ID_BTN_TEST_SOUND), hinst, nullptr), 0);
            y += s(24);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Запускать Ultimakey при входе в Windows", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_AUTOSTART), hinst, nullptr), 0); y += s(22);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Вести журнал работы программы для отладки", WS_CHILD | BS_AUTOCHECKBOX, s(25), y, s(420), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_LOG), hinst, nullptr), 0);

            // TAB 1 Controls (Hotkeys)
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Клавиша для смены языка слова или выделенного текста:", WS_CHILD, s(25), s(50), s(480), s(20), hwnd, reinterpret_cast<HMENU>(ID_LBL_HOTKEY), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, s(25), s(75), s(220), s(200), hwnd, reinterpret_cast<HMENU>(ID_COMBO_HOTKEY), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Дополнительные клавиши-модификаторы:", WS_CHILD, s(25), s(115), s(480), s(20), hwnd, nullptr, hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Ctrl", WS_CHILD | BS_AUTOCHECKBOX, s(25), s(140), s(75), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_CTRL), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Alt", WS_CHILD | BS_AUTOCHECKBOX, s(110), s(140), s(75), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_ALT), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Shift", WS_CHILD | BS_AUTOCHECKBOX, s(195), s(140), s(75), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_SHIFT), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Win", WS_CHILD | BS_AUTOCHECKBOX, s(280), s(140), s(75), s(20), hwnd, reinterpret_cast<HMENU>(ID_CHK_WIN), hinst, nullptr), 1);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Совет: если выделить фрагмент текста и нажать горячую клавишу,\nUltimakey изменит раскладку всего выделенного фрагмента.", WS_CHILD, s(25), s(185), s(480), s(40), hwnd, nullptr, hinst, nullptr), 1);

            // TAB 2 Controls (Snippets / Text Replacement) - Crystal Clear GroupBoxes
            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Сохранённые правила автозамены ", WS_CHILD | BS_GROUPBOX, s(20), s(42), s(505), s(200), hwnd, reinterpret_cast<HMENU>(ID_GRP_SNIP_LIST), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Список активных правил (выберите правило для просмотра или удаления):", WS_CHILD, s(30), s(62), s(365), s(18), hwnd, nullptr, hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL, s(30), s(82), s(365), s(148), hwnd, reinterpret_cast<HMENU>(ID_LIST_SNIPPETS), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Удалить", WS_CHILD, s(405), s(82), s(105), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_SNIP), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Выберите правило\nв списке слева,\nчтобы удалить его", WS_CHILD, s(405), s(120), s(110), s(45), hwnd, nullptr, hinst, nullptr), 2);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Добавить или изменить правило ", WS_CHILD | BS_GROUPBOX, s(20), s(252), s(505), s(165), hwnd, reinterpret_cast<HMENU>(ID_GRP_SNIP_ADD), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Что вводите (сокращение):", WS_CHILD, s(30), s(272), s(175), s(18), hwnd, nullptr, hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL, s(30), s(292), s(175), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_TRIG), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"На что заменять (полный текст):", WS_CHILD, s(215), s(272), s(180), s(18), hwnd, nullptr, hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL, s(215), s(292), s(180), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_EXP), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Добавить", WS_CHILD, s(405), s(290), s(105), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_SNIP), hinst, nullptr), 2);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Как это работает: при вводе сокращения (например, 'спс') и нажатии пробела\nпрограмма мгновенно заменит его на полный текст ('Спасибо большое!').", WS_CHILD, s(30), s(330), s(485), s(36), hwnd, nullptr, hinst, nullptr), 2);

            // TAB 3 Controls (Apps Exclusions) - Crystal Clear GroupBoxes
            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Программы с особым режимом работы ", WS_CHILD | BS_GROUPBOX, s(20), s(42), s(505), s(200), hwnd, reinterpret_cast<HMENU>(ID_GRP_APP_LIST), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Список программ-исключений (выберите для просмотра или удаления):", WS_CHILD, s(30), s(62), s(365), s(18), hwnd, nullptr, hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | LBS_NOTIFY | WS_VSCROLL, s(30), s(82), s(365), s(148), hwnd, reinterpret_cast<HMENU>(ID_LIST_APPS), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Удалить", WS_CHILD, s(405), s(82), s(105), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_DEL_APP), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Выберите программу\nв списке слева,\nчтобы удалить её", WS_CHILD, s(405), s(120), s(110), s(45), hwnd, nullptr, hinst, nullptr), 3);

            add_ctrl(CreateWindowExW(0, L"BUTTON", L" Добавить программу в исключения ", WS_CHILD | BS_GROUPBOX, s(20), s(252), s(505), s(165), hwnd, reinterpret_cast<HMENU>(ID_GRP_APP_ADD), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Имя программы (напр., cs2.exe):", WS_CHILD, s(30), s(272), s(180), s(18), hwnd, nullptr, hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL, s(30), s(292), s(180), s(24), hwnd, reinterpret_cast<HMENU>(ID_EDIT_APP), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"Режим работы:", WS_CHILD, s(220), s(272), s(175), s(18), hwnd, nullptr, hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, s(220), s(292), s(175), s(120), hwnd, reinterpret_cast<HMENU>(ID_COMBO_APP_MODE), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Добавить", WS_CHILD, s(405), s(290), s(105), s(28), hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD_APP), hinst, nullptr), 3);
            add_ctrl(CreateWindowExW(0, L"STATIC", L"• Мягкий режим: исправляются только длинные слова (от 4 букв).\n• Отключено: автоисправление полностью выключено в этой программе.", WS_CHILD, s(30), s(330), s(485), s(36), hwnd, nullptr, hinst, nullptr), 3);

            // Bottom Buttons (always visible)
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Сохранить", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, s(325), s(462), s(100), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_SAVE), hinst, nullptr), -1);
            add_ctrl(CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE, s(435), s(462), s(100), s(30), hwnd, reinterpret_cast<HMENU>(ID_BTN_CANCEL), hinst, nullptr), -1);

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
                        int len = static_cast<int>(SendMessageW(happs, LB_GETTEXTLEN, sel, 0));
                        std::vector<wchar_t> text(len + 1);
                        SendMessageW(happs, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(text.data()));
                        std::wstring full(text.data());
                        size_t sp = full.rfind(L"  [");
                        if (sp != std::wstring::npos) {
                            std::wstring app_key = full.substr(0, sp);
                            SetDlgItemTextW(hwnd, ID_EDIT_APP, app_key.c_str());
                            bool is_off = (full.find(L"[Отключено]") != std::wstring::npos);
                            SendMessageW(GetDlgItem(hwnd, ID_COMBO_APP_MODE), CB_SETCURSEL, is_off ? 1 : 0, 0);
                        }
                    }
                    return 0;
                }
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
                wchar_t trig[128] = {}, exp[512] = {};
                GetDlgItemTextW(hwnd, ID_EDIT_TRIG, trig, 128);
                GetDlgItemTextW(hwnd, ID_EDIT_EXP, exp, 512);
                if (wcslen(trig) > 0 && wcslen(exp) > 0) {
                    bool found = false;
                    for (auto& [t, e] : Settings::Instance().snippets) {
                        if (t == trig) {
                            e = exp;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        Settings::Instance().snippets.emplace_back(trig, exp);
                    }
                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_TRIG, L"");
                    SetDlgItemTextW(hwnd, ID_EDIT_EXP, L"");
                }
            } else if (id == ID_BTN_DEL_SNIP) {
                HWND hsnip = GetDlgItem(hwnd, ID_LIST_SNIPPETS);
                int sel = static_cast<int>(SendMessageW(hsnip, LB_GETCURSEL, 0, 0));
                if (sel >= 0 && sel < static_cast<int>(Settings::Instance().snippets.size())) {
                    Settings::Instance().snippets.erase(Settings::Instance().snippets.begin() + sel);
                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_TRIG, L"");
                    SetDlgItemTextW(hwnd, ID_EDIT_EXP, L"");
                }
            } else if (id == ID_BTN_ADD_APP) {
                wchar_t app_name[128] = {};
                GetDlgItemTextW(hwnd, ID_EDIT_APP, app_name, 128);
                HWND happ_mode_combo = GetDlgItem(hwnd, ID_COMBO_APP_MODE);
                int cur_mode_sel = static_cast<int>(SendMessageW(happ_mode_combo, CB_GETCURSEL, 0, 0));
                const wchar_t* mode_code = (cur_mode_sel == 1) ? L"off" : L"soft";
                if (wcslen(app_name) > 0) {
                    Settings::Instance().app_modes[ToLower(app_name)] = mode_code;
                    PopulateDialog(hwnd);
                    SetDlgItemTextW(hwnd, ID_EDIT_APP, L"");
                }
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
                        PopulateDialog(hwnd);
                        SetDlgItemTextW(hwnd, ID_EDIT_APP, L"");
                    }
                }
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
            g_dialog_hwnd = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void SettingsDialog::Show(HWND parent_hwnd, HINSTANCE hinstance) {
    if (g_dialog_hwnd) {
        SetForegroundWindow(g_dialog_hwnd);
        return;
    }

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

    int win_w = MulDiv(560, static_cast<int>(dpi), 96);
    int win_h = MulDiv(545, static_cast<int>(dpi), 96);
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    g_dialog_hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
                                   kClassName, L"Настройки Ultimakey",
                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                   win_x, win_y, win_w, win_h,
                                   nullptr, nullptr, hinstance, nullptr);

    SetForegroundWindow(g_dialog_hwnd);
}

} // namespace Ultimakey
