#include "pch.h"
#include "Pose.h"
#include <DxLib.h>

namespace {
    // 関節の座標を J_Hip から J_FFoot の順に並べてポーズを作る
    Pose MakePose(std::initializer_list<float> v) {
        Pose p;
        auto it = v.begin();
        for (int i = 0; i < J_Count && it != v.end(); ++i) {
            p.j[i].x = *it++;
            p.j[i].y = *it++;
        }
        return p;
    }

    Pose poses[P_Count];
    bool posesReady = false;

    void BuildPoses() {
        //                        腰          首           頭          奥ひじ        奥手          手前ひじ      手前手        奥ひざ        奥足          手前ひざ      手前足
        poses[P_Stand]     = MakePose({ 0, 92,    6, 142,    10, 158,   -6, 112,     18, 126,     26, 118,     40, 138,    -14, 48,    -28, 0,      22, 50,      24, 0 });
        poses[P_Stand2]    = MakePose({ 0, 89,    5, 139,     9, 155,   -6, 109,     18, 122,     26, 115,     40, 134,    -15, 46,    -28, 0,      23, 48,      24, 0 });
        poses[P_WalkA]     = MakePose({ 2, 92,    8, 142,    12, 158,   -4, 112,     20, 126,     28, 118,     42, 138,     -4, 48,    -14, 0,      26, 48,      36, 2 });
        poses[P_WalkB]     = MakePose({ -2, 92,   4, 142,     8, 158,   -8, 112,     16, 126,     24, 118,     38, 138,    -22, 46,    -36, 2,      10, 50,       6, 0 });
        poses[P_Crouch]    = MakePose({ 0, 58,   16, 104,    22, 119,    8, 74,      32, 86,      36, 82,      50, 100,    -12, 18,    -48, 0,      44, 40,      30, 0 });
        poses[P_PreJump]   = MakePose({ 0, 72,   10, 120,    15, 136,   -4, 94,      18, 104,     28, 98,      40, 116,    -16, 38,    -28, 0,      26, 40,      24, 0 });
        poses[P_JumpUp]    = MakePose({ 0, 90,    4, 140,     8, 156,  -10, 118,     10, 128,     24, 116,     36, 134,     14, 52,    -12, 40,      36, 74,      16, 48 });
        poses[P_JumpFall]  = MakePose({ 0, 90,    5, 140,     9, 156,  -12, 124,      4, 140,     26, 124,     40, 142,     -8, 50,    -24, 10,      24, 52,      20, 8 });
        poses[P_DashF]     = MakePose({ 4, 86,   24, 132,    32, 146,    0, 108,    -14, 96,      30, 112,     46, 126,    -24, 40,    -44, 10,      30, 52,      30, 4 });
        poses[P_DashB]     = MakePose({ -4, 90, -12, 140,   -14, 156,  -14, 112,      8, 124,     16, 116,     30, 134,    -20, 40,    -30, 12,      16, 46,      30, 6 });
        poses[P_Jab]       = MakePose({ 2, 92,   12, 142,    16, 158,    0, 114,     22, 128,     38, 134,     66, 136,    -14, 48,    -28, 0,      22, 50,      24, 0 });
        poses[P_Straight]  = MakePose({ 6, 90,   22, 138,    28, 153,    4, 112,     14, 122,     46, 130,     74, 128,    -12, 46,    -32, 0,      30, 48,      36, 0 });
        poses[P_HighKick]  = MakePose({ -4, 92, -18, 138,   -24, 153,  -36, 114,    -44, 92,      0, 112,     20, 124,     -6, 46,     -6, 0,      36, 116,     80, 136 });
        poses[P_RaiseArms] = MakePose({ -2, 92,   0, 142,     2, 158,  -14, 160,     -6, 182,     10, 162,      4, 184,    -14, 48,    -28, 0,      22, 50,      24, 0 });
        poses[P_Hammer]    = MakePose({ 6, 88,   26, 132,    34, 145,   10, 110,     24, 100,     48, 134,     72, 112,    -12, 46,    -32, 0,      30, 48,      36, 0 });
        poses[P_LowKick]   = MakePose({ 0, 58,   14, 104,    20, 119,    8, 74,      32, 86,      34, 82,      48, 98,     -12, 18,    -48, 0,      38, 30,      78, 14 });
        poses[P_LowKick2]  = MakePose({ -4, 54,   6, 100,    10, 116,   -2, 72,      20, 82,      24, 78,      40, 92,     -16, 16,    -50, 0,      36, 24,      84, 10 });
        poses[P_Sweep]     = MakePose({ -6, 40, -26, 82,    -32, 97,   -30, 58,     -36, 30,     -6, 62,      10, 48,     -20, 20,    -22, 0,      36, 22,      88, 6 });
        poses[P_AirKnee]   = MakePose({ 0, 90,    4, 140,     8, 156,  -10, 118,     10, 128,     20, 120,     10, 132,     10, 56,    -14, 44,      40, 100,     24, 72 });
        poses[P_AirPunch]  = MakePose({ 0, 90,   12, 138,    18, 153,   -4, 118,      6, 130,     36, 118,     60, 104,      8, 56,    -14, 46,      30, 66,      14, 44 });
        poses[P_AirKick]   = MakePose({ 0, 88,  -10, 136,   -14, 152,  -26, 118,    -34, 136,      4, 118,     16, 130,     -4, 58,    -20, 36,      38, 74,      84, 62 });
        poses[P_Hit]       = MakePose({ -6, 90, -20, 136,   -30, 148,  -34, 120,    -48, 104,     -4, 112,     10, 96,     -20, 46,    -30, 0,      16, 48,      22, 0 });
        poses[P_HitCrouch] = MakePose({ -2, 56,  -2, 100,    -8, 114,  -16, 82,     -26, 66,     10, 78,      24, 66,     -14, 18,    -48, 0,      40, 40,      28, 0 });
        poses[P_Block]     = MakePose({ -2, 92,   2, 140,     4, 156,   18, 112,     22, 140,     24, 116,     28, 146,    -16, 48,    -30, 0,      20, 50,      22, 0 });
        poses[P_BlockCrouch] = MakePose({ -2, 58, 12, 102,   16, 117,   28, 80,      30, 104,     34, 84,      36, 110,    -14, 18,    -48, 0,      42, 40,      30, 0 });
        poses[P_AirHit]    = MakePose({ 0, 70,  -36, 96,    -50, 104,  -30, 120,    -20, 140,    -50, 80,     -62, 66,      30, 86,     56, 96,      30, 62,      54, 52 });
        poses[P_Down]      = MakePose({ 0, 14,  -50, 16,    -66, 18,   -60, 6,      -40, 2,      -34, 28,     -18, 36,      20, 10,     50, 2,       24, 26,      52, 6 });
        poses[P_GrabReach] = MakePose({ 4, 92,   16, 140,    20, 156,   30, 120,     52, 112,     36, 130,     56, 126,    -12, 46,    -32, 0,      30, 48,      36, 0 });
        poses[P_ThrowPull] = MakePose({ -6, 90, -18, 138,   -24, 154,  -30, 124,    -40, 150,    -20, 132,    -30, 160,    -20, 46,    -36, 0,      16, 50,      20, 0 });
        poses[P_Win]       = MakePose({ 0, 94,    2, 144,     4, 160,  -20, 118,     -4, 100,     14, 160,     18, 188,    -10, 48,    -18, 0,      16, 48,      20, 0 });
        poses[P_Lose]      = MakePose({ -4, 48,   4, 92,     10, 104,  -10, 66,      -4, 44,     16, 68,      22, 46,     -18, 10,    -50, 0,      24, 40,      22, 0 });
        poses[P_ChargeBack] = MakePose({ -4, 88, -6, 138,    -2, 154,  -30, 116,    -40, 128,    -18, 112,    -34, 120,    -18, 46,    -32, 0,      22, 48,      28, 0 });
        poses[P_PalmThrust] = MakePose({ 8, 88,  24, 134,    30, 149,   40, 118,     66, 114,     42, 128,     70, 130,    -12, 46,    -34, 0,      32, 48,      40, 0 });
        poses[P_RiseKick]  = MakePose({ 0, 96,  -10, 144,   -14, 160,  -30, 124,    -40, 108,     10, 130,     22, 148,     -4, 52,    -10, 8,      20, 138,     30, 186 });
        poses[P_Uppercut]  = MakePose({ 4, 96,   14, 146,    18, 162,  -12, 120,     -2, 104,     26, 170,     30, 200,    -12, 50,    -24, 4,      22, 54,      26, 2 });
        poses[P_GroundPound] = MakePose({ 0, 60, 22, 98,     30, 110,   10, 76,      20, 60,      44, 52,      56, 6,      -12, 20,    -46, 0,      42, 44,      30, 0 });
        posesReady = true;
    }

