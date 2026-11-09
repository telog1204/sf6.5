#include "pch.h"
#include "Text.h"
#include <DxLib.h>

namespace {
    int fonts[Text::FontCount] = { -1, -1, -1, -1 };
    const int sizes[Text::FontCount] = { 16, 24, 40, 80 };
}

void Text::Init() {
    fonts[Small] = CreateFontToHandle(nullptr, sizes[Small], 4, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 1);
    fonts[Medium] = CreateFontToHandle(nullptr, sizes[Medium], 6, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);
    fonts[Large] = CreateFontToHandle(nullptr, sizes[Large], 8, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);
    fonts[Huge] = CreateFontToHandle(nullptr, sizes[Huge], 9, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 4);
}

int Text::Width(Font font, const char* str) {
    if (fonts[font] == -1) {
        return 0;
    }
    return GetDrawStringWidthToHandle(str, static_cast<int>(std::strlen(str)), fonts[font]);
}

int Text::Height(Font font) {
    return sizes[font];
}

void Text::Draw(Font font, int x, int y, const char* str, unsigned color, unsigned edgeColor, Align align) {
    if (fonts[font] == -1) {
        return;
    }
    if (align != AlignLeft) {
        const int w = Width(font, str);
        x -= (align == AlignCenter) ? w / 2 : w;
    }
    DrawStringToHandle(x, y, str, color, fonts[font], edgeColor);
}
