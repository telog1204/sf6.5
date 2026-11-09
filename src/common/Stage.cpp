#include "pch.h"
#include "Stage.h"
#include <DxLib.h>
#include "Config.h"
#include "MathUtil.h"

namespace {
    constexpr float W = static_cast<float>(Config::ScreenWidth);
    constexpr float H = static_cast<float>(Config::ScreenHeight);

    unsigned Col(const Rgb& c) {
        return GetColor(c.r, c.g, c.b);
    }

    // 縦のグラデーション
    void Gradient(int y0, int y1, const Rgb& top, const Rgb& bottom) {
        const int bands = std::max(1, (y1 - y0) / 6);
        for (int i = 0; i < bands; ++i) {
            const int a = y0 + (y1 - y0) * i / bands;
            const int b = y0 + (y1 - y0) * (i + 1) / bands;
            DrawBox(0, a, Config::ScreenWidth, b + 1, Col(MixRgb(top, bottom, static_cast<float>(i) / bands)), TRUE);
        }
    }

    // 山並み。parallax が小さいほど遠く（ゆっくり動く）
    void Ridge(float cameraLeft, float parallax, float spacing, float baseline, float height, float seed, const Rgb& color) {
        const float offset = cameraLeft * parallax;
        const int k0 = static_cast<int>(std::floor(offset / spacing)) - 2;
        const int k1 = static_cast<int>(std::floor((offset + W) / spacing)) + 2;
        const unsigned c = Col(color);
        for (int k = k0; k <= k1; ++k) {
            const float sx = k * spacing - offset;
            const float h = height * (0.55f + 0.45f * std::sin(k * 1.7f + seed) * std::cos(k * 0.9f + seed * 2.0f));
            DrawTriangleAA(sx - spacing * 0.85f, baseline, sx, baseline - h, sx + spacing * 0.85f, baseline, c, TRUE);
        }
    }

