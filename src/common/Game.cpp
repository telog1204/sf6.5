#include "pch.h"
#include "Game.h"
#include <DxLib.h>
#include "Config.h"
#include "Sound.h"
#include "Text.h"

namespace {
    constexpr int FadeFrames = 12;
}

Game::Game() {
    Text::Init();
    Sound::Init();
    scene_ = MakeTitleScene();
    fadeIn_ = FadeFrames;
}

Game::~Game() = default;

void Game::Update() {
    ++app_.frame;
    app_.input.Poll(scene_ != nullptr && scene_->UsesVirtualPad() && pending_ == nullptr);

    if (pending_ != nullptr) {
        // 前の画面を暗くしてから切り替える
        if (++fadeOut_ >= FadeFrames) {
            scene_ = std::move(pending_);
            fadeOut_ = 0;
            fadeIn_ = FadeFrames;
        }
        return;
    }
    if (fadeIn_ > 0) {
        --fadeIn_;
    }
    scene_->Update(app_);
    if (app_.next != nullptr) {
        pending_ = std::move(app_.next);
        fadeOut_ = 0;
    }
}

void Game::Draw() {
    if (scene_ != nullptr) {
        scene_->Draw(app_);
    }
    int alpha = 0;
    if (pending_ != nullptr) {
        alpha = 255 * fadeOut_ / FadeFrames;
    } else if (fadeIn_ > 0) {
        alpha = 255 * fadeIn_ / FadeFrames;
    }
    if (alpha > 0) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(0, 0, Config::ScreenWidth, Config::ScreenHeight, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}
