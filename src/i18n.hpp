#pragma once

#include <string_view>

namespace Ultimakey {

enum class StrId {
    // Tray
    TrayTitle,
    TrayTipRunning,
    TrayTipPaused,
    TrayTipAutoOff,
    TrayMenuAuto,
    TrayMenuPause,
    TrayMenuSettings,
    TrayMenuLog,
    TrayMenuExit,
    NotifSettingsSaved,

    // Dialog common
    SettingsTitle,
    BtnSave,
    BtnCancel,
    BtnAdd,
    BtnDelete,
    BtnBrowse,

    // Tabs
    TabGeneral,
    TabHotkeys,
    TabSnippets,
    TabAppExceptions,
    TabWordExceptions,
    TabUserWords,
    TabAbout,

    // Tab 0: General
    OptAutoEnabled,
    OptTriggerSpace,
    OptTriggerEnter,
    OptTriggerTab,
    OptArrowsCancel,
    OptTypofix,
    OptDoubleSpace,
    OptCapsRemap,
    OptSound,
    BtnListen,
    OptStartup,
    OptLog,
    LblLanguage,

    // Tab 1: Hotkeys
    LblHotkeySwitch,
    LblHotkeyMods,
    ModCtrl,
    ModAlt,
    ModShift,
    ModWin,
    HintHotkeySelection,

    // Tab 2: Snippets
    GrpSnippetsList,
    LblSnippetsList,
    HintDeleteSnippet,
    GrpSnippetsAdd,
    LblSnippetTrigger,
    LblSnippetReplacement,
    HintSnippetsHow,

    // Tab 3: App Exceptions
    GrpAppList,
    LblAppList,
    HintDeleteApp,
    GrpAppAdd,
    LblAppPath,
    AppModeSoft,
    AppModeOff,
    HintAppModes,

    // Tab 4: Word Exceptions
    GrpWordList,
    LblWordList,
    HintDeleteWord,
    GrpWordAdd,
    LblWordAdd,
    HintWordFootnote,

    // Tab 5: User Dictionary (Positive List)
    GrpUserWordList,
    LblUserWordList,
    HintDeleteUserWord,
    GrpUserWordAdd,
    LblUserWordAdd,
    HintUserWordFootnote,

    // Search
    SearchPlaceholder,

    // Tab 5: About
    AboutTitle,
    AboutDesc,
    GrpAboutLinks,
    LblAboutRepo,
    BtnAboutRepo,
    LblAboutDonate,
    BtnAboutDonate,

    // Error messages
    ErrLoadDicts,
    ErrInstallHook,
    ErrCreateWindow,

    Count
};

const wchar_t* Tr(StrId id) noexcept;
const wchar_t* Tr(StrId id, std::string_view lang) noexcept;

} // namespace Ultimakey
