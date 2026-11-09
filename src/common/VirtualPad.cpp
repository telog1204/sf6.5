#include "pch.h"
#include "VirtualPad.h"
#include <DxLib.h>
#include "MathUtil.h"
#include "Text.h"

namespace {
    // スティック（左下）
    constexpr float StickX = 118.0f;
    constexpr float StickY = 474.0f;
    constexpr float StickRadius = 80.0f;
    constexpr float StickDeadZone = 18.0f;
    // スティックとして扱う範囲（画面の左下。拡大表示の左の余白も含む）
    constexpr float StickAreaRight = 360.0f;
    constexpr float StickAreaTop = 220.0f;

    // ボタン（右下）
    struct ButtonLayout {
        Btn btn;
        float x;
        float y;
        float r;
        const char* label;
    };
    const ButtonLayout buttons[] = {
        { BtnL, 540.0f, 482.0f, 36.0f, "弱" },
        { BtnM, 630.0f, 452.0f, 36.0f, "中" },
        { BtnH, 720.0f, 422.0f, 36.0f, "強" },
        { BtnSP, 700.0f, 532.0f, 36.0f, "必殺" },
        { BtnStart, 400.0f, 104.0f, 20.0f, "II" },
    };
    // 指がボタンから少しずれても押せるようにする倍率
    constexpr float HitScale = 1.3f;
}

Pad VirtualPad::Read(const std::vector<TouchPoint>& touches) {
    Pad pad;
    for (bool& p : pressed_) {
        p = false;
    }
    stickActive_ = false;
    knobX_ = StickX;
    knobY_ = StickY;

    for (const TouchPoint& t : touches) {
        const float tx = static_cast<float>(t.x);
        const float ty = static_cast<float>(t.y);

        // 一番近いボタン（押せる範囲に入っているもの）
        int best = -1;
        float bestDist = 0.0f;
        for (int i = 0; i < static_cast<int>(sizeof(buttons) / sizeof(buttons[0])); ++i) {
            const float dx = tx - buttons[i].x;
            const float dy = ty - buttons[i].y;
            const float d = std::sqrt(dx * dx + dy * dy);
            if (d <= buttons[i].r * HitScale && (best == -1 || d < bestDist)) {
                best = i;
                bestDist = d;
            }
        }
        if (best != -1) {
            pressed_[buttons[best].btn] = true;
            continue;
        }

        if (!stickActive_ && tx < StickAreaRight && ty > StickAreaTop) {
            stickActive_ = true;
            float dx = tx - StickX;
            float dy = ty - StickY;
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len > StickRadius) {
                knobX_ = StickX + dx / len * StickRadius;
                knobY_ = StickY + dy / len * StickRadius;
            } else {
                knobX_ = tx;
                knobY_ = ty;
            }
            if (len >= StickDeadZone) {
                // 8方向。上は誤ってジャンプしないよう少し入りにくくする
                pad.left = dx < -0.40f * len;
                pad.right = dx > 0.40f * len;
                pad.down = dy > 0.40f * len;
                pad.up = dy < -0.55f * len;
            }
        }
    }
    for (int i = 0; i < BtnCount; ++i) {
        pad.btn[i] = pressed_[i];
    }
    return pad;
}

void VirtualPad::Draw() const {
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
    DrawCircleAA(StickX, StickY, StickRadius, 40, GetColor(30, 30, 40), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
    DrawCircleAA(StickX, StickY, StickRadius, 40, GetColor(200, 200, 220), FALSE, 3.0f);
    DrawCircleAA(knobX_, knobY_, 34.0f, 32, stickActive_ ? GetColor(255, 230, 120) : GetColor(180, 180, 200), TRUE);

    for (const ButtonLayout& b : buttons) {
        const bool on = pressed_[b.btn];
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, on ? 220 : 130);
        unsigned fill = GetColor(60, 60, 80);
        if (b.btn == BtnL) {
            fill = on ? GetColor(120, 200, 255) : GetColor(40, 90, 150);
        } else if (b.btn == BtnM) {
            fill = on ? GetColor(255, 230, 110) : GetColor(150, 120, 30);
        } else if (b.btn == BtnH) {
            fill = on ? GetColor(255, 120, 110) : GetColor(150, 40, 40);
        } else if (b.btn == BtnSP) {
            fill = on ? GetColor(220, 150, 255) : GetColor(100, 50, 140);
        } else {
            fill = on ? GetColor(200, 200, 200) : GetColor(70, 70, 80);
        }
        DrawCircleAA(b.x, b.y, b.r, 32, fill, TRUE);
        DrawCircleAA(b.x, b.y, b.r, 32, GetColor(230, 230, 240), FALSE, 2.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
        const Text::Font font = (b.btn == BtnStart || b.btn == BtnSP) ? Text::Small : Text::Medium;
        Text::Draw(font, static_cast<int>(b.x), static_cast<int>(b.y) - Text::Height(font) / 2, b.label,
                   GetColor(255, 255, 255), GetColor(0, 0, 0), Text::AlignCenter);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
