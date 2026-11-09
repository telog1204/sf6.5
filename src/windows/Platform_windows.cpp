#include "pch.h"
#include "Platform.h"
#include <DxLib.h>

// Windows：キーボード・パッド・マウスで操作し、ウィンドウ（Config::ScreenWidth × Config::ScreenHeight）にそのまま描く

InputState Platform::PollInput() {
    InputState state;
    GetMousePoint(&state.x, &state.y);
    const int input = GetMouseInput();
    state.primaryDown = (input & MOUSE_INPUT_LEFT) != 0;
    state.secondaryDown = (input & MOUSE_INPUT_RIGHT) != 0;
    return state;
}

std::vector<TouchPoint> Platform::GetTouches() {
    std::vector<TouchPoint> touches;
    if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
        TouchPoint t;
        GetMousePoint(&t.x, &t.y);
        touches.push_back(t);
    }
    return touches;
}

Pad Platform::ReadKeyboard(int set) {
    char keys[256] = {};
    GetHitKeyStateAll(keys);
    auto k = [&keys](int code) { return keys[code] != 0; };

    Pad pad;
    if (set == 0) {
        // 1P：WASD で移動、J・K・L で弱・中・強、I で必殺ショートカット
        pad.up = k(KEY_INPUT_W);
        pad.down = k(KEY_INPUT_S);
        pad.left = k(KEY_INPUT_A);
        pad.right = k(KEY_INPUT_D);
        pad.btn[BtnL] = k(KEY_INPUT_J);
        pad.btn[BtnM] = k(KEY_INPUT_K);
        pad.btn[BtnH] = k(KEY_INPUT_L);
        pad.btn[BtnSP] = k(KEY_INPUT_I);
        pad.btn[BtnStart] = k(KEY_INPUT_RETURN) || k(KEY_INPUT_SPACE);
        pad.btn[BtnBack] = k(KEY_INPUT_ESCAPE) || k(KEY_INPUT_BACK);
    } else {
        // 2P：矢印キーで移動、テンキーの 1・2・3（または , . /）で弱・中・強、テンキーの 0（または右Shift）で必殺ショートカット
        pad.up = k(KEY_INPUT_UP);
        pad.down = k(KEY_INPUT_DOWN);
        pad.left = k(KEY_INPUT_LEFT);
        pad.right = k(KEY_INPUT_RIGHT);
        pad.btn[BtnL] = k(KEY_INPUT_NUMPAD1) || k(KEY_INPUT_COMMA);
        pad.btn[BtnM] = k(KEY_INPUT_NUMPAD2) || k(KEY_INPUT_PERIOD);
        pad.btn[BtnH] = k(KEY_INPUT_NUMPAD3) || k(KEY_INPUT_SLASH);
        pad.btn[BtnSP] = k(KEY_INPUT_NUMPAD0) || k(KEY_INPUT_RSHIFT);
        pad.btn[BtnStart] = k(KEY_INPUT_NUMPADENTER);
    }
    return pad;
}

bool Platform::UsesTouchControls() {
    return false;
}

bool Platform::SupportsLocalVersus() {
    return true;
}

bool Platform::CanQuitFromMenu() {
    return true;
}

std::string Platform::SaveFilePath(const char* fileName) {
    // 実行ファイルを起動したフォルダ（作業フォルダ）に置く
    return fileName;
}

void Platform::BeginDraw() {
}

void Platform::EndDraw() {
    ScreenFlip();
}
