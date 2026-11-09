#include "pch.h"
#include "CpuBrain.h"
#include "Battle.h"
#include "Config.h"
#include "Fighter.h"

namespace {
    constexpr int ObservationSize = 64;
    const int kMotions[4][6] = {
        { 2, 3, 6, 0, 0, 0 },  // ↓↘→
        { 6, 2, 3, 0, 0, 0 },  // →↓↘
        { 2, 1, 4, 0, 0, 0 },  // ↓↙←
        { 2, 3, 6, 2, 3, 6 },  // ↓↘→↓↘→
    };
    const int kMotionLength[4] = { 3, 3, 3, 6 };

    unsigned Bit(Btn b) {
        return 1u << b;
    }
}

void CpuBrain::Reset(int level, unsigned seed) {
    rng_.seed(seed);
    switch (Clampi(level, 0, 2)) {
    case 0:
        reaction_ = 24;
        blockRate_ = 0.2f;
        antiAirRate_ = 0.1f;
        comboRate_ = 0.2f;
        punishRate_ = 0.15f;
        techRate_ = 0.05f;
        break;
    case 1:
        reaction_ = 15;
        blockRate_ = 0.55f;
        antiAirRate_ = 0.45f;
        comboRate_ = 0.65f;
        punishRate_ = 0.5f;
        techRate_ = 0.3f;
        break;
    default:
        reaction_ = 8;
        blockRate_ = 0.85f;
        antiAirRate_ = 0.8f;
        comboRate_ = 0.95f;
        punishRate_ = 0.85f;
        techRate_ = 0.6f;
        break;
    }
    level_ = Clampi(level, 0, 2);
    queue_.clear();
    observations_.assign(ObservationSize, Observation());
    obsHead_ = 0;
    obsCount_ = 0;
    cooldown_ = 30;
    holdDir_ = 5;
    holdFrames_ = 0;
    blockDecision_ = 0;
    antiAirTried_ = false;
    punishTried_ = false;
    techTried_ = false;
    comboTried_ = false;
    lastMove_ = nullptr;
    lastMoveFrame_ = 0;
}

bool CpuBrain::Roll(float probability) {
    std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rng_) < probability;
}

int CpuBrain::RandomInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(rng_);
}

void CpuBrain::Observe(const Fighter& opp) {
    Observation o;
    if (opp.IsAttacking()) {
        const MoveDef& m = *opp.move;
        const bool beforeEnd = opp.moveFrame < m.startup + m.active;
        o.attacking = beforeEnd && m.level != LvThrow;
        o.recovering = !beforeEnd && !opp.moveHit;
        o.guard = m.guard;
        o.reach = m.hit.Valid() ? m.hit.x1 * opp.Scale() + 30.0f : 0.0f;
        if (m.untilLanding && !beforeEnd) {
            o.recovering = true;
        }
    }
    if (opp.state == FState::Land && opp.landLag > 6) {
        o.recovering = true;
    }
    o.airborne = opp.IsAirborne() && (opp.state == FState::Air || opp.state == FState::Attack);
    o.knockedDown = opp.state == FState::Knockdown || opp.state == FState::WakeUp || opp.state == FState::AirHit;
    o.x = opp.x;
    o.vx = opp.vx;
    obsHead_ = (obsHead_ + 1) % ObservationSize;
    observations_[obsHead_] = o;
    if (obsCount_ < ObservationSize) {
        ++obsCount_;
    }
}

const CpuBrain::Observation& CpuBrain::Seen() const {
    const int ago = std::min(reaction_, obsCount_ - 1);
    const int index = ((obsHead_ - ago) % ObservationSize + ObservationSize) % ObservationSize;
    return observations_[index];
}

void CpuBrain::Queue(int dir, unsigned buttons, int frames) {
    for (int i = 0; i < frames; ++i) {
        Step s;
        s.dir = dir;
        s.buttons = buttons;
        queue_.push_back(s);
    }
}

void CpuBrain::QueueMotion(const int* dirs, int count, Btn button) {
    for (int i = 0; i < count; ++i) {
        Queue(dirs[i], i == count - 1 ? Bit(button) : 0u);
    }
    Queue(5, 0u);
}

void CpuBrain::QueueSpecial(int kind, Btn button) {
    QueueMotion(kMotions[kind], kMotionLength[kind], button);
}

Pad CpuBrain::ToPad(const Step& s, int facing) const {
    Pad p;
    const int col = (s.dir - 1) % 3;  // 0：後ろ、1：中央、2：前
    const int row = (s.dir - 1) / 3;  // 0：下、1：中央、2：上
    const bool forward = col == 2;
    const bool back = col == 0;
    p.down = row == 0;
    p.up = row == 2;
    if (facing >= 0) {
        p.right = forward;
        p.left = back;
    } else {
        p.left = forward;
        p.right = back;
    }
    for (int i = 0; i < BtnCount; ++i) {
        p.btn[i] = (s.buttons & (1u << i)) != 0;
    }
    return p;
}

