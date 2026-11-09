#pragma once

// 1つの操作機器（キーボード・パッド・画面上の仮想パッド・CPU）の、1フレーム分の入力。
// 方向は画面上の上下左右。キャラから見た前後への変換は InputHistory で行う
enum Btn {
    BtnL,      // 弱攻撃
    BtnM,      // 中攻撃
    BtnH,      // 強攻撃
    BtnSP,     // 必殺技ショートカット
    BtnStart,  // ポーズ・決定
    BtnBack,   // 戻る
    BtnCount
};

struct Pad {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool btn[BtnCount] = {};

    // 別の機器の入力を重ねる（どちらかで押されていれば押している）
    void Merge(const Pad& o) {
        up = up || o.up;
        down = down || o.down;
        left = left || o.left;
        right = right || o.right;
        for (int i = 0; i < BtnCount; ++i) {
            btn[i] = btn[i] || o.btn[i];
        }
    }

    // 反対の方向が同時に押されているときは、その向きを中立にする
    void Clean() {
        if (up && down) {
            up = false;
            down = false;
        }
        if (left && right) {
            left = false;
            right = false;
        }
    }
};
