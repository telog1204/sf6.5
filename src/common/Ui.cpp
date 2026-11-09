#include "pch.h"
#include "Ui.h"
#include <DxLib.h>
#include "CharacterData.h"
#include "Config.h"

namespace {
    // UTF-8 の1文字のバイト数
    int CharBytes(unsigned char c) {
        if (c < 0x80) {
            return 1;
        }
        if ((c & 0xE0) == 0xC0) {
            return 2;
        }
        if ((c & 0xF0) == 0xE0) {
            return 3;
        }
        return 4;
    }
}

void Ui::Panel(int x0, int y0, int x1, int y1, int alpha, bool highlight) {
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(x0, y0, x1, y1, highlight ? GetColor(70, 50, 30) : GetColor(15, 15, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(x0, y0, x1, y1, highlight ? GetColor(255, 200, 80) : GetColor(140, 140, 170), FALSE);
    if (highlight) {
        DrawBox(x0 + 1, y0 + 1, x1 - 1, y1 - 1, GetColor(255, 200, 80), FALSE);
    }
}

void Ui::Dim(int alpha) {
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(0, 0, Config::ScreenWidth, Config::ScreenHeight, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

int Ui::DrawWrapped(Text::Font font, int x, int y, int maxWidth, const char* text, unsigned color, int lineGap) {
    const std::string s = text;
    const int lineHeight = Text::Height(font) + lineGap;
    int top = y;
    size_t lineStart = 0;
    size_t pos = 0;
    while (pos < s.size()) {
        const int n = CharBytes(static_cast<unsigned char>(s[pos]));
        const std::string candidate = s.substr(lineStart, pos + n - lineStart);
        if (Text::Width(font, candidate.c_str()) > maxWidth && pos > lineStart) {
            Text::Draw(font, x, y, s.substr(lineStart, pos - lineStart).c_str(), color, GetColor(0, 0, 0));
            y += lineHeight;
            lineStart = pos;
        }
        pos += n;
    }
    if (lineStart < s.size()) {
        Text::Draw(font, x, y, s.substr(lineStart).c_str(), color, GetColor(0, 0, 0));
        y += lineHeight;
    }
    return y - top;
}

bool Ui::Inside(int px, int py, int x0, int y0, int x1, int y1) {
    return px >= x0 && px < x1 && py >= y0 && py < y1;
}

void Ui::DrawCharacter(int character, float footX, float footY, float scaleMul, int facing, const Pose& pose, int frame) {
    BodyStyle style = GetCharacter(character).body;
    style.scale *= scaleMul;
    DrawBody(pose, style, footX, footY, facing, frame);
}