bool CpuBrain::QueueCombo(const Fighter& me, const Battle& battle) {
    if (me.move == nullptr) {
        return false;
    }
    const MoveDef& m = *me.move;
    const bool kazane = me.def->body.look == Look::Kazane;
    const bool full = me.meter >= Config::MeterMax;
    switch (m.level) {
    case LvLight:
        Queue(me.crouching ? 2 : 5, Bit(BtnM));
        Queue(5, 0u);
        return true;
    case LvMedium:
        if (Roll(0.45f)) {
            Queue(5, Bit(BtnH));
            Queue(5, 0u);
        } else if (kazane) {
            if (battle.HasProjectile(me.side)) {
                QueueSpecial(2, BtnM);
            } else {
                QueueSpecial(0, BtnH);
            }
        } else {
            QueueSpecial(0, BtnH);
        }
        return true;
    case LvHeavy:
        if (m.hitKind != HitKind::Normal) {
            return false;
        }
        if (kazane) {
            QueueSpecial(Roll(0.5f) ? 1 : 2, BtnH);
        } else {
            QueueSpecial(Roll(0.5f) ? 0 : 1, BtnM);
        }
        return true;
    case LvSpecial:
        if (full && !m.projectile) {
            QueueSpecial(3, BtnH);
            return true;
        }
        return false;
    default:
        return false;
    }
}

void CpuBrain::Neutral(const Fighter& me, const Fighter& opp, const Battle& battle, float dist) {
    const bool kazane = me.def->body.look == Look::Kazane;
    const bool hasProj = battle.HasProjectile(me.side);
    const Observation& s = Seen();
    cooldown_ = RandomInt(4, 14) + (level_ == 0 ? 14 : 0);
    const Btn strengths[3] = { BtnL, BtnM, BtnH };
    const Btn strength = strengths[RandomInt(0, 2)];

    auto hold = [this](int dir, int frames) {
        holdDir_ = dir;
        holdFrames_ = frames;
    };

    if (s.knockedDown) {
        if (dist > 150.0f) {
            hold(6, 10);
        } else {
            hold(5, 8);
        }
        return;
    }
    if (me.meter >= Config::MeterMax && dist < (kazane ? 230.0f : 115.0f) && Roll(0.2f)) {
        QueueSpecial(3, BtnH);
        return;
    }
    const float pokeReach = me.def->moves[M_2M].hit.x1 * me.Scale() + opp.HalfWidth() - 6.0f;
    if (dist > 420.0f) {
        if (kazane && !hasProj && Roll(0.45f)) {
            QueueSpecial(0, strength);
        } else if (Roll(0.25f)) {
            Queue(6, 0u);
            Queue(5, 0u);
            Queue(6, 0u, 3);
        } else if (Roll(kazane ? 0.5f : 0.8f)) {
            hold(6, RandomInt(15, 35));
        } else {
            hold(5, RandomInt(10, 20));
        }
    } else if (dist > 200.0f) {
        if (kazane && !hasProj && Roll(0.3f)) {
            QueueSpecial(0, strength);
        } else if (!kazane && !hasProj && dist < 360.0f && Roll(0.22f)) {
            QueueSpecial(2, dist > 280.0f ? BtnH : BtnM);
        } else if (dist < 320.0f && Roll(0.16f)) {
            Queue(9, 0u, 2);
        } else if (Roll(0.45f)) {
            hold(6, RandomInt(10, 25));
        } else if (Roll(0.35f)) {
            hold(4, RandomInt(8, 20));
        } else {
            hold(5, RandomInt(6, 14));
        }
    } else {
        if (dist < 100.0f && Roll(0.22f)) {
            Queue(Roll(0.3f) ? 4 : 5, Bit(BtnL) | Bit(BtnM));
            Queue(5, 0u);
        } else if (dist < pokeReach && Roll(0.38f)) {
            Queue(2, Bit(BtnM));
            Queue(2, 0u);
        } else if (dist < 115.0f && Roll(0.3f)) {
            Queue(Roll(0.5f) ? 2 : 5, Bit(BtnL));
            Queue(5, 0u);
        } else if (!kazane && Roll(0.15f)) {
            QueueSpecial(0, BtnM);
        } else if (Roll(0.15f)) {
            Queue(4, 0u);
            Queue(5, 0u);
            Queue(4, 0u, 3);
        } else if (Roll(0.35f)) {
            hold(6, RandomInt(6, 12));
        } else {
            hold(1, RandomInt(10, 20));
        }
    }
}

