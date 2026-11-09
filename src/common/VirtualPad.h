#pragma once
#include <vector>
#include "Pad.h"
#include "Platform.h"

// 画面上の仮想パッド（Android の対戦中に使う）。左下のスティックで移動し、右下のボタンで攻撃する。
// 画面上部中央の「II」ボタンでポーズ
class VirtualPad {
public:
    // 指の位置から、そのフレームの入力を作る
    Pad Read(const std::vector<TouchPoint>& touches);

    // スティックとボタンを描く（押しているものは明るく）
    void Draw() const;

private:
    bool pressed_[BtnCount] = {};
    bool stickActive_ = false;
    float knobX_ = 0.0f;
    float knobY_ = 0.0f;
};
