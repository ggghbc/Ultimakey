#pragma once

#include "types.hpp"
#include "dawg.hpp"
#include "trigrams.hpp"
#include "typo_rules.hpp"
#include "detector.hpp"
#include "buffer.hpp"
#include "antiresonance.hpp"
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <atomic>

namespace Ultimakey {

class Engine {
public:
    static Engine& Instance();

    bool Initialize(HINSTANCE hinstance);
    void Shutdown();

    // Hook callbacks
    bool OnKeyboardHook(int n_code, WPARAM w_param, const KBDLLHOOKSTRUCT& kb);
    void OnMouseHook(int n_code, WPARAM w_param);

    void ConvertFromBuffer(bool manual, bool soft);
    void SetPaused(bool paused) noexcept { paused_ = paused; }
    bool IsPaused() const noexcept { return paused_; }

    void OnForegroundChanged(HWND hwnd);
    void OnBoundaryTimer();
    DWORD CurrentForegroundThreadId() const noexcept { return front_tid_; }
    HWND LastForegroundHwnd() const noexcept { return last_fg_hwnd_; }
    void SetMessageHwnd(HWND hwnd) noexcept;
    void OnSyntheticFlightFinished(bool ok);

private:
    Engine();
    ~Engine();

    bool OnKeyDown(int vk, int scan, bool injected);
    bool OnKeyUp(int vk);

    bool OnBoundary(int vk, bool command, bool shift);
    bool ConvertBeforeReturn(bool shift);
    bool CheckRecentSnippet(std::wstring_view ws = {});
    bool CheckSnippet(std::wstring_view word);
    bool CheckTypo(std::wstring_view word, std::wstring_view ws);

    struct Proposal {
        std::wstring text;
        bool to_cyrillic;
        bool rescue;
    };
    std::optional<Proposal> AutoProposal(std::wstring_view word, bool soft, bool completed);

    bool ConvertSelection();
    void HandleContextReset();
    void ApplyContextClear();
    void RefreshForeground();

    wchar_t DecodeChar(int vk, int scan, bool shift) noexcept;

    bool MutedStuck();
    void SetMuted();
    void EndSyntheticFlight(bool ok);

    Dawg words_ru_;
    Dawg words_en_;
    TrigramTable trigrams_ru_;
    TrigramTable trigrams_en_;
    TypoRules typo_rules_;

    std::unique_ptr<LayoutDetector> detector_;
    KeystrokeBuffer buf_;
    AntiResonance anti_;

    bool initialized_ = false;
    bool paused_ = false;

    bool muted_ = false;
    double muted_at_ = 0.0;
    int in_flight_real_keys_ = 0;
    bool pending_manual_ = false;
    bool pending_context_clear_ = false;
    bool caret_jumped_since_clear_ = false;
    bool backspace_since_jump_ = false;

    HWND last_fg_hwnd_ = nullptr;
    DWORD front_pid_ = 0;
    DWORD front_tid_ = 0;
    std::wstring front_process_;

    HWND msg_hwnd_ = nullptr;
    HWINEVENTHOOK fg_event_hook_ = nullptr;
    bool boundary_mode_soft_ = false;
    HWND boundary_fg_ = nullptr;
    std::atomic<uint32_t> boundary_gen_{0};
    uint32_t boundary_gen_at_start_ = 0;

    AppMode current_app_mode_ = AppMode::Default;

    std::unordered_map<int, int64_t> swallowed_ups_;
    TransparentStringSet session_protected_;
    int current_modifiers_ = 0;
};

} // namespace Ultimakey