    // 色を混ぜて DxLib の色にする
    unsigned ToColor(const Rgb& c, const Rgb& tint, float tintAlpha) {
        const Rgb m = tintAlpha > 0.0f ? MixRgb(c, tint, tintAlpha) : c;
        return GetColor(m.r, m.g, m.b);
    }

    void Segment(const Vec2& a, const Vec2& b, float w, unsigned color) {
        DrawLineAA(a.x, a.y, b.x, b.y, color, w);
        DrawCircleAA(a.x, a.y, w * 0.5f, 16, color, TRUE);
        DrawCircleAA(b.x, b.y, w * 0.5f, 16, color, TRUE);
    }
}

const Pose& GetPose(PoseId id) {
    if (!posesReady) {
        BuildPoses();
    }
    return poses[id];
}

Pose BlendPose(const Pose& a, const Pose& b, float t) {
    Pose p;
    for (int i = 0; i < J_Count; ++i) {
        p.j[i] = LerpVec(a.j[i], b.j[i], t);
    }
    return p;
}

void DrawBody(const Pose& pose, const BodyStyle& st, float footX, float footY, int facing, int anim,
              const Rgb& tint, float tintAlpha) {
    const float s = st.scale;
    const float f = static_cast<float>(facing);
    Vec2 p[J_Count];
    for (int i = 0; i < J_Count; ++i) {
        p[i].x = footX + pose.j[i].x * s * f;
        p[i].y = footY - pose.j[i].y * s;
    }
    const Vec2 shoulder = LerpVec(p[J_Neck], p[J_Hip], 0.1f);
    const float limb = st.limb * s;
    const float leg = st.limb * 1.2f * s;
    const float torso = st.torso * s;
    const float headR = st.headR * s;

    const Rgb dark{ 0, 0, 0 };
    auto C = [&](const Rgb& c) { return ToColor(c, tint, tintAlpha); };
    auto D = [&](const Rgb& c) { return ToColor(MixRgb(c, dark, 0.3f), tint, tintAlpha); };

    const bool kazane = st.look == Look::Kazane;
    const Rgb sleeve = kazane ? st.top : st.skin;

    // 奥の腕
    Segment(shoulder, p[J_BElbow], limb, D(sleeve));
    Segment(p[J_BElbow], p[J_BHand], limb * 0.9f, D(st.skin));
    if (!kazane) {
        Segment(LerpVec(p[J_BElbow], p[J_BHand], 0.45f), LerpVec(p[J_BElbow], p[J_BHand], 0.85f), limb * 1.15f, D(st.accent));
    }
    DrawCircleAA(p[J_BHand].x, p[J_BHand].y, limb * 0.75f, 16, D(st.glove), TRUE);

    // 奥の脚
    Segment(p[J_Hip], p[J_BKnee], leg, D(st.pants));
    Segment(p[J_BKnee], p[J_BFoot], leg * 0.9f, D(st.pants));
    Segment(p[J_BFoot], Vec2{ p[J_BFoot].x + 9.0f * s * f, p[J_BFoot].y }, leg * 0.6f, D(st.hair));

    // 胴
    Segment(p[J_Hip], LerpVec(p[J_Hip], p[J_Neck], 0.92f), torso, C(st.top));
    if (kazane) {
        // 道着の襟
        const Vec2 collarTop = LerpVec(p[J_Hip], p[J_Neck], 0.92f);
        const Vec2 collarMid = LerpVec(p[J_Hip], p[J_Neck], 0.55f);
        DrawLineAA(collarTop.x - 6.0f * s * f, collarTop.y, collarMid.x + 3.0f * s * f, collarMid.y, C(ScaleRgb(st.top, 0.7f)), 2.5f * s);
    } else {
        // 胸元（上着の前が開いている）
        const Vec2 chestTop = LerpVec(p[J_Hip], p[J_Neck], 0.9f);
        const Vec2 chestBottom = LerpVec(p[J_Hip], p[J_Neck], 0.45f);
        Segment(Vec2{ chestTop.x + torso * 0.18f * f, chestTop.y }, Vec2{ chestBottom.x + torso * 0.12f * f, chestBottom.y },
                torso * 0.3f, C(st.skin));
    }
    // 帯
    {
        const Vec2 beltPos = LerpVec(p[J_Hip], p[J_Neck], 0.12f);
        DrawLineAA(LerpVec(p[J_Hip], p[J_Neck], 0.05f).x, LerpVec(p[J_Hip], p[J_Neck], 0.05f).y,
                   LerpVec(p[J_Hip], p[J_Neck], 0.2f).x, LerpVec(p[J_Hip], p[J_Neck], 0.2f).y, C(st.belt), torso * 1.04f);
        if (kazane) {
            const float sway = std::sin(anim * 0.15f) * 3.0f * s;
            Segment(beltPos, Vec2{ beltPos.x - 14.0f * s * f, beltPos.y + 16.0f * s + sway }, 4.0f * s, C(st.belt));
        }
    }

    // 手前の脚
    Segment(p[J_Hip], p[J_FKnee], leg, C(st.pants));
    Segment(p[J_FKnee], p[J_FFoot], leg * 0.9f, C(st.pants));
    Segment(p[J_FFoot], Vec2{ p[J_FFoot].x + 9.0f * s * f, p[J_FFoot].y }, leg * 0.6f, C(st.hair));

    // 首と頭
    Segment(LerpVec(p[J_Hip], p[J_Neck], 0.85f), p[J_Head], limb * 0.9f, C(st.skin));
    const Vec2 h = p[J_Head];
    if (kazane) {
        // 後ろで結んだ髪
        const float sway = std::sin(anim * 0.12f) * 4.0f * s;
        const Vec2 tieRoot{ h.x - headR * 0.8f * f, h.y - headR * 0.2f };
        Segment(tieRoot, Vec2{ tieRoot.x - 14.0f * s * f, tieRoot.y + 18.0f * s + sway }, 7.0f * s, C(st.hair));
        DrawCircleAA(h.x, h.y, headR, 24, C(st.skin), TRUE);
        // 髪（頭の上と後ろ）
        DrawCircleAA(h.x - headR * 0.25f * f, h.y - headR * 0.3f, headR * 0.92f, 24, C(st.hair), TRUE);
        DrawCircleAA(h.x + headR * 0.3f * f, h.y + headR * 0.15f, headR * 0.7f, 20, C(st.skin), TRUE);
        // 鉢巻き
        Segment(Vec2{ h.x - headR * 0.9f * f, h.y - headR * 0.35f }, Vec2{ h.x + headR * 0.85f * f, h.y - headR * 0.45f },
                4.0f * s, C(st.accent));
        const float tail = std::sin(anim * 0.2f) * 5.0f * s;
        const Vec2 knot{ h.x - headR * 0.95f * f, h.y - headR * 0.35f };
        Segment(knot, Vec2{ knot.x - 18.0f * s * f, knot.y + 4.0f * s + tail }, 3.5f * s, C(st.accent));
        Segment(knot, Vec2{ knot.x - 14.0f * s * f, knot.y + 10.0f * s - tail * 0.6f }, 3.0f * s, C(st.accent));
    } else {
        DrawCircleAA(h.x, h.y, headR, 24, C(st.skin), TRUE);
        // ひげ（顔の下側）
        DrawCircleAA(h.x + headR * 0.25f * f, h.y + headR * 0.45f, headR * 0.62f, 20, C(st.hair), TRUE);
        DrawCircleAA(h.x + headR * 0.3f * f, h.y + headR * 0.1f, headR * 0.45f, 16, C(st.skin), TRUE);
        // 眉
        DrawLineAA(h.x + headR * 0.15f * f, h.y - headR * 0.3f, h.x + headR * 0.85f * f, h.y - headR * 0.2f, C(st.hair), 3.0f * s);
    }
    // 目
    DrawCircleAA(h.x + headR * 0.55f * f, h.y - headR * 0.08f, 1.8f * s, 8, C(Rgb{ 20, 20, 20 }), TRUE);

    // 手前の腕
    Segment(shoulder, p[J_FElbow], limb, C(sleeve));
    Segment(p[J_FElbow], p[J_FHand], limb * 0.9f, C(st.skin));
    if (!kazane) {
        Segment(LerpVec(p[J_FElbow], p[J_FHand], 0.45f), LerpVec(p[J_FElbow], p[J_FHand], 0.85f), limb * 1.15f, C(st.accent));
    }
    DrawCircleAA(p[J_FHand].x, p[J_FHand].y, limb * 0.8f, 16, C(st.glove), TRUE);
}
