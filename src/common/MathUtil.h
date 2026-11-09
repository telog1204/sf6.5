#pragma once
#include <cmath>

constexpr float kPi = 3.14159265358979f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 LerpVec(const Vec2& a, const Vec2& b, float t) {
    return Vec2{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

inline float Lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float Clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline int Clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float SmoothStep(float t) {
    t = Clampf(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// 長方形。対戦の判定では、x は右が正、y は上が正（地面が 0）のワールド座標で使う
struct Box {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;

    bool Valid() const { return x1 > x0 && y1 > y0; }
    bool Overlaps(const Box& o) const { return x0 < o.x1 && o.x0 < x1 && y0 < o.y1 && o.y0 < y1; }
};

// 色（0〜255）。DxLib の色は描くときに GetColor で作る
struct Rgb {
    int r = 0;
    int g = 0;
    int b = 0;
};

inline Rgb MixRgb(const Rgb& a, const Rgb& b, float t) {
    return Rgb{ static_cast<int>(Lerpf(static_cast<float>(a.r), static_cast<float>(b.r), t)),
                static_cast<int>(Lerpf(static_cast<float>(a.g), static_cast<float>(b.g), t)),
                static_cast<int>(Lerpf(static_cast<float>(a.b), static_cast<float>(b.b), t)) };
}

inline Rgb ScaleRgb(const Rgb& a, float k) {
    return Rgb{ Clampi(static_cast<int>(a.r * k), 0, 255), Clampi(static_cast<int>(a.g * k), 0, 255),
                Clampi(static_cast<int>(a.b * k), 0, 255) };
}
