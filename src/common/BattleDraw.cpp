#include "pch.h"
#include "Battle.h"
#include <DxLib.h>
#include "Config.h"
#include "Stage.h"
#include "Text.h"

// Battle の描画（背景・キャラ・飛び道具・演出・体力ゲージなど）

namespace {
    constexpr float GroundY = Config::GroundScreenY;

    unsigned Col(const Rgb& c) {
        return GetColor(c.r, c.g, c.b);
    }

    const unsigned kWhite = 0xFFFFFFu;

    void DrawBar(int x0, int y0, int x1, int y1, float ratio, bool fromRight, unsigned color) {
        ratio = Clampf(ratio, 0.0f, 1.0f);
        const int w = static_cast<int>((x1 - x0) * ratio);
        if (w <= 0) {
            return;
        }
        if (fromRight) {
            DrawBox(x1 - w, y0, x1, y1, color, TRUE);
        } else {
            DrawBox(x0, y0, x0 + w, y1, color, TRUE);
        }
    }
}

void Battle::Draw() const {
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (shakeFrames_ > 0) {
        const float k = shakePower_ * std::min(1.0f, shakeFrames_ / 12.0f);
        shakeX = std::sin(frame_ * 2.3f) * k;
        shakeY = std::cos(frame_ * 3.1f) * k * 0.6f;
    }
    const float camLeft = camX_ - Config::ScreenWidth * 0.5f;

    DrawStage(setup_.stage, camLeft - shakeX, frame_);

    if (superFreeze_ > 0) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
        DrawBox(0, 0, Config::ScreenWidth, Config::ScreenHeight, GetColor(0, 0, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 影
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
    for (int i = 0; i < 2; ++i) {
        const Fighter& f = f_[i];
        const float k = Clampf(1.0f - f.y / 400.0f, 0.4f, 1.0f);
        DrawOvalAA(f.x - camLeft + shakeX, GroundY + 4.0f + shakeY, 42.0f * f.Scale() * k, 9.0f * k, 24, GetColor(0, 0, 0), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 攻撃している側を手前に描く
    int back = 1;
    int front = 0;
    if ((f_[1].state == FState::Attack || f_[1].state == FState::Throwing) &&
        !(f_[0].state == FState::Attack || f_[0].state == FState::Throwing)) {
        back = 0;
        front = 1;
    }
    f_[back].Draw(camLeft, GroundY, shakeX, shakeY, frame_, superFreeze_ > 0 && superSide_ == back);
    f_[front].Draw(camLeft, GroundY, shakeX, shakeY, frame_, superFreeze_ > 0 && superSide_ == front);

    for (const Projectile& p : projectiles_) {
        DrawProjectile(p, camLeft - shakeX);
    }
    DrawEffects(camLeft - shakeX);
    if (showBoxes) {
        DrawBoxes(camLeft);
    }
    DrawHud();
}

void Battle::DrawProjectile(const Projectile& p, float camLeft) const {
    const float sx = p.x - camLeft;
    const float sy = GroundY - p.y;
    const Rgb c = f_[p.owner].def->effect;
    const float dir = p.vx >= 0.0f ? 1.0f : -1.0f;
    if (p.move->groundWave) {
        // 地面を走る衝撃波：突き出る岩と土煙
        for (int i = 0; i < 4; ++i) {
            const float ox = sx - dir * i * 14.0f;
            const float h = (34.0f - i * 7.0f) * (0.8f + 0.2f * std::sin(p.age * 0.9f + i));
            DrawTriangleAA(ox - 10.0f, sy, ox + dir * 2.0f, sy - h, ox + 10.0f, sy, GetColor(120, 95, 75), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_ADD, 140);
        DrawOvalAA(sx, sy - 10.0f, 30.0f, 14.0f, 24, Col(c), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawOvalAA(sx - dir * 26.0f, sy - 8.0f, 26.0f, 10.0f, 20, GetColor(170, 150, 130), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        return;
    }
    // 風の刃：三日月形の光と尾
    SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
    for (int i = 1; i <= 4; ++i) {
        DrawOvalAA(sx - dir * i * 16.0f, sy, 22.0f - i * 3.0f, 14.0f - i * 2.0f, 20, Col(c), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
    DrawOvalAA(sx, sy, 26.0f, 20.0f, 28, Col(c), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    DrawOvalAA(sx + dir * 4.0f, sy, 14.0f, 13.0f, 24, kWhite, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    const float spin = p.age * 0.5f;
    for (int i = 0; i < 3; ++i) {
        const float a = spin + i * 2.09f;
        DrawLineAA(sx + std::cos(a) * 8.0f, sy + std::sin(a) * 8.0f, sx + std::cos(a) * 22.0f, sy + std::sin(a) * 18.0f,
                   GetColor(230, 250, 255), 2.0f);
    }
}

void Battle::DrawEffects(float camLeft) const {
    for (const Effect& e : effects_) {
        const float sx = e.x - camLeft;
        const float sy = GroundY - e.y;
        const float t = static_cast<float>(e.age) / e.life;
        const int alpha = static_cast<int>(255.0f * (1.0f - t));
        switch (e.type) {
        case Effect::HitSpark: {
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            const float r = (12.0f + 40.0f * t) * e.size;
            if (e.age < 4) {
                DrawCircleAA(sx, sy, 22.0f * e.size, 24, kWhite, TRUE);
            }
            for (int i = 0; i < 10; ++i) {
                const float a = i * 0.628f + e.x * 0.01f;
                const float len = r * ((i % 2) ? 1.0f : 0.6f);
                DrawLineAA(sx + std::cos(a) * r * 0.3f, sy + std::sin(a) * r * 0.3f, sx + std::cos(a) * len, sy + std::sin(a) * len,
                           Col(e.color), 4.0f * e.size * (1.0f - t) + 1.0f);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }
        case Effect::BlockSpark:
        case Effect::Clash: {
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            const float r = (10.0f + 34.0f * t) * e.size;
            DrawCircleAA(sx, sy, r, 32, Col(e.color), FALSE, 4.0f * (1.0f - t) + 1.0f);
            DrawCircleAA(sx, sy, r * 0.5f, 24, kWhite, FALSE, 2.0f);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }
        case Effect::Dust: {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 2);
            for (int i = -1; i <= 1; i += 2) {
                const float ox = sx + i * (10.0f + 50.0f * t) * e.size;
                DrawOvalAA(ox, sy - 6.0f - 10.0f * t, (14.0f + 20.0f * t) * e.size, (6.0f + 8.0f * t) * e.size, 20, Col(e.color), TRUE);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }
        case Effect::SuperBurst: {
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            for (int k = 0; k < 3; ++k) {
                const float r = std::fmod(t * 3.0f + k / 3.0f, 1.0f) * 220.0f;
                DrawCircleAA(sx, sy, r, 48, Col(e.color), FALSE, 6.0f);
            }
            for (int i = 0; i < 16; ++i) {
                const float a = i * 0.3927f + t * 2.0f;
                DrawLineAA(sx + std::cos(a) * 60.0f, sy + std::sin(a) * 60.0f, sx + std::cos(a) * 400.0f, sy + std::sin(a) * 400.0f,
                           Col(e.color), 2.0f);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }
        case Effect::Popup: {
            const int y = static_cast<int>(sy - 20.0f * t);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::min(255, alpha * 2));
            Text::Draw(Text::Medium, static_cast<int>(sx), y, e.text.c_str(), Col(e.color), GetColor(0, 0, 0), Text::AlignCenter);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }
        }
    }
}

void Battle::DrawBoxes(float camLeft) const {
    auto box = [camLeft](const Box& b, unsigned color) {
        DrawBoxAA(b.x0 - camLeft, GroundY - b.y1, b.x1 - camLeft, GroundY - b.y0, color, FALSE, 2.0f);
    };
    for (int i = 0; i < 2; ++i) {
        const Fighter& f = f_[i];
        box(f.PushBox(), GetColor(255, 255, 0));
        Box hurts[2];
        const int n = f.Hurtboxes(hurts);
        for (int k = 0; k < n; ++k) {
            box(hurts[k], GetColor(80, 160, 255));
        }
        Box hb;
        int idx = 0;
        if (f.Hitbox(hb, idx)) {
            box(hb, GetColor(255, 60, 60));
        }
    }
    for (const Projectile& p : projectiles_) {
        box(p.Bounds(), GetColor(255, 60, 60));
    }
}

void Battle::DrawHud() const {
    const unsigned black = GetColor(0, 0, 0);
    const bool training = setup_.mode == BattleMode::Training;

    // 体力ゲージ（外側の端から伸びる）
    const int barTop = 22;
    const int barBottom = 44;
    const int leftX0 = 24;
    const int leftX1 = 352;
    const int rightX0 = 448;
    const int rightX1 = 776;
    for (int i = 0; i < 2; ++i) {
        const Fighter& f = f_[i];
        const int x0 = i == 0 ? leftX0 : rightX0;
        const int x1 = i == 0 ? leftX1 : rightX1;
        DrawBox(x0 - 3, barTop - 3, x1 + 3, barBottom + 3, GetColor(20, 20, 30), TRUE);
        DrawBox(x0, barTop, x1, barBottom, GetColor(60, 20, 20), TRUE);
        const float maxHp = static_cast<float>(f.def->maxHp);
        DrawBar(x0, barTop, x1, barBottom, f.redHp / maxHp, i == 1, GetColor(220, 50, 40));
        const float ratio = f.hp / maxHp;
        unsigned fill = GetColor(250, 210, 60);
        if (ratio < 0.3f) {
            fill = (frame_ / 8) % 2 == 0 ? GetColor(255, 120, 40) : GetColor(255, 170, 60);
        }
        DrawBar(x0, barTop, x1, barTop + 8, ratio, i == 1, GetColor(255, 245, 170));
        DrawBar(x0, barTop + 8, x1, barBottom, ratio, i == 1, fill);
        DrawBox(x0 - 3, barTop - 3, x1 + 3, barBottom + 3, GetColor(230, 230, 240), FALSE);

        // 名前
        std::string name = f.def->name;
        if (cpu_[i] || (training && i == 1)) {
            name += training ? "（ダミー）" : "（CPU）";
        }
        Text::Draw(Text::Small, i == 0 ? x0 : x1, barBottom + 6, name.c_str(), kWhite, black,
                   i == 0 ? Text::AlignLeft : Text::AlignRight);

        // 取ったラウンド
        if (!training) {
            for (int k = 0; k < Config::RoundsToWin; ++k) {
                const float cx = i == 0 ? static_cast<float>(leftX1 - 10 - k * 22) : static_cast<float>(rightX0 + 10 + k * 22);
                const bool won = f.roundWins > k;
                DrawCircleAA(cx, static_cast<float>(barBottom + 15), 8.0f, 20, won ? GetColor(255, 200, 60) : GetColor(40, 40, 50), TRUE);
                DrawCircleAA(cx, static_cast<float>(barBottom + 15), 8.0f, 20, GetColor(230, 230, 240), FALSE, 2.0f);
            }
        }

        // 超必殺技ゲージ
        const int my0 = 78;
        const int my1 = 88;
        const int mx0 = i == 0 ? 24 : 556;
        const int mx1 = i == 0 ? 244 : 776;
        DrawBox(mx0 - 2, my0 - 2, mx1 + 2, my1 + 2, GetColor(20, 20, 30), TRUE);
        const float mratio = static_cast<float>(f.meter) / Config::MeterMax;
        const bool full = f.meter >= Config::MeterMax;
        const unsigned mcol = full ? ((frame_ / 6) % 2 == 0 ? GetColor(120, 240, 255) : GetColor(255, 255, 255)) : GetColor(60, 140, 230);
        DrawBar(mx0, my0, mx1, my1, mratio, i == 1, mcol);
        DrawBox(mx0 - 2, my0 - 2, mx1 + 2, my1 + 2, GetColor(180, 180, 200), FALSE);
        if (full) {
            Text::Draw(Text::Small, i == 0 ? mx1 + 8 : mx0 - 8, my0 - 4, "SUPER", GetColor(120, 240, 255), black,
                       i == 0 ? Text::AlignLeft : Text::AlignRight);
        }

        // コンボ数
        if (comboTimer_[i] > 0 && comboShow_[i] >= 2) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%d HIT", comboShow_[i]);
            const int x = i == 0 ? 24 : 776;
            const Text::Align al = i == 0 ? Text::AlignLeft : Text::AlignRight;
            Text::Draw(Text::Large, x, 150, buf, GetColor(255, 220, 80), black, al);
            std::snprintf(buf, sizeof(buf), "%d ダメージ", lastComboDamage_[i]);
            Text::Draw(Text::Small, x, 194, buf, kWhite, black, al);
        }
    }

    // タイマー
    DrawBox(372, 12, 428, 60, GetColor(20, 20, 30), TRUE);
    DrawBox(372, 12, 428, 60, GetColor(230, 230, 240), FALSE);
    if (training) {
        Text::Draw(Text::Large, 400, 15, "∞", kWhite, black, Text::AlignCenter);
        Text::Draw(Text::Small, 400, 140, "トレーニング（ポーズでメニュー）", GetColor(200, 230, 255), black, Text::AlignCenter);
    } else {
        char buf[16];
        const int sec = (timerFrames_ + Config::LogicFps - 1) / Config::LogicFps;
        std::snprintf(buf, sizeof(buf), "%02d", sec);
        Text::Draw(Text::Large, 400, 15, buf, sec <= 10 ? GetColor(255, 120, 100) : kWhite, black, Text::AlignCenter);
    }

    // ラウンドの開始・終わりの表示
    const int cy = 220;
    char big[64] = "";
    unsigned bigColor = GetColor(255, 230, 120);
    if (phase_ == Phase::Intro) {
        if (phaseFrame_ >= 10 && phaseFrame_ < 64) {
            const bool isFinal = f_[0].roundWins == Config::RoundsToWin - 1 && f_[1].roundWins == Config::RoundsToWin - 1;
            if (isFinal) {
                std::snprintf(big, sizeof(big), "FINAL ROUND");
            } else {
                std::snprintf(big, sizeof(big), "ROUND %d", round_);
            }
        } else if (phaseFrame_ >= 66) {
            std::snprintf(big, sizeof(big), "FIGHT!");
            bigColor = GetColor(255, 120, 80);
        }
    } else if (phase_ == Phase::KO) {
        if (phaseFrame_ < 160) {
            std::snprintf(big, sizeof(big), "%s", doubleKo_ ? "DOUBLE K.O." : "K.O.");
            bigColor = GetColor(255, 80, 60);
        }
    } else if (phase_ == Phase::TimeUp) {
        std::snprintf(big, sizeof(big), "TIME UP");
    } else if (phase_ == Phase::MatchOver) {
        if (matchWinner_ < 0) {
            std::snprintf(big, sizeof(big), "DRAW");
        } else {
            std::snprintf(big, sizeof(big), "%s WIN", matchWinner_ == 0 ? "1P" : "2P");
        }
    }
    if (big[0] != '\0') {
        Text::Draw(Text::Huge, 400, cy, big, bigColor, black, Text::AlignCenter);
    }
    if (phase_ == Phase::MatchOver && matchWinner_ >= 0) {
        std::string s = std::string(f_[matchWinner_].def->name) + " の勝利";
        Text::Draw(Text::Medium, 400, cy + 96, s.c_str(), kWhite, black, Text::AlignCenter);
    }
    if (phase_ == Phase::TimeUp && phaseFrame_ >= 60) {
        const char* s = roundWinner_ < 0 ? "引き分け" : (roundWinner_ == 0 ? "1P の勝ち" : "2P の勝ち");
        Text::Draw(Text::Medium, 400, cy + 96, s, kWhite, black, Text::AlignCenter);
    }
}
