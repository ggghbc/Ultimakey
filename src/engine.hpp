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

private:
    Engine();
    ~Engine();

    bool OnKeyDown(int vk, int scan, bool injected);
    bool OnKeyUp(int vk);

    bool OnBoundary(int vk, bool command, bool shift);
    bool ConvertBeforeReturn(std::wstring_view mode, bool shift);
    bool CheckSnippet(std::wstring_view word);
    bool CheckTypo(std::wstring_view word);

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

    std::wstring DecodeChar(int vk, int scan, bool shift);

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
    std::wstring front_process_;

    std::unordered_map<int, int64_t> swallowed_ups_;
    std::unordered_set<std::wstring> session_protected_;
};

} // namespace Ultimakey
