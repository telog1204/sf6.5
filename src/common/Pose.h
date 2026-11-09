#pragma once
#include "MathUtil.h"

// キャラの体は、関節の位置（ポーズ）を線と円でつないで描く。
// ポーズの座標は足元を原点とし、x は体の前が正、y は上が正。身長はおよそ170（キャラごとの拡大率を掛ける）

enum Joint {
    J_Hip,     // 腰
    J_Neck,    // 首の付け根
    J_Head,    // 頭の中心
    J_BElbow,  // 奥の腕のひじ
    J_BHand,   // 奥の手
    J_FElbow,  // 手前の腕のひじ
    J_FHand,   // 手前の手
    J_BKnee,   // 奥の脚のひざ
    J_BFoot,   // 奥の足
    J_FKnee,   // 手前の脚のひざ
    J_FFoot,   // 手前の足
    J_Count
};

enum PoseId {
    P_Stand,
    P_Stand2,
    P_WalkA,
    P_WalkB,
    P_Crouch,
    P_PreJump,
    P_JumpUp,
    P_JumpFall,
    P_DashF,
    P_DashB,
    P_Jab,
    P_Straight,
    P_HighKick,
    P_RaiseArms,
    P_Hammer,
    P_LowKick,
    P_LowKick2,
    P_Sweep,
    P_AirKnee,
    P_AirPunch,
    P_AirKick,
    P_Hit,
    P_HitCrouch,
    P_Block,
    P_BlockCrouch,
    P_AirHit,
    P_Down,
    P_GrabReach,
    P_ThrowPull,
    P_Win,
    P_Lose,
    P_ChargeBack,
    P_PalmThrust,
    P_RiseKick,
    P_Uppercut,
    P_GroundPound,
    P_Count
};

struct Pose {
    Vec2 j[J_Count];
};

const Pose& GetPose(PoseId id);
Pose BlendPose(const Pose& a, const Pose& b, float t);

// 見た目の違い（体格・色・髪型）
enum class Look {
    Kazane,   // 鉢巻きと後ろで結んだ髪、道着
    Gantetsu  // 坊主頭とひげ、袖のない上着、手甲
};

struct BodyStyle {
    Look look = Look::Kazane;
    float scale = 1.0f;   // 拡大率
    float limb = 10.0f;   // 手足の太さ
    float torso = 22.0f;  // 胴の太さ
    float headR = 14.0f;  // 頭の半径
    Rgb skin;
    Rgb top;     // 上着
    Rgb pants;   // ズボン
    Rgb belt;    // 帯
    Rgb hair;
    Rgb accent;  // 鉢巻き・手甲など
    Rgb glove;   // 拳
};

// 体を描く。footX, footY は足元の画面座標、facing は向き（1：右向き、-1：左向き）。
// anim は揺れもの（鉢巻きの端など）を動かすためのフレーム数。tintAlpha（0〜1）が 0 より大きいと tint の色を混ぜる
void DrawBody(const Pose& pose, const BodyStyle& style, float footX, float footY, int facing, int anim,
              const Rgb& tint = Rgb(), float tintAlpha = 0.0f);
