#pragma once
#include <string>
#include <vector>
#include "CpuBrain.h"
#include "Fighter.h"
#include "Pad.h"

enum class BattleMode {
    Cpu,       // 1P 対 CPU
    Versus,    // 1P 対 2P
    Training   // トレーニング（相手はダミー）
};

struct BattleSetup {
    BattleMode mode = BattleMode::Cpu;
    int character[2] = { 0, 1 };
    int cpuLevel = 1;  // 0：よわい、1：ふつう、2：つよい
    int stage = 0;
};

// トレーニングの相手の動き
enum class DummyMode {
    Stand,     // 立ったまま
    Crouch,    // しゃがんだまま
    Guard,     // すべてガード
    Cpu,       // CPU（ふつう）
    Count
};

struct Projectile {
    int owner = 0;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    int age = 0;
    int life = 0;
    const MoveDef* move = nullptr;
    bool alive = true;

    Box Bounds() const;
};

// 画面の演出（火花・土煙・文字）
struct Effect {
    enum Type { HitSpark, BlockSpark, Dust, Clash, Popup, SuperBurst };
    Type type = HitSpark;
    float x = 0.0f;
    float y = 0.0f;
    int age = 0;
    int life = 20;
    float size = 1.0f;
    Rgb color;
    std::string text;
};

// 1試合（数ラウンド）を進めて描く
class Battle {
public:
    void Start(const BattleSetup& setup);

    // 1フレーム進める。pads はプレイヤーの入力（CPU・ダミーの側の入力は使わない）
    void Step(const Pad& pad1, const Pad& pad2);
    void Draw() const;

    bool IsFinished() const;
    int Winner() const { return matchWinner_; }  // 0：1P、1：2P、-1：引き分け
    const BattleSetup& Setup() const { return setup_; }

    // Fighter から呼ぶ
    void SpawnProjectile(Fighter& owner, const MoveDef& m);
    bool HasProjectile(int side) const;
    void OnSuperStart(Fighter& f);
    void OnLand(Fighter& f, bool hard);

    // CPU の判断に使う
    const Fighter& GetFighter(int i) const { return f_[i]; }
    const std::vector<Projectile>& Projectiles() const { return projectiles_; }

    // トレーニング
    DummyMode dummyMode = DummyMode::Stand;
    bool showBoxes = false;
    void ResetTraining();

private:
    enum class Phase { Intro, Fight, KO, TimeUp, MatchOver };

    void StartRound();
    void SetPhase(Phase p);
    void UpdatePhase();
    void ComputeAiPads(Pad pads[2]);
    void UpdateFacing();
    void ResolvePush();
    void ClampPositions(const float oldX[2]);
    void UpdateProjectiles();
    void CheckHits();
    void TryThrow(int attacker);
    void UpdateThrows();
    void ApplyHit(Fighter& a, Fighter& d, const MoveDef& m, int hitIndex, const Box& area, bool projectile, float dir);
    void DealDamage(Fighter& attacker, Fighter& d, int damage, bool allowKo);
    void Knockout(Fighter& d, float dir);
    void AddEffect(Effect::Type type, float x, float y, float size, const Rgb& color, int life, const char* text = "");
    void AddPopup(int side, const char* text, const Rgb& color);
    void Shake(int frames, float power);
    void UpdateCamera();
    void EndRound(int winner);

    void DrawProjectile(const Projectile& p, float camLeft) const;
    void DrawEffects(float camLeft) const;
    void DrawHud() const;
    void DrawBoxes(float camLeft) const;

    BattleSetup setup_;
    Fighter f_[2];
    CpuBrain brain_[2];
    bool cpu_[2] = { false, false };
    std::vector<Projectile> projectiles_;
    std::vector<Effect> effects_;

    Phase phase_ = Phase::Intro;
    int phaseFrame_ = 0;
    int frame_ = 0;
    int round_ = 1;
    int timerFrames_ = 0;
    int roundWinner_ = -1;
    int matchWinner_ = -1;
    bool doubleKo_ = false;
    int superFreeze_ = 0;
    int superSide_ = -1;
    float camX_ = 700.0f;
    int shakeFrames_ = 0;
    float shakePower_ = 0.0f;
    int comboShow_[2] = { 0, 0 };
    int comboTimer_[2] = { 0, 0 };
    int lastComboDamage_[2] = { 0, 0 };
};
