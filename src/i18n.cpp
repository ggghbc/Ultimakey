#include "i18n.hpp"
#include "settings.hpp"
#include <array>

namespace Ultimakey {

// [0] = English, [1] = Russian
static const wchar_t* const kStrings[static_cast<size_t>(StrId::Count)][2] = {
    // Tray
    { L"Ultimakey - Keyboard Switcher", L"Ultimakey - Переключатель раскладки" },
    { L"Ultimakey — Auto-switching active", L"Ultimakey — переключение раскладки (Работает)" },
    { L"Ultimakey — Paused", L"Ultimakey — переключение раскладки (Пауза)" },
    { L"Ultimakey — Auto-mode disabled", L"Ultimakey — переключение раскладки (Авторежим выключен)" },
    { L"Automatic Layout Switching", L"Автоматическое переключение раскладки" },
    { L"Pause Switching", L"Приостановить работу (Пауза)" },
    { L"Settings…", L"Настройки программы…" },
    { L"Show Log File", L"Показать файл журнала (лога)" },
    { L"Exit", L"Выход из программы" },
    { L"Settings saved", L"Настройки сохранены" },

    // Dialog common
    { L"Ultimakey Settings", L"Настройки Ultimakey" },
    { L"Save", L"Сохранить" },
    { L"Cancel", L"Отмена" },
    { L"Add", L"Добавить" },
    { L"Delete", L"Удалить" },
    { L"Browse .exe…", L"Выбрать .exe файл…" },

    // Tabs
    { L"General", L"Основные" },
    { L"Hotkeys", L"Горячие клавиши" },
    { L"Auto-replace", L"Автозамена текста" },
    { L"App Exceptions", L"Исключения программ" },
    { L"Word Exceptions", L"Исключения слов" },
    { L"About", L"О программе" },

    // Tab 0: General
    { L"Automatically fix incorrect layout", L"Автоматически исправлять неверную раскладку" },
    { L"  • When pressing Space", L"  • При нажатии Пробела" },
    { L"  • When pressing Enter", L"  • При нажатии Enter" },
    { L"  • When pressing Tab", L"  • При нажатии Tab" },
    { L"Cancel fix when moving with arrow keys", L"Отменять исправление при перемещении стрелками" },
    { L"Automatically fix common typos", L"Автоматически исправлять частые опечатки" },
    { L"Replace double space with period and space (. )", L"Двойной пробел заменять на точку и пробел (. )" },
    { L"CapsLock toggles layout (without locking)", L"Клавиша CapsLock переключает раскладку (без фиксации)" },
    { L"Sound click on word correction", L"Звуковой щелчок при исправлении слова" },
    { L"▶", L"▶" },
    { L"Launch Ultimakey on Windows startup", L"Запускать Ultimakey при входе в Windows" },
    { L"Write diagnostic debug log to disk", L"Вести журнал работы программы для отладки" },
    { L"Interface language:", L"Язык интерфейса:" },

    // Tab 1: Hotkeys
    { L"Key to switch language of typed word or selected text:", L"Клавиша для смены языка слова или выделенного текста:" },
    { L"Additional modifier keys:", L"Дополнительные клавиши-модификаторы:" },
    { L"Ctrl", L"Ctrl" },
    { L"Alt", L"Alt" },
    { L"Shift", L"Shift" },
    { L"Win", L"Win" },
    { L"Tip: select any text in any app and press the hotkey to instantly convert the selection.", L"Совет: если выделить фрагмент текста и нажать горячую клавишу,\nUltimakey изменит раскладку всего выделенного фрагмента." },

    // Tab 2: Snippets
    { L" Saved Auto-replace Rules ", L" Сохранённые правила автозамены " },
    { L"Active rules list (select a rule to view or delete):", L"Список активных правил (выберите правило для просмотра или удаления):" },
    { L"Select a rule\non the left\nto delete it", L"Выберите правило\nв списке слева,\nчтобы удалить его" },
    { L" Add or Edit Auto-replace Rule ", L" Добавить или изменить правило " },
    { L"Abbreviation to type:", L"Что вводите (сокращение):" },
    { L"Replacement (full text):", L"На что заменять (полный текст):" },
    { L"How it works: type an abbreviation (e.g. 'thx') and hit Space or Tab;\nUltimakey instantly expands it to full text ('Thank you very much!').", L"Как это работает: при вводе сокращения (например, 'спс') и нажатии пробела\nпрограмма мгновенно заменит его на полный текст ('Спасибо большое!')." },

    // Tab 3: App Exceptions
    { L" Applications with Special Rules ", L" Программы с особым режимом работы " },
    { L"Excluded apps list (select to inspect or delete):", L"Список программ-исключений (выберите для просмотра или удаления):" },
    { L"Select an app\non the left\nto delete it", L"Выберите программу\nв списке слева,\nчтобы удалить её" },
    { L" Add Application to Exclusions ", L" Добавить программу в исключения " },
    { L"Click «Browse .exe…» or enter executable name:", L"Нажмите «Выбрать .exe файл…» или введите имя:" },
    { L"Soft mode", L"Мягкий режим" },
    { L"Disabled", L"Отключено" },
    { L"Click «Browse .exe…» to pick an app via Windows Explorer.\n• Soft mode: only long words (4+ chars) are auto-corrected.\n• Disabled: auto-correction is completely disabled in this app.", L"Нажмите «Выбрать .exe файл…» для выбора программы через проводник Windows.\n• Мягкий режим: исправляются только длинные слова (от 4 букв).\n• Отключено: автоисправление полностью выключено в этой программе." },

    // Tab 4: Word Exceptions
    { L" Words that are not corrected ", L" Слова, которые не исправляются " },
    { L"Ignored words & extensions list (exe, dll, txt, png, github, etc.):", L"Список слов-исключений (exe, dll, txt, png, github и др.):" },
    { L"Select a word\non the left\nto delete it", L"Выберите слово\nв списке слева,\nчтобы удалить его" },
    { L" Add Word or Extension to Ignored List ", L" Добавить слово или расширение в список " },
    { L"Word or file extension (e.g., exe or torrent):", L"Слово или расширение файла (например, exe или torrent):" },
    { L"Words and file extensions in this list will never be\nautomatically converted by Ultimakey to another layout.", L"Любые слова и форматы файлов из этого списка программа никогда не будет\nавтоматически переводить на другую раскладку клавиатуры." },

    // Tab 5: About
    { L"Ultimakey 1.0.0", L"Ultimakey 1.0.0" },
    { L"Lightweight, ultra-fast and private automatic keyboard layout switcher for Windows.\nOpen-source and runs 100% offline with zero telemetry.", L"Легкий, быстрый и конфиденциальный автоматический переключатель раскладки клавиатуры.\nОткрытый исходный код, работает полностью офлайн без телеметрии." },
    { L" Project & Author ", L" Проект и автор " },
    { L"Official repository, source code & releases:", L"Официальный репозиторий, исходный код и обновления:" },
    { L"GitHub Repository (github.com/ggghbc/Ultimakey)", L"Репозиторий GitHub (github.com/ggghbc/Ultimakey)" },
    { L"Support the developer:", L"Поддержать автора и разработку:" },
    { L"Support on Boosty (boosty.to/ggghbc)", L"Поддержать на Boosty (boosty.to/ggghbc)" },

    // Error messages
    { L"Failed to load built-in language dictionaries!", L"Не удалось загрузить встроенные языковые словари!" },
    { L"Failed to install keyboard hook!", L"Не удалось установить хук клавиатуры!" },
    { L"Failed to create message window!", L"Ошибка создания окна сообщений" }
};

const wchar_t* Tr(StrId id) noexcept {
    const std::string& lang = Settings::Instance().language;
    return Tr(id, lang);
}

const wchar_t* Tr(StrId id, std::string_view lang) noexcept {
    size_t idx = static_cast<size_t>(id);
    if (idx >= static_cast<size_t>(StrId::Count)) return L"";
    size_t lang_idx = (lang == "ru") ? 1 : 0;
    return kStrings[idx][lang_idx];
}

} // namespace Ultimakey
