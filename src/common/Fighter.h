#pragma once
#include "CharacterData.h"
#include "InputHistory.h"
#include "Pad.h"
#include "Pose.h"

class Battle;

enum class FState {
    Idle,       // 立ち
    Walk,       // 歩き
    Crouch,     // しゃがみ
    PreJump,    // ジャンプの踏み込み
    Air,        // ジャンプ中
    Land,       // 着地の硬直
    Dash,       // 前ダッシュ
    BackDash,   // バックステップ
    Attack,     // 技を出している
    HitStun,    // のけぞり
    BlockStun,  // ガード硬直
    AirHit,     // 吹き飛び（空中で攻撃を受けた・ダウンする攻撃を受けた）
    Knockdown,  // ダウン
    WakeUp,     // 起き上がり
    Throwing,   // 投げている
    Thrown,     // 投げられている
    Win,
    Lose
};

// 対戦するキャラ1人。判定や移動の計算はワールド座標（x は右が正、y は上が正、地面が 0）
class Fighter {
public:
    void Setup(const CharacterDef& def, int side);
    void ResetRound(float x, int facing);

    // 1フレームの処理（Battle が呼ぶ）
    void RecordInput(const Pad& pad, bool frozen);
    void Step(Fighter& opp, Battle& battle, bool controls);
    void UpdateRenderPose();

    // 判定（ワールド座標）
    int Hurtboxes(Box out[2]) const;
    bool Hitbox(Box& out, int& hitIndex) const;
    Box PushBox() const;
    Box ToWorld(const Box& local) const;

    bool IsInvulnerable() const;
    bool CanBeThrown() const;
    bool CanBlock(Guard guard) const;
    bool IsAirborne() const { return y > 0.0f; }
    bool IsActionable() const;
    bool IsAttacking() const { return state == FState::Attack && move != nullptr; }
    bool InCancelWindow() const;
    bool ArmorActive() const;
    bool ThrowTechInput() const;
    float Scale() const { return def->body.scale; }
    float HalfWidth() const { return def->width * 0.5f; }

    void AddMeter(int amount);
    void StartMove(int slot, Battle& battle);
    void SetState(FState s);
    void EnterKnockdown();

    void Draw(float cameraLeft, float groundY, float shakeX, float shakeY, int frame, bool superGlow) const;

    const CharacterDef* def = nullptr;
    int side = 0;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float pushVel = 0.0f;    // 攻撃を当てた・受けたときに押される速さ
    int pushOwner = -1;      // pushVel の原因になった相手（画面端で押し返しを相手に移すため）
    int facing = 1;
    int hp = 0;
    float redHp = 0.0f;      // 体力ゲージの赤い部分（減った体力を遅れて表示する）
    int redDelay = 0;
    int meter = 0;
    int roundWins = 0;

    FState state = FState::Idle;
    int stateFrame = 0;
    const MoveDef* move = nullptr;
    int moveSlot = -1;
    int moveFrame = 0;
    bool moveConnected = false;  // 当たったかガードされた（キャンセルできる）
    bool moveHit = false;        // ガードされずに当たった
    int hitsDone = 0;
    int armorLeft = 0;

    bool crouching = false;
    bool guarding = false;       // 後ろを入れてガードの構えをしている
    bool threatened = false;     // 相手の攻撃が近い（後ろ歩きの代わりにガードの構えをとる）
    bool airAttackUsed = false;
    int jumpDir = 0;
    int landLag = 0;
    int stun = 0;
    int hitstop = 0;
    int throwProtect = 0;        // のけぞりから戻った直後の投げられない時間
    int flash = 0;               // 攻撃を受けたときの白い光
    bool knockedOut = false;

    // 投げの演出
    bool throwSuper = false;
    bool backThrow = false;

    // コンボ（この人が受けている連続ヒット）
    int comboCount = 0;
    int comboDamage = 0;
    float comboScaleBonus = 0.0f;

    int idleFrames = 0;          // 操作できる状態が続いているフレーム数（トレーニングの体力回復に使う）

    InputHistory history;

private:
    void Neutral(Battle& battle);
    bool TrySpecial(Battle& battle, bool superOnly);
    bool TryCancel(Battle& battle);
    bool StrongestPressed(int frames, int& level) const;
    void StepAttack(Battle& battle, bool controls);
    bool ApplyAirPhysics();   // 着地したら true
    void EnterLand(int lag, Battle& battle);
    void EndMove();
    void EndStun();
    Pose TargetPose() const;

    Pose renderPose_;
    bool renderPoseReady_ = false;
};
