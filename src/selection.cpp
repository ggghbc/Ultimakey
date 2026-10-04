#include "selection.hpp"
#include "secure_input.hpp"

namespace Ultimakey {

std::wstring SelectionText::TryGet(int timeout_ms) {
    return SecureInput::Instance().GetSelectionText(timeout_ms);
}

} // namespace Ultimakey
