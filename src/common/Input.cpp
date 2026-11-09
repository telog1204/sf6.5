#include "pch.h"
#include "Input.h"
#include <DxLib.h>

namespace {
    // メニューで方向を押しっぱなしにしたとき、くり返し始めるまでと、くり返しの間隔（フレーム）
    constexpr int RepeatDelay = 18;
    constexpr int RepeatInterval = 6;

    int RepeatStep(bool negative, bool positive, int& hold) {
        const int dir = negative ? -1 : (positive ? 1 : 0);
        if (dir == 0) {
            hold = 0;
            return 0;
        }
        if ((hold > 0 && dir < 0) || (hold < 0 && dir > 0)) {
            hold = 0;
        }
        hold += dir;
        const int n = hold < 0 ? -hold : hold;
        if (n == 1 || (n > RepeatDelay && (n - RepeatDelay) % RepeatInterval == 0)) {
            return dir;
        }
        return 0;
    }

    // ゲームパッド（Xbox 系の配置）：X＝弱、Y＝中、B＝強、A＝必殺、RB＝強、LB＝必殺、START＝ポーズ、BACK＝戻る
    Pad ReadJoypad(int index) {
        const int s = GetJoypadInputState(index == 0 ? DX_INPUT_PAD1 : DX_INPUT_PAD2);
        Pad pad;
        pad.up = (s & PAD_INPUT_UP) != 0;
        pad.down = (s & PAD_INPUT_DOWN) != 0;
        pad.left = (s & PAD_INPUT_LEFT) != 0;
        pad.right = (s & PAD_INPUT_RIGHT) != 0;
        pad.btn[BtnL] = (s & PAD_INPUT_3) != 0;
        pad.btn[BtnM] = (s & PAD_INPUT_4) != 0;
        pad.btn[BtnH] = (s & (PAD_INPUT_2 | PAD_INPUT_6)) != 0;
        pad.btn[BtnSP] = (s & (PAD_INPUT_1 | PAD_INPUT_5)) != 0;
        pad.btn[BtnStart] = (s & PAD_INPUT_8) != 0;
        pad.btn[BtnBack] = (s & PAD_INPUT_7) != 0;
        return pad;
    }
}

void MenuNav::Update(const Pad& pad) {
    prev_ = now_;
    now_ = pad;
    now_.Clean();
    moveX_ = RepeatStep(now_.left, now_.right, holdX_);
    moveY_ = RepeatStep(now_.up, now_.down, holdY_);
}

void InputSystem::Poll(bool virtualPadActive) {
    devices_[static_cast<int>(Device::Keyboard1)] = Platform::ReadKeyboard(0);
    devices_[static_cast<int>(Device::Keyboard2)] = Platform::ReadKeyboard(1);
    devices_[static_cast<int>(Device::Joypad1)] = ReadJoypad(0);
    devices_[static_cast<int>(Device::Joypad2)] = ReadJoypad(1);
    if (virtualPadActive && Platform::UsesTouchControls()) {
        devices_[static_cast<int>(Device::Touch)] = virtual_.Read(Platform::GetTouches());
    } else {
        devices_[static_cast<int>(Device::Touch)] = Pad();
    }

    prevPointer_ = pointer_;
    pointer_ = Platform::PollInput();

    Pad all;
    for (const Pad& p : devices_) {
        all.Merge(p);
    }
    menu_.Update(all);
    playerMenu_[0].Update(PlayerPad(0, true));
    playerMenu_[1].Update(PlayerPad(1, true));
}

Pad InputSystem::PlayerPad(int player, bool separate) const {
    Pad pad;
    if (!separate) {
        if (player == 0) {
            for (const Pad& p : devices_) {
                pad.Merge(p);
            }
        }
        return pad;
    }
    if (player == 0) {
        pad.Merge(devices_[static_cast<int>(Device::Keyboard1)]);
        pad.Merge(devices_[static_cast<int>(Device::Joypad1)]);
        pad.Merge(devices_[static_cast<int>(Device::Touch)]);
    } else {
        pad.Merge(devices_[static_cast<int>(Device::Keyboard2)]);
        pad.Merge(devices_[static_cast<int>(Device::Joypad2)]);
    }
    return pad;
}

bool InputSystem::Tapped(int& x, int& y) const {
    if (pointer_.primaryDown && !prevPointer_.primaryDown) {
        x = pointer_.x;
        y = pointer_.y;
        return true;
    }
    return false;
}