    void DrawSunsetRiver(float cameraLeft, int frame) {
        Gradient(0, 260, Rgb{ 48, 38, 92 }, Rgb{ 200, 100, 110 });
        Gradient(260, 440, Rgb{ 200, 100, 110 }, Rgb{ 255, 190, 120 });

        // 夕日
        const float sunX = 560.0f - cameraLeft * 0.04f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 60);
        DrawCircleAA(sunX, 350.0f, 110.0f, 48, GetColor(255, 150, 80), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
        DrawCircleAA(sunX, 350.0f, 75.0f, 48, GetColor(255, 170, 90), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawCircleAA(sunX, 350.0f, 52.0f, 48, GetColor(255, 225, 160), TRUE);

        // 雲
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        for (int i = 0; i < 5; ++i) {
            const float cx = std::fmod(i * 230.0f - cameraLeft * 0.08f + frame * 0.05f + 2000.0f, 1150.0f) - 150.0f;
            const float cy = 90.0f + static_cast<float>((i * 37) % 120);
            DrawOvalAA(cx, cy, 90.0f, 12.0f, 32, GetColor(255, 170, 160), TRUE);
            DrawOvalAA(cx + 40.0f, cy - 8.0f, 60.0f, 10.0f, 32, GetColor(255, 190, 170), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        Ridge(cameraLeft, 0.12f, 150.0f, 440.0f, 120.0f, 0.3f, Rgb{ 110, 70, 110 });
        Ridge(cameraLeft, 0.3f, 110.0f, 460.0f, 70.0f, 2.1f, Rgb{ 70, 50, 85 });
        DrawBox(0, 438, Config::ScreenWidth, 462, GetColor(70, 50, 85), TRUE);

        // 川
        Gradient(460, 492, Rgb{ 230, 150, 120 }, Rgb{ 120, 110, 150 });
        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        for (int i = 0; i < 14; ++i) {
            const float lx = std::fmod(i * 97.0f - cameraLeft * 0.6f + 4000.0f, 900.0f) - 50.0f;
            const float ly = 466.0f + (i * 7) % 22;
            const float len = 20.0f + (i * 13) % 30 + std::sin(frame * 0.05f + i) * 6.0f;
            DrawLineAA(lx, ly, lx + len, ly, GetColor(255, 220, 160), 2.0f);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 河原の地面
        Gradient(490, Config::ScreenHeight, Rgb{ 150, 130, 100 }, Rgb{ 90, 75, 60 });
        for (float wx = std::floor((cameraLeft - 300.0f) / 100.0f) * 100.0f; wx < cameraLeft + W + 300.0f; wx += 100.0f) {
            const float sx = wx - cameraLeft;
            const float bx = W * 0.5f + (sx - W * 0.5f) * 1.7f;
            DrawLineAA(sx, 490.0f, bx, H, GetColor(120, 100, 80), 2.0f);
        }
        // 石
        for (int i = 0; i < 24; ++i) {
            const float wx = i * 61.0f + (i % 3) * 17.0f;
            const float sx = wx - cameraLeft;
            if (sx < -20.0f || sx > W + 20.0f) {
                continue;
            }
            const float sy = 505.0f + (i * 23) % 80;
            DrawOvalAA(sx, sy, 6.0f + i % 4, 3.0f + i % 2, 12, GetColor(110, 95, 80), TRUE);
        }
        // 葦（手前の草）
        for (int i = 0; i < 30; ++i) {
            const float wx = i * 47.0f;
            const float sx = wx - cameraLeft * 1.15f + 100.0f;
            if (sx < -20.0f || sx > W + 20.0f || (i % 4) == 1) {
                continue;
            }
            const float sway = std::sin(frame * 0.04f + i) * 4.0f;
            DrawLineAA(sx, 600.0f, sx + sway, 560.0f - (i % 3) * 10.0f, GetColor(70, 80, 40), 3.0f);
            DrawLineAA(sx + 6.0f, 600.0f, sx + 10.0f + sway, 572.0f, GetColor(85, 95, 45), 2.0f);
        }
    }

    void DrawNightQuarry(float cameraLeft, int frame) {
        Gradient(0, 440, Rgb{ 8, 10, 28 }, Rgb{ 45, 38, 72 });

        // 星
        for (int i = 0; i < 70; ++i) {
            const float sx = std::fmod(i * 137.0f - cameraLeft * 0.02f + 2000.0f, W);
            const float sy = static_cast<float>((i * 71) % 300);
            const int b = 150 + static_cast<int>(100.0f * (0.5f + 0.5f * std::sin(frame * 0.05f + i * 1.3f)));
            DrawCircleAA(sx, sy, (i % 5 == 0) ? 1.8f : 1.1f, 8, GetColor(b, b, std::min(255, b + 20)), TRUE);
        }
        // 月
        const float moonX = 190.0f - cameraLeft * 0.03f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 50);
        DrawCircleAA(moonX, 110.0f, 70.0f, 40, GetColor(120, 120, 160), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawCircleAA(moonX, 110.0f, 34.0f, 40, GetColor(235, 235, 215), TRUE);
        DrawCircleAA(moonX + 10.0f, 102.0f, 7.0f, 16, GetColor(215, 215, 195), TRUE);
        DrawCircleAA(moonX - 12.0f, 118.0f, 5.0f, 16, GetColor(215, 215, 195), TRUE);

        // 岩壁
        Ridge(cameraLeft, 0.15f, 170.0f, 430.0f, 170.0f, 1.2f, Rgb{ 50, 45, 62 });
        Ridge(cameraLeft, 0.35f, 120.0f, 470.0f, 120.0f, 3.7f, Rgb{ 75, 68, 78 });
        DrawBox(0, 428, Config::ScreenWidth, 472, GetColor(75, 68, 78), TRUE);
        // 岩壁の削り跡
        for (int k = 0; k < 12; ++k) {
            const float sx = std::fmod(k * 140.0f - cameraLeft * 0.35f + 3000.0f, 1100.0f) - 150.0f;
            DrawLineAA(sx, 440.0f, sx + 90.0f, 440.0f, GetColor(95, 88, 98), 2.0f);
            DrawLineAA(sx + 20.0f, 456.0f, sx + 120.0f, 456.0f, GetColor(95, 88, 98), 2.0f);
        }

        // 照明塔
        for (int k = 0; k < 4; ++k) {
            const float sx = k * 420.0f + 150.0f - cameraLeft * 0.6f;
            if (sx < -150.0f || sx > W + 150.0f) {
                continue;
            }
            DrawLineAA(sx, 480.0f, sx, 250.0f, GetColor(40, 40, 50), 5.0f);
            DrawBoxAA(sx - 22.0f, 238.0f, sx + 22.0f, 256.0f, GetColor(60, 60, 70), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 40 + static_cast<int>(10.0f * std::sin(frame * 0.07f + k)));
            DrawTriangleAA(sx - 18.0f, 252.0f, sx + 18.0f, 252.0f, sx + 140.0f, 540.0f, GetColor(255, 240, 180), TRUE);
            DrawTriangleAA(sx - 18.0f, 252.0f, sx - 120.0f, 540.0f, sx + 140.0f, 540.0f, GetColor(255, 240, 180), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
            DrawCircleAA(sx, 247.0f, 10.0f, 16, GetColor(255, 250, 220), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 地面（砂利）
        Gradient(470, Config::ScreenHeight, Rgb{ 110, 104, 98 }, Rgb{ 55, 52, 50 });
        for (float wx = std::floor((cameraLeft - 300.0f) / 100.0f) * 100.0f; wx < cameraLeft + W + 300.0f; wx += 100.0f) {
            const float sx = wx - cameraLeft;
            const float bx = W * 0.5f + (sx - W * 0.5f) * 1.7f;
            DrawLineAA(sx, 470.0f, bx, H, GetColor(85, 80, 76), 2.0f);
        }
        for (int i = 0; i < 28; ++i) {
            const float wx = i * 53.0f + (i % 4) * 11.0f;
            const float sx = wx - cameraLeft;
            if (sx < -30.0f || sx > W + 30.0f) {
                continue;
            }
            const float sy = 490.0f + (i * 29) % 100;
            const float r = 4.0f + (i % 5) * 2.0f;
            DrawTriangleAA(sx - r, sy, sx + r * 0.2f, sy - r * 1.2f, sx + r, sy, GetColor(80, 75, 72), TRUE);
        }
    }
}

const char* StageName(int stage) {
    return stage == 1 ? "夜の採石場" : "夕焼けの河原";
}

void DrawStage(int stage, float cameraLeft, int frame) {
    if (stage == 1) {
        DrawNightQuarry(cameraLeft, frame);
    } else {
        DrawSunsetRiver(cameraLeft, frame);
    }
}
