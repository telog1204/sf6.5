#pragma once
#include "MathUtil.h"
#include "Pose.h"

// 技とキャラのデータ。数値の単位は、時間がフレーム（1/60秒）、距離がポーズの座標（キャラの拡大率を掛ける前）

enum class Guard {
    Mid,          // 立ちでもしゃがみでもガードできる
    Low,          // しゃがみガードのみ（下段）
    Overhead,     // 立ちガードのみ（ジャンプ攻撃）
    Unblockable   // ガードできない
};

enum class HitKind {
    Normal,     // のけぞり
    Knockdown,  // ダウンさせる
    Launch      // 浮かせてダウンさせる
};

enum MoveLevel {
    LvLight,
    LvMedium,
    LvHeavy,
    LvSpecial,
    LvSuper,
    LvThrow
};

// 技の置き場所。L/M/H は弱・中・強、Qcf は↓↘→、Dp は→↓↘、Qcb は↓↙←
enum MoveSlot {
    M_5L, M_5M, M_5H,
    M_2L, M_2M, M_2H,
    M_JL, M_JM, M_JH,
    M_Throw,
    M_QcfL, M_QcfM, M_QcfH,
    M_DpL, M_DpM, M_DpH,
    M_QcbL, M_QcbM, M_QcbH,
    M_Super,
    M_Count
};

struct MoveDef {
    const char* name = "";
    MoveLevel level = LvLight;

    // 発生前・攻撃判定が出ている間・戻り（フレーム）
    int startup = 4;
    int active = 2;
    int recovery = 8;

    Box hit;                 // 攻撃判定（無い技は幅 0）
    int damage = 0;
    int chip = 0;            // ガードさせたときに削る体力
    int hitstun = 12;        // 当てたときの相手ののけぞり
    int blockstun = 9;       // ガードさせたときの相手の硬直
    int hitstop = 7;         // 当たった瞬間に止まる長さ
    Guard guard = Guard::Mid;
    HitKind hitKind = HitKind::Normal;
    HitKind lastHitKind = HitKind::Normal;  // 多段技の最後の1発
    float pushHit = 5.0f;    // 当てたとき相手を押す速さ
    float pushBlock = 6.0f;  // ガードさせたとき相手を押す速さ
    float launchVx = 3.0f;
    float launchVy = 9.0f;
    int meterGain = 20;      // 当てたときに増える超必殺技ゲージ
    float minScale = 0.3f;   // コンボの補正の下限

    bool crouching = false;  // しゃがみ技
    bool air = false;        // ジャンプ技

    // 前進（moveFrom〜moveTo フレームの間、毎フレーム moveSpeed だけ進む）
    float moveSpeed = 0.0f;
    int moveFrom = 0;
    int moveTo = -1;
    bool stopOnHit = false;  // 当たったら前進をやめる

    // 上昇（riseFrame に飛び上がる。untilLanding なら着地で技が終わる）
    int riseFrame = -1;
    float riseVx = 0.0f;
    float riseVy = 0.0f;
    bool untilLanding = false;
    int landingLag = 3;

    int invulnFrom = -1;     // 無敵（攻撃を受けない）のフレーム範囲
    int invulnTo = -1;
    int armorFrom = -1;      // アーマー（1回だけ攻撃を受けても止まらない）のフレーム範囲
    int armorTo = -1;

    int hits = 1;            // 多段技の回数
    int hitInterval = 1;     // 多段技の間隔

    // 飛び道具
    bool projectile = false;
    bool groundWave = false; // 地面を走る衝撃波
    float projX = 0.0f;
    float projY = 0.0f;
    float projSpeed = 0.0f;
    int projLife = 0;
    float projW = 0.0f;
    float projH = 0.0f;

    // 投げ
    bool isThrow = false;
    bool superGrab = false;  // 超必殺投げ
    float throwRange = 0.0f; // 体どうしの隙間がこれ以下なら投げられる

    // 見た目：出始め・攻撃中・戻りのポーズ
    PoseId pose0 = P_Stand;
    PoseId pose1 = P_Stand;
    PoseId pose2 = P_Stand;
};

struct CharacterDef {
    const char* name = "";
    const char* title = "";       // 二つ名
    const char* style = "";       // 戦い方の説明
    const char* winQuote = "";    // 勝利のセリフ
    int maxHp = 1000;
    float width = 50.0f;          // 体の幅（押し合い・やられ判定）
    float height = 170.0f;        // 身長
    float walkF = 4.0f;           // 前歩きの速さ
    float walkB = 3.2f;           // 後ろ歩きの速さ
    float jumpVy = 15.0f;
    float jumpVx = 4.5f;
    float gravity = 0.8f;
    float dashSpeed = 12.0f;
    int dashFrames = 16;
    float backDashSpeed = 11.0f;
    int backDashFrames = 18;
    int statPower = 3;            // 選択画面の目安（1〜5）
    int statSpeed = 3;
    int statRange = 3;
    BodyStyle body;
    Rgb effect;                   // 飛び道具・超必殺技の色
    int stage = 0;                // ホームステージ
    MoveDef moves[M_Count];

    // コマンド表に出す技（コマンドの書き方と名前）
    struct CommandText {
        const char* command;
        const char* name;
    };
    CommandText commands[6];
};

int CharacterCount();
const CharacterDef& GetCharacter(int index);
