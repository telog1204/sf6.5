#pragma once

// 文字の表示。フォントは起動時に一度だけ作る
namespace Text {
    enum Font {
        Small,   // 16px：説明文
        Medium,  // 24px：メニュー
        Large,   // 40px：見出し・タイマー
        Huge,    // 80px：ラウンドの開始や K.O. の表示
        FontCount
    };

    enum Align {
        AlignLeft,
        AlignCenter,
        AlignRight
    };

    void Init();

    // x, y は文字の左上（AlignCenter は上端の中央、AlignRight は右上）
    void Draw(Font font, int x, int y, const char* str, unsigned color, unsigned edgeColor = 0, Align align = AlignLeft);
    int Width(Font font, const char* str);
    int Height(Font font);
}
