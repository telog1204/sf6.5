#pragma once
#include <deque>
#include <random>
#include <vector>
#include "CharacterData.h"
#include "Pad.h"

class Battle;
class Fighter;

// CPU の操作。人と同じく「方向＋ボタン」の入力を作ってキャラに渡す。
// 相手の動きは reaction フレーム遅れて見えるようにして、人らしい反応の遅れを出す
class CpuBrain {
public:
    void Reset(int level, unsigned seed);
    Pad Think(const Fighter& me, const Fighter& opp, const Battle& battle);

private:
    struct Observation {
        bool attacking = false;   // 攻撃の出始めか攻撃判定が出ている
        bool recovering = false;  // 技の戻り（反撃のチャンス）
        Guard guard = Guard::Mid;
        bool airborne = false;
        bool knockedDown = false;
        float x = 0.0f;
        float vx = 0.0f;
        float reach = 0.0f;       // 技の届く距離（相手の位置から）
    };

    // 入力の予定（キャラの向き基準の方向と、押すボタン）
    struct Step {
        int dir = 5;
        unsigned buttons = 0;
    };

    void Observe(const Fighter& opp);
    const Observation& Seen() const;
    bool Roll(float probability);
    int RandomInt(int lo, int hi);
    void Queue(int dir, unsigned buttons, int frames = 1);
    void QueueMotion(const int* dirs, int count, Btn button);
    void QueueSpecial(int kind, Btn button);  // 0：↓↘→、1：→↓↘、2：↓↙←、3：↓↘→↓↘→
    bool QueueCombo(const Fighter& me, const Battle& battle);
    void Neutral(const Fighter& me, const Fighter& opp, const Battle& battle, float dist);
    Pad ToPad(const Step& s, int facing) const;

    // 強さごとの性格
    int level_ = 1;
    int reaction_ = 15;
    float blockRate_ = 0.5f;
    float antiAirRate_ = 0.4f;
    float comboRate_ = 0.6f;
    float punishRate_ = 0.5f;
    float techRate_ = 0.3f;

    std::mt19937 rng_;
    std::deque<Step> queue_;
    std::vector<Observation> observations_;
    int obsHead_ = 0;
    int obsCount_ = 0;
    int cooldown_ = 0;
    int holdDir_ = 5;
    int holdFrames_ = 0;
    int blockDecision_ = 0;   // 0：まだ決めていない、1：ガードする、-1：しない
    bool antiAirTried_ = false;
    bool punishTried_ = false;
    bool techTried_ = false;
    bool comboTried_ = false;
    const MoveDef* lastMove_ = nullptr;
    int lastMoveFrame_ = 0;
};
