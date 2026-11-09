#pragma once
#include "Pad.h"
#include "Platform.h"
#include "VirtualPad.h"

// 操作機器
enum class Device {
    Keyboard1,  // キーボードの 1P 用のキー
    Keyboard2,  // キーボードの 2P 用のキー
    Joypad1,
    Joypad2,
    Touch,      // 画面上の仮想パッド
    Count
};

// メニュー操作のための、押した瞬間と長押しのくり返しの判定
class MenuNav {
public:
    void Update(const Pad& pad);
    bool Pressed(Btn b) const { return now_.btn[b] && !prev_.btn[b]; }
    bool Confirm() const { return Pressed(BtnL) || Pressed(BtnSP) || Pressed(BtnStart); }
    bool Cancel() const { return Pressed(BtnH) || Pressed(BtnBack); }
    int MoveX() const { return moveX_; }  // -1：左、1：右（押しっぱなしで一定間隔ごとにくり返す）
    int MoveY() const { return moveY_; }  // -1：上、1：下

private:
    Pad now_;
    Pad prev_;
    int holdX_ = 0;
    int holdY_ = 0;
    int moveX_ = 0;
    int moveY_ = 0;
};

// すべての操作機器の入力をまとめて読み、プレイヤーごとの入力やメニュー操作に分けて渡す
class InputSystem {
public:
    // 1フレームに1回呼ぶ。virtualPadActive が true のときは、タッチを仮想パッドとして読む
    void Poll(bool virtualPadActive);

    // 対戦で使うプレイヤーの入力。separate が false（CPU戦・トレーニング）のときは、どの機器でも 1P を操作できる
    Pad PlayerPad(int player, bool separate) const;

    // メニュー操作（すべての機器をまとめたもの）
    const MenuNav& Menu() const { return menu_; }
    // 2人対戦のキャラ選択などで使う、プレイヤーごとのメニュー操作
    const MenuNav& PlayerMenu(int player) const { return playerMenu_[player]; }

    // タップ・クリックした瞬間か。x, y にその位置を入れる
    bool Tapped(int& x, int& y) const;
    const InputState& Pointer() const { return pointer_; }

    const VirtualPad& Virtual() const { return virtual_; }

private:
    Pad devices_[static_cast<int>(Device::Count)];
    MenuNav menu_;
    MenuNav playerMenu_[2];
    InputState pointer_;
    InputState prevPointer_;
    VirtualPad virtual_;
};
