#include "sound.hpp"
#include <mmsystem.h>
#include <cmath>
#include <cstring>

namespace Ultimakey {

SoundEffect& SoundEffect::Instance() {
    static SoundEffect instance;
    return instance;
}

SoundEffect::SoundEffect() {
    GenerateWav();
}

void SoundEffect::GenerateWav() {
    constexpr int sample_rate = 44100;
    constexpr double duration = 0.034; // 34 ms
    constexpr int num_samples = static_cast<int>(sample_rate * duration);
    constexpr double freq = 440.0;
    constexpr double pi = 3.14159265358979323846;

    std::vector<int16_t> samples(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        // Soft attack (7 ms), fast exponential decay
        double attack = (t < 0.007) ? (t / 0.007) : 1.0;
        double decay = std::exp(-t / 0.012);
        double envelope = attack * decay * 0.35; // moderate amplitude
        double wave = std::sin(2.0 * pi * freq * t);
        samples[i] = static_cast<int16_t>(wave * envelope * 32767.0);
    }

    // RIFF WAV Header
    uint32_t data_size = num_samples * sizeof(int16_t);
    uint32_t file_size = 36 + data_size;

    wav_data_.resize(44 + data_size);
    uint8_t* p = wav_data_.data();

    std::memcpy(p, "RIFF", 4);
    *reinterpret_cast<uint32_t*>(p + 4) = file_size;
    std::memcpy(p + 8, "WAVEfmt ", 8);
    *reinterpret_cast<uint32_t*>(p + 16) = 16;              // Subchunk1Size (16 for PCM)
    *reinterpret_cast<uint16_t*>(p + 20) = 1;               // AudioFormat (1 for PCM)
    *reinterpret_cast<uint16_t*>(p + 22) = 1;               // NumChannels (1 = Mono)
    *reinterpret_cast<uint32_t*>(p + 24) = sample_rate;     // SampleRate
    *reinterpret_cast<uint32_t*>(p + 28) = sample_rate * 2; // ByteRate
    *reinterpret_cast<uint16_t*>(p + 32) = 2;               // BlockAlign
    *reinterpret_cast<uint16_t*>(p + 34) = 16;              // BitsPerSample
    std::memcpy(p + 36, "data", 4);
    *reinterpret_cast<uint32_t*>(p + 40) = data_size;
    std::memcpy(p + 44, samples.data(), data_size);
}

void SoundEffect::PlaySwitchSound() {
    if (!enabled_ || wav_data_.empty()) return;
    PlaySoundW(reinterpret_cast<LPCWSTR>(wav_data_.data()), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

} // namespace Ultimakey
