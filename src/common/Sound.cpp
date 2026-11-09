#include "pch.h"
#include "Sound.h"
#include <DxLib.h>
#include "MathUtil.h"

namespace {
    constexpr int SampleRate = 44100;
    int handles[Sound::Count];
    bool initialized = false;

    // 合成の材料
    struct Synth {
        std::vector<float> data;
        std::mt19937 rng{ 12345u };

        explicit Synth(float seconds) : data(static_cast<size_t>(seconds * SampleRate), 0.0f) {}

        // 雑音。cutoff（0〜1）が小さいほどこもった音になる。decay は減衰の速さ
        void Noise(float start, float length, float volume, float cutoff, float decay) {
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            const int s0 = static_cast<int>(start * SampleRate);
            const int n = static_cast<int>(length * SampleRate);
            float lp = 0.0f;
            for (int i = 0; i < n && s0 + i < static_cast<int>(data.size()); ++i) {
                const float t = static_cast<float>(i) / SampleRate;
                lp += (dist(rng) - lp) * cutoff;
                const float env = std::exp(-t * decay) * std::min(1.0f, i / 40.0f);
                data[s0 + i] += lp * volume * env;
            }
        }

        // 音程のある音。周波数を f0 から f1 へ変える。square が true なら矩形波
        void Tone(float start, float length, float volume, float f0, float f1, float decay, bool square) {
            const int s0 = static_cast<int>(start * SampleRate);
            const int n = static_cast<int>(length * SampleRate);
            float phase = 0.0f;
            for (int i = 0; i < n && s0 + i < static_cast<int>(data.size()); ++i) {
                const float u = static_cast<float>(i) / n;
                const float t = static_cast<float>(i) / SampleRate;
                const float f = f0 + (f1 - f0) * u;
                phase += f / SampleRate;
                float v = std::sin(phase * 2.0f * kPi);
                if (square) {
                    v = v >= 0.0f ? 0.6f : -0.6f;
                }
                const float env = std::exp(-t * decay) * std::min(1.0f, i / 60.0f) * std::min(1.0f, (n - i) / 200.0f);
                data[s0 + i] += v * volume * env;
            }
        }

        int Build() const {
            const int soft = MakeSoftSound1Ch16Bit44KHz(static_cast<int>(data.size()));
            if (soft == -1) {
                return -1;
            }
            for (int i = 0; i < static_cast<int>(data.size()); ++i) {
                const int v = static_cast<int>(Clampf(data[i], -1.0f, 1.0f) * 30000.0f);
                WriteSoftSoundData(soft, i, v, v);
            }
            const int handle = LoadSoundMemFromSoftSound(soft);
            DeleteSoftSound(soft);
            return handle;
        }
    };
}

void Sound::Init() {
    for (int& h : handles) {
        h = -1;
    }
    {
        Synth s(0.12f);
        s.Noise(0.0f, 0.12f, 0.7f, 0.5f, 40.0f);
        s.Tone(0.0f, 0.06f, 0.4f, 320.0f, 180.0f, 40.0f, false);
        handles[HitLight] = s.Build();
    }
    {
        Synth s(0.18f);
        s.Noise(0.0f, 0.18f, 0.8f, 0.35f, 25.0f);
        s.Tone(0.0f, 0.1f, 0.5f, 220.0f, 110.0f, 25.0f, false);
        handles[HitMedium] = s.Build();
    }
    {
        Synth s(0.32f);
        s.Noise(0.0f, 0.32f, 0.9f, 0.22f, 12.0f);
        s.Tone(0.0f, 0.25f, 0.7f, 150.0f, 50.0f, 10.0f, false);
        handles[HitHeavy] = s.Build();
    }
    {
        Synth s(0.1f);
        s.Noise(0.0f, 0.05f, 0.5f, 0.9f, 60.0f);
        s.Tone(0.0f, 0.08f, 0.35f, 1400.0f, 1100.0f, 30.0f, true);
        handles[Block] = s.Build();
    }
    {
        Synth s(0.14f);
        s.Noise(0.0f, 0.14f, 0.35f, 0.15f, 18.0f);
        handles[Whiff] = s.Build();
    }
    {
        Synth s(0.4f);
        s.Noise(0.0f, 0.4f, 0.5f, 0.3f, 6.0f);
        s.Tone(0.0f, 0.35f, 0.35f, 300.0f, 700.0f, 6.0f, false);
        handles[Projectile] = s.Build();
    }
    {
        Synth s(0.45f);
        s.Noise(0.0f, 0.45f, 0.9f, 0.08f, 5.0f);
        s.Tone(0.0f, 0.3f, 0.6f, 90.0f, 40.0f, 8.0f, false);
        handles[Shockwave] = s.Build();
    }
    {
        Synth s(0.6f);
        s.Tone(0.0f, 0.6f, 0.3f, 440.0f, 880.0f, 3.0f, true);
        s.Tone(0.05f, 0.55f, 0.25f, 554.0f, 1108.0f, 3.0f, true);
        s.Tone(0.1f, 0.5f, 0.25f, 659.0f, 1318.0f, 3.0f, true);
        s.Noise(0.0f, 0.3f, 0.3f, 0.6f, 8.0f);
        handles[SuperStart] = s.Build();
    }
    {
        Synth s(0.9f);
        s.Noise(0.0f, 0.9f, 1.0f, 0.12f, 4.0f);
        s.Tone(0.0f, 0.8f, 0.8f, 120.0f, 35.0f, 3.5f, false);
        handles[KO] = s.Build();
    }
    {
        Synth s(0.12f);
        s.Noise(0.0f, 0.12f, 0.6f, 0.2f, 30.0f);
        s.Tone(0.0f, 0.08f, 0.3f, 200.0f, 160.0f, 30.0f, false);
        handles[Grab] = s.Build();
    }
    {
        Synth s(0.08f);
        s.Noise(0.0f, 0.08f, 0.3f, 0.1f, 50.0f);
        handles[Land] = s.Build();
    }
    {
        Synth s(0.05f);
        s.Tone(0.0f, 0.05f, 0.25f, 880.0f, 880.0f, 20.0f, true);
        handles[Cursor] = s.Build();
    }
    {
        Synth s(0.14f);
        s.Tone(0.0f, 0.06f, 0.3f, 660.0f, 660.0f, 10.0f, true);
        s.Tone(0.06f, 0.08f, 0.3f, 990.0f, 990.0f, 10.0f, true);
        handles[Decide] = s.Build();
    }
    {
        Synth s(0.12f);
        s.Tone(0.0f, 0.12f, 0.25f, 440.0f, 300.0f, 10.0f, true);
        handles[Cancel] = s.Build();
    }
    {
        Synth s(0.35f);
        s.Tone(0.0f, 0.35f, 0.3f, 523.0f, 523.0f, 4.0f, true);
        s.Tone(0.0f, 0.35f, 0.2f, 784.0f, 784.0f, 4.0f, true);
        handles[RoundCall] = s.Build();
    }
    initialized = true;
}

void Sound::Play(Id id) {
    if (!initialized || handles[id] == -1) {
        return;
    }
    PlaySoundMem(handles[id], DX_PLAYTYPE_BACK, TRUE);
}
