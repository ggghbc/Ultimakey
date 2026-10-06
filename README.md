# Ultimakey

Ultimakey is an ultra-lightweight, high-performance utility that switches keyboard layouts between RU and EN while typing. Designed for Windows 10/11 (with backward compatibility for Windows 7/8.1).

Ultimakey delivers instantaneous layout switching with virtually zero CPU usage and less than 3 MB of RAM consumption.

![demo](./readme/demo.gif)

## Key Performance Metrics

| Metric | Ultimakey (C++20) | Alternatives |
| :--- | :--- | :--- |
| RAM Footprint | ~2.3 MB (trims to < 1 MB on idle) | 50 to 150+ MB |
| Idle CPU Usage | 0.00% | 0.5% to 2.0% |
| Input Hook Overhead | 1-5 microseconds | 1 to 15 milliseconds |
| Binary Size | ~4.4 MB (fully self-contained executable) | 30 to 80+ MB |
| External Dependencies | None (embedded PE RCDATA resources) | Multiple external files / runtimes |
| Cold Start Time | < 5 milliseconds | 500 to 2000 milliseconds |

---

## WARNING
Unfortunately, one of the antivirus programs on **[VirusTotal](https://www.virustotal.com/gui/file/2aaa3b45cc370e3bec9a8e5eb1727a0f252115a4e729e2782fd92279d8e0e8fc?nocache=1)** reports this app as a trojan (*Trojan:Win32/Wacatac.B!ml*). You'll have to trust that the app is clean and won't harm your computer.

Therefore, the first time you download a file from a browser, Windows may display a warning: "Windows protected your PC... Unknown publisher."

You just need to click "More details" → "Run anyway."


## Features

### Intelligent Automatic Layout Switching
- Trigram frequency evaluation (over 11,400 Russian and 8,100 English trigrams) combined with instantaneous dictionary lookups (over 163,000 Russian and 59,500 English words).
- Triggered seamlessly upon word boundaries: Space, Enter (with pre-conversion before newline delivery), or Tab.
- Smart punctuation and prefix handling: automatically handles words typed with trailing punctuation or layout-dependent letters (such as Russian letters located on English punctuation keys).

### Manual Conversion Hotkeys
- Instant conversion of the most recently typed word via hotkey (default: `Pause/Break`, fully customizable).
- Selection conversion: converts selected text across layouts using UI Automation TextPattern, falling back to a non-destructive clipboard mechanism that preserves original clipboard contents.

### TypoFix (Automatic Typo Correction)
- Built-in database of over 5,000 common typographical errors and misspellings with instantaneous binary search lookup.

### Snippets (Text Expansion)
- Fast expansion of user-defined shortcuts regardless of the active keyboard layout (for example, typing `!email` automatically expands to `user@example.com`).

### Double Space to Period
- Automatically replaces a double space sequence with a period followed by a space (`. `), mirroring modern mobile operating system ergonomics.

### CapsLock Remap
- Instant keyboard layout toggling on a single tap of the `CapsLock` key without toggling uppercase lock state. Long press or standard combination retains original CapsLock functionality if desired.

### Procedural Audio Feedback (CueSynth)
- A low-latency 34-millisecond procedural click generated entirely in memory via Windows Multimedia API (`waveOut`), signaling successful word conversion without loading external sound files from disk.

### Per-Application Profiles (App Modes)
- Customizable per-process behaviors:
  - `Auto`: Full automatic conversion enabled.
  - `Soft`: Context-sensitive mode ideal for IDEs and text editors.
  - `Off`: Complete suppression for gaming, command-line shells, remote desktop, and graphic design software.

### Secure Input Protection
- Automatic suppression in password fields and secure credential prompts through asynchronous Windows UI Automation inspection.

### Native Win32 Settings Interface
- Lightweight native settings dialog (`SysTabControl32`) with tabbed navigation: General, Hotkeys, Applications, Snippets, and Dictionary.
- Opens instantly without spawning browser engines or background runtimes.

---

## Building from Source

### Prerequisites
- **C++ Compiler:** GCC (MinGW-w64) 14+ or Clang / MSVC with full **C++20** support.
- **Build System:** CMake 3.20+ and Ninja (or Make).
- **Python 3.10+** (only required if rebuilding binary dictionary files from raw wordlists).

### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/rehan-remade/Ultimakey.git
   cd Ultimakey
   ```

2. **(Optional) Recompile binary dictionaries:**
   ```bash
   python tools/bake_data.py
   ```
   *Note: Precompiled binary datasets are already included in `src/data/`.*

   *[Datasets based on this dictionaries.](https://github.com/ggghbc/DictionaryAndTrigramGenerator)*

3. **Configure and build using CMake (MinGW-w64 example):**
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

4. **Output:**
   The compiled standalone binary `Ultimakey.exe` will be located in the `build/` directory.

---

## Usage and Tray Controls

When launched, Ultimakey runs quietly in the Windows notification area (System Tray).

### Tray Icon Indicators
- **Green Icon:** Automatic conversion mode is active.
- **Gray Icon:** Automatic conversion mode is suspended.

### Context Menu (Right-Click on Tray Icon)
- **Enable Auto-Switching:** Toggle automatic conversion on or off.
- **Settings...:** Open the native configuration dialog.
- **View Log:** Open the diagnostic log window.
- **About Ultimakey:** Display version and build information.
- **Exit:** Terminate the application.

### Default Hotkeys
- `Pause/Break`: Convert the last typed word or selection between Russian and English layouts.
- `CapsLock`: Instant layout switch (when CapsLock Remap is enabled).

### Autorun
When the user checks the "Autorun" box in the settings, the program writes the path to the current location of Ultimakey.exe to the registry.

If the user launches it from the `Downloads` folder and enables autorun, but then moves the file to another location, autorun will fail.

It is recommended to prompt users to place Ultimakey.exe in a permanent folder (e.g., `C:\Tools\Ultimakey\` or the user's home folder).

---

## Configuration File

The configuration file is located at:
```text
%APPDATA%\Ultimakey\settings.json
```

---

## Special Thanks
### To [keyboop](https://github.com/iffuno/keyboop) project for idea and inspiration.

## License

[MIT License](./LICENSE). Free to use, modify, and distribute.
