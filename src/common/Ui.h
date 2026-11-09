#pragma once
#include "Pose.h"
#include "Text.h"

// メニュー画面で共通に使う描画
namespace Ui {
    // 半透明の枠
    void Panel(int x0, int y0, int x1, int y1, int alpha = 200, bool highlight = false);
    // 画面全体を暗くする
    void Dim(int alpha);
    // 横幅 maxWidth で折り返して描く。描いた高さを返す
    int DrawWrapped(Text::Font font, int x, int y, int maxWidth, const char* text, unsigned color, int lineGap = 6);
    bool Inside(int px, int py, int x0, int y0, int x1, int y1);
    // キャラを描く（メニュー用。scaleMul で大きさを変える）
    void DrawCharacter(int character, float footX, float footY, float scaleMul, int facing, const Pose& pose, int frame);
}