Pad CpuBrain::Think(const Fighter& me, const Fighter& opp, const Battle& battle) {
    Observe(opp);
    const Observation& s = Seen();
    const float dist = std::fabs(opp.x - me.x);

    // 投げられたら、一定の確率で投げ抜けを試みる
    if (me.state == FState::Thrown) {
        if (!techTried_) {
            techTried_ = true;
            queue_.clear();
            if (Roll(techRate_)) {
                Queue(5, Bit(BtnL) | Bit(BtnM), 2);
            }
        }
    } else {
        techTried_ = false;
    }

    // 技が変わったら、コンボの判断をやり直す
    if (me.move != lastMove_ || me.moveFrame < lastMoveFrame_) {
        comboTried_ = false;
    }
    lastMove_ = me.move;
    lastMoveFrame_ = me.moveFrame;

    if (me.InCancelWindow() && me.moveHit && !comboTried_) {
        comboTried_ = true;
        if (Roll(comboRate_)) {
            queue_.clear();
            QueueCombo(me, battle);
        }
    }

    if (!queue_.empty()) {
        const Step st = queue_.front();
        queue_.pop_front();
        return ToPad(st, me.facing);
    }

    Step out;
    // ガード硬直中はガードを続ける
    if (me.state == FState::BlockStun) {
        out.dir = s.guard == Guard::Low ? 1 : 4;
        return ToPad(out, me.facing);
    }

    // ジャンプ中：降りながら近づいたら攻撃
    if (me.state == FState::Air) {
        if (!me.airAttackUsed && me.vy < 2.0f && dist < 150.0f && Roll(0.5f)) {
            out.buttons = Bit(Roll(0.6f) ? BtnH : BtnM);
        }
        return ToPad(out, me.facing);
    }

    if (!me.IsActionable()) {
        holdFrames_ = 0;
        return ToPad(out, me.facing);
    }

    // 対空
    if (s.airborne && dist < 240.0f) {
        if (!antiAirTried_) {
            antiAirTried_ = true;
            if (Roll(antiAirRate_)) {
                QueueSpecial(1, BtnH);
                const Step st = queue_.front();
                queue_.pop_front();
                return ToPad(st, me.facing);
            }
        }
    } else if (!s.airborne) {
        antiAirTried_ = false;
    }

    // ガード
    bool threat = s.attacking && std::fabs(s.x - me.x) < s.reach + 60.0f;
    bool lowThreat = threat && s.guard == Guard::Low;
    for (const Projectile& p : battle.Projectiles()) {
        if (p.owner == me.side) {
            continue;
        }
        const float approach = (me.x - p.x) * (p.vx >= 0.0f ? 1.0f : -1.0f);
        if (approach > 0.0f && approach < 140.0f + std::fabs(p.vx) * (reaction_ * 0.5f)) {
            threat = true;
            if (p.move->guard == Guard::Low) {
                lowThreat = true;
            }
        }
    }
    if (threat) {
        if (blockDecision_ == 0) {
            blockDecision_ = Roll(blockRate_) ? 1 : -1;
        }
        if (blockDecision_ == 1) {
            out.dir = (lowThreat && !s.airborne) ? 1 : 4;
            return ToPad(out, me.facing);
        }
    } else {
        blockDecision_ = 0;
    }

    // 空振りへの反撃
    if (s.recovering && dist < 210.0f) {
        if (!punishTried_) {
            punishTried_ = true;
            if (Roll(punishRate_)) {
                queue_.clear();
                if (me.meter >= Config::MeterMax && dist < 150.0f) {
                    QueueSpecial(3, BtnH);
                } else if (dist < 120.0f) {
                    Queue(5, Bit(BtnM));
                    Queue(5, 0u);
                } else {
                    Queue(2, Bit(BtnM));
                    Queue(2, 0u);
                }
                const Step st = queue_.front();
                queue_.pop_front();
                return ToPad(st, me.facing);
            }
        }
    } else {
        punishTried_ = false;
    }

    if (holdFrames_ > 0) {
        --holdFrames_;
        out.dir = holdDir_;
        return ToPad(out, me.facing);
    }
    if (cooldown_ > 0) {
        --cooldown_;
        return ToPad(out, me.facing);
    }
    Neutral(me, opp, battle, dist);
    if (!queue_.empty()) {
        const Step st = queue_.front();
        queue_.pop_front();
        return ToPad(st, me.facing);
    }
    if (holdFrames_ > 0) {
        --holdFrames_;
        out.dir = holdDir_;
    }
    return ToPad(out, me.facing);
}
