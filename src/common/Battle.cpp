#include "pch.h"
#include "Battle.h"
#include <DxLib.h>
#include "Config.h"
#include "Sound.h"

namespace {
    constexpr int IntroFrames = 110;
    constexpr int SuperFreezeFrames = 45;
    constexpr int TrainingRefillFrames = 40;

    Box Intersect(const Box& a, const Box& b) {
        Box r;
        r.x0 = std::max(a.x0, b.x0);
        r.y0 = std::max(a.y0, b.y0);
        r.x1 = std::min(a.x1, b.x1);
        r.y1 = std::min(a.y1, b.y1);
        return r;
    }

    float ComboScale(int hitsSoFar, float minScale) {
        if (hitsSoFar < 2) {
            return 1.0f;
        }
        return std::max(minScale, 1.0f - 0.1f * hitsSoFar);
    }
}

Box Projectile::Bounds() const {
    Box b;
    const float w = move != nullptr ? move->projW : 20.0f;
    const float h = move != nullptr ? move->projH : 20.0f;
    b.x0 = x - w;
    b.x1 = x + w;
    if (move != nullptr && move->groundWave) {
        b.y0 = 0.0f;
        b.y1 = h * 2.0f;
    } else {
        b.y0 = y - h;
        b.y1 = y + h;
    }
    return b;
}

void Battle::Start(const BattleSetup& setup) {
    setup_ = setup;
    for (int i = 0; i < 2; ++i) {
        f_[i].Setup(GetCharacter(setup.character[i]), i);
    }
    cpu_[0] = false;
    cpu_[1] = setup.mode == BattleMode::Cpu;
    const unsigned seed = static_cast<unsigned>(GetNowHiPerformanceCount() & 0x7fffffff);
    brain_[1].Reset(setup.mode == BattleMode::Training ? 1 : setup.cpuLevel, seed);
    round_ = 1;
    matchWinner_ = -1;
    frame_ = 0;
    StartRound();
}

void Battle::StartRound() {
    const float center = Config::StageWidth * 0.5f;
    f_[0].ResetRound(center - 150.0f, 1);
    f_[1].ResetRound(center + 150.0f, -1);
    projectiles_.clear();
    effects_.clear();
    timerFrames_ = Config::RoundSeconds * Config::LogicFps;
    roundWinner_ = -1;
    doubleKo_ = false;
    superFreeze_ = 0;
    superSide_ = -1;
    camX_ = center;
    for (int i = 0; i < 2; ++i) {
        comboShow_[i] = 0;
        comboTimer_[i] = 0;
    }
    if (setup_.mode == BattleMode::Training) {
        f_[0].meter = Config::MeterMax;
        f_[1].meter = Config::MeterMax;
        SetPhase(Phase::Fight);
    } else {
        SetPhase(Phase::Intro);
    }
}

void Battle::ResetTraining() {
    StartRound();
}

void Battle::SetPhase(Phase p) {
    phase_ = p;
    phaseFrame_ = 0;
}

bool Battle::IsFinished() const {
    return phase_ == Phase::MatchOver && phaseFrame_ >= 170;
}

void Battle::UpdatePhase() {
    switch (phase_) {
    case Phase::Intro:
        if (phaseFrame_ == 10 || phaseFrame_ == 66) {
            Sound::Play(Sound::RoundCall);
        }
        if (phaseFrame_ >= IntroFrames) {
            SetPhase(Phase::Fight);
        }
        break;
    case Phase::Fight:
        if (setup_.mode != BattleMode::Training && superFreeze_ == 0) {
            if (--timerFrames_ <= 0) {
                timerFrames_ = 0;
                SetPhase(Phase::TimeUp);
            }
        }
        break;
    case Phase::KO: {
        if (phaseFrame_ >= 110 && roundWinner_ >= 0) {
            Fighter& w = f_[roundWinner_];
            if (w.state != FState::Win && !w.IsAirborne() && w.state != FState::Throwing && w.state != FState::Thrown) {
                w.SetState(FState::Win);
                w.vx = 0.0f;
            }
        }
        if (phaseFrame_ >= 220) {
            EndRound(roundWinner_);
        }
        break;
    }
    case Phase::TimeUp:
        if (phaseFrame_ == 60) {
            const float r0 = static_cast<float>(f_[0].hp) / f_[0].def->maxHp;
            const float r1 = static_cast<float>(f_[1].hp) / f_[1].def->maxHp;
            if (std::fabs(r0 - r1) < 0.0001f) {
                roundWinner_ = -1;
            } else {
                roundWinner_ = r0 > r1 ? 0 : 1;
                if (!f_[roundWinner_].IsAirborne()) {
                    f_[roundWinner_].SetState(FState::Win);
                }
                Fighter& loser = f_[1 - roundWinner_];
                if (!loser.IsAirborne() && loser.state != FState::Knockdown) {
                    loser.SetState(FState::Lose);
                }
            }
        }
        if (phaseFrame_ >= 200) {
            EndRound(roundWinner_);
        }
        break;
    case Phase::MatchOver:
        break;
    }
}

void Battle::EndRound(int winner) {
    if (winner < 0) {
        ++f_[0].roundWins;
        ++f_[1].roundWins;
    } else {
        ++f_[winner].roundWins;
    }
    const bool p0 = f_[0].roundWins >= Config::RoundsToWin;
    const bool p1 = f_[1].roundWins >= Config::RoundsToWin;
    if (p0 || p1) {
        matchWinner_ = (p0 && p1) ? -1 : (p0 ? 0 : 1);
        SetPhase(Phase::MatchOver);
        if (matchWinner_ >= 0) {
            Fighter& w = f_[matchWinner_];
            if (w.state != FState::Win && !w.IsAirborne()) {
                w.SetState(FState::Win);
            }
        }
        return;
    }
    ++round_;
    StartRound();
}

void Battle::ComputeAiPads(Pad pads[2]) {
    for (int i = 0; i < 2; ++i) {
        if (cpu_[i]) {
            pads[i] = brain_[i].Think(f_[i], f_[1 - i], *this);
        }
    }
    if (setup_.mode != BattleMode::Training) {
        return;
    }
    // トレーニングの相手（2P 側）
    const Fighter& me = f_[1];
    const Fighter& opp = f_[0];
    Pad p;
    const bool backIsLeft = me.facing > 0;
    switch (dummyMode) {
    case DummyMode::Stand:
        break;
    case DummyMode::Crouch:
        p.down = true;
        break;
    case DummyMode::Guard:
        if (me.threatened || me.state == FState::BlockStun) {
            p.left = backIsLeft;
            p.right = !backIsLeft;
            bool low = opp.IsAttacking() && opp.move->guard == Guard::Low;
            for (const Projectile& pr : projectiles_) {
                if (pr.owner == 0 && pr.move->guard == Guard::Low) {
                    low = true;
                }
            }
            const bool overhead = opp.IsAirborne() || (opp.IsAttacking() && opp.move->guard == Guard::Overhead);
            p.down = low && !overhead;
        }
        break;
    case DummyMode::Cpu:
        p = brain_[1].Think(f_[1], f_[0], *this);
        break;
    case DummyMode::Count:
        break;
    }
    pads[1] = p;
}

void Battle::UpdateFacing() {
    for (int i = 0; i < 2; ++i) {
        Fighter& f = f_[i];
        const Fighter& o = f_[1 - i];
        const bool canTurn = f.IsActionable() || f.state == FState::Land || f.state == FState::PreJump ||
                             f.state == FState::WakeUp || f.state == FState::Knockdown;
        if (!canTurn || f.IsAirborne()) {
            continue;
        }
        const float dx = o.x - f.x;
        if (dx > 2.0f) {
            f.facing = 1;
        } else if (dx < -2.0f) {
            f.facing = -1;
        }
    }
}

void Battle::Step(const Pad& pad1, const Pad& pad2) {
    ++frame_;
    ++phaseFrame_;
    for (Effect& e : effects_) {
        ++e.age;
    }
    effects_.erase(std::remove_if(effects_.begin(), effects_.end(), [](const Effect& e) { return e.age >= e.life; }),
                   effects_.end());
    for (int i = 0; i < 2; ++i) {
        if (comboTimer_[i] > 0) {
            --comboTimer_[i];
        }
    }
    if (shakeFrames_ > 0) {
        --shakeFrames_;
    }

    UpdatePhase();

    Pad pads[2] = { pad1, pad2 };
    ComputeAiPads(pads);

    if (superFreeze_ > 0) {
        --superFreeze_;
        for (int i = 0; i < 2; ++i) {
            f_[i].RecordInput(pads[i], true);
        }
        return;
    }
    // K.O. の直後はスローモーション
    if (phase_ == Phase::KO && phaseFrame_ < 60 && (frame_ % 2) == 1) {
        return;
    }

    const bool controls = phase_ == Phase::Fight;
    UpdateFacing();

    // 相手の攻撃が近いか（後ろを入れたときにガードの構えをとる）
    for (int i = 0; i < 2; ++i) {
        const Fighter& o = f_[1 - i];
        bool threat = false;
        if (o.IsAttacking() && o.move->level != LvThrow && o.moveFrame < o.move->startup + o.move->active &&
            std::fabs(o.x - f_[i].x) < 320.0f) {
            threat = true;
        }
        for (const Projectile& p : projectiles_) {
            if (p.owner != i && std::fabs(p.x - f_[i].x) < 300.0f && (f_[i].x - p.x) * p.vx > 0.0f) {
                threat = true;
            }
        }
        f_[i].threatened = threat;
    }

    const float oldX[2] = { f_[0].x, f_[1].x };
    for (int i = 0; i < 2; ++i) {
        f_[i].RecordInput(pads[i], f_[i].hitstop > 0);
    }
    for (int i = 0; i < 2; ++i) {
        if (f_[i].hitstop > 0) {
            --f_[i].hitstop;
        } else {
            f_[i].Step(f_[1 - i], *this, controls);
        }
    }
    UpdateThrows();
    ResolvePush();
    ClampPositions(oldX);
    UpdateProjectiles();
    CheckHits();
    for (int i = 0; i < 2; ++i) {
        f_[i].UpdateRenderPose();
    }
    UpdateCamera();

    if (phase_ == Phase::Fight && setup_.mode != BattleMode::Training) {
        const bool k0 = f_[0].knockedOut;
        const bool k1 = f_[1].knockedOut;
        if (k0 || k1) {
            doubleKo_ = k0 && k1;
            roundWinner_ = doubleKo_ ? -1 : (k0 ? 1 : 0);
            SetPhase(Phase::KO);
            Sound::Play(Sound::KO);
        }
    }

    if (setup_.mode == BattleMode::Training) {
        for (int i = 0; i < 2; ++i) {
            Fighter& f = f_[i];
            if (superFreeze_ == 0 && !f.IsAttacking()) {
                f.meter = Config::MeterMax;
            }
            if (f.idleFrames > TrainingRefillFrames && f.hp < f.def->maxHp) {
                f.hp = f.def->maxHp;
                f.redHp = static_cast<float>(f.hp);
            }
        }
    }
}

void Battle::ResolvePush() {
    Fighter& a = f_[0];
    Fighter& b = f_[1];
    for (const Fighter* f : { &a, &b }) {
        if (f->state == FState::Throwing || f->state == FState::Thrown) {
            return;
        }
    }
    const Box pa = a.PushBox();
    const Box pb = b.PushBox();
    if (!pa.Overlaps(pb)) {
        return;
    }
    const float overlap = std::min(pa.x1, pb.x1) - std::max(pa.x0, pb.x0);
    if (overlap <= 0.0f) {
        return;
    }
    // 左にいる方を左へ、右にいる方を右へ半分ずつ押す
    bool aLeft = a.x < b.x;
    if (std::fabs(a.x - b.x) < 0.5f) {
        aLeft = a.facing > 0;
    }
    Fighter& left = aLeft ? a : b;
    Fighter& right = aLeft ? b : a;
    left.x -= overlap * 0.5f;
    right.x += overlap * 0.5f;
    // 画面端なら、もう片方が全部押される
    const float minLeft = left.HalfWidth();
    if (left.x < minLeft) {
        right.x += minLeft - left.x;
        left.x = minLeft;
    }
    const float maxRight = Config::StageWidth - right.HalfWidth();
    if (right.x > maxRight) {
        left.x -= right.x - maxRight;
        right.x = maxRight;
    }
}

void Battle::ClampPositions(const float oldX[2]) {
    // 2人が離れすぎないように（離れようとした側を止める）
    const int L = f_[0].x <= f_[1].x ? 0 : 1;
    const int R = 1 - L;
    const float dist = f_[R].x - f_[L].x;
    if (dist > Config::MaxFighterDistance) {
        const float excess = dist - Config::MaxFighterDistance;
        const float outL = std::max(0.0f, oldX[L] - f_[L].x);
        const float outR = std::max(0.0f, f_[R].x - oldX[R]);
        const float total = outL + outR;
        if (total > 0.0001f) {
            f_[L].x += excess * outL / total;
            f_[R].x -= excess * outR / total;
        } else {
            f_[L].x += excess * 0.5f;
            f_[R].x -= excess * 0.5f;
        }
    }
    // ステージの端
    for (int i = 0; i < 2; ++i) {
        Fighter& f = f_[i];
        const float lo = f.HalfWidth();
        const float hi = Config::StageWidth - f.HalfWidth();
        int wall = 0;
        if (f.x < lo) {
            f.x = lo;
            wall = -1;
        } else if (f.x > hi) {
            f.x = hi;
            wall = 1;
        }
        // 画面端で押される側が動けないときは、押し返しを攻撃した側に移す
        if (wall != 0 && f.pushVel * wall > 0.0f) {
            if (f.pushOwner >= 0) {
                Fighter& attacker = f_[f.pushOwner];
                if (!attacker.IsAirborne()) {
                    attacker.pushVel = -f.pushVel;
                    attacker.pushOwner = -1;
                }
            }
            f.pushVel = 0.0f;
            f.pushOwner = -1;
        }
    }
}

void Battle::SpawnProjectile(Fighter& owner, const MoveDef& m) {
    Projectile p;
    p.owner = owner.side;
    p.x = owner.x + owner.facing * m.projX * owner.Scale();
    p.y = m.groundWave ? 0.0f : m.projY * owner.Scale();
    p.vx = m.projSpeed * owner.facing;
    p.life = m.projLife;
    p.move = &m;
    projectiles_.push_back(p);
    if (m.groundWave) {
        Sound::Play(Sound::Shockwave);
        AddEffect(Effect::Dust, p.x, 0.0f, 1.6f, Rgb{ 170, 140, 110 }, 24);
        Shake(8, 4.0f);
    } else {
        Sound::Play(Sound::Projectile);
    }
}

bool Battle::HasProjectile(int side) const {
    for (const Projectile& p : projectiles_) {
        if (p.alive && p.owner == side) {
            return true;
        }
    }
    return false;
}

void Battle::OnSuperStart(Fighter& f) {
    superFreeze_ = SuperFreezeFrames;
    superSide_ = f.side;
    Sound::Play(Sound::SuperStart);
    AddEffect(Effect::SuperBurst, f.x, f.def->height * 0.55f * f.Scale(), 1.0f, f.def->effect, SuperFreezeFrames);
    AddPopup(f.side, f.def->moves[M_Super].name, f.def->effect);
}

void Battle::OnLand(Fighter& f, bool hard) {
    AddEffect(Effect::Dust, f.x, 0.0f, hard ? 1.4f : 0.7f, Rgb{ 180, 170, 150 }, hard ? 24 : 14);
    Sound::Play(hard ? Sound::HitMedium : Sound::Land);
    if (hard) {
        Shake(6, 3.0f);
    }
}

void Battle::UpdateProjectiles() {
    const float camLeft = camX_ - Config::ScreenWidth * 0.5f;
    for (Projectile& p : projectiles_) {
        if (!p.alive) {
            continue;
        }
        p.x += p.vx;
        ++p.age;
        if (p.age >= p.life || p.x < camLeft - 120.0f || p.x > camLeft + Config::ScreenWidth + 120.0f) {
            p.alive = false;
            if (p.move->groundWave) {
                AddEffect(Effect::Dust, p.x, 0.0f, 0.8f, Rgb{ 170, 140, 110 }, 16);
            }
        }
    }
    // 飛び道具どうしがぶつかったら両方消える
    for (size_t i = 0; i < projectiles_.size(); ++i) {
        for (size_t j = i + 1; j < projectiles_.size(); ++j) {
            Projectile& a = projectiles_[i];
            Projectile& b = projectiles_[j];
            if (a.alive && b.alive && a.owner != b.owner && a.Bounds().Overlaps(b.Bounds())) {
                a.alive = false;
                b.alive = false;
                const Box area = Intersect(a.Bounds(), b.Bounds());
                AddEffect(Effect::Clash, (area.x0 + area.x1) * 0.5f, (area.y0 + area.y1) * 0.5f, 1.2f, Rgb{ 255, 255, 255 }, 18);
                Sound::Play(Sound::Block);
            }
        }
    }
    // 相手に当たったか
    for (Projectile& p : projectiles_) {
        if (!p.alive) {
            continue;
        }
        Fighter& owner = f_[p.owner];
        Fighter& target = f_[1 - p.owner];
        Box hurts[2];
        const int n = target.Hurtboxes(hurts);
        const Box pb = p.Bounds();
        for (int k = 0; k < n; ++k) {
            if (pb.Overlaps(hurts[k])) {
                p.alive = false;
                ApplyHit(owner, target, *p.move, 0, Intersect(pb, hurts[k]), true, p.vx >= 0.0f ? 1.0f : -1.0f);
                break;
            }
        }
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(), [](const Projectile& p) { return !p.alive; }),
                       projectiles_.end());
}

void Battle::CheckHits() {
    struct Pending {
        int attacker;
        const MoveDef* move;
        int hitIndex;
        Box area;
    };
    Pending pending[2];
    int count = 0;
    for (int i = 0; i < 2; ++i) {
        Box hb;
        int idx = 0;
        if (!f_[i].Hitbox(hb, idx)) {
            continue;
        }
        Box hurts[2];
        const int n = f_[1 - i].Hurtboxes(hurts);
        for (int k = 0; k < n; ++k) {
            if (hb.Overlaps(hurts[k])) {
                pending[count].attacker = i;
                pending[count].move = f_[i].move;
                pending[count].hitIndex = idx;
                pending[count].area = Intersect(hb, hurts[k]);
                ++count;
                break;
            }
        }
    }
    // 相打ちに対応するため、両方の判定を先に調べてから当てる
    for (int k = 0; k < count; ++k) {
        Fighter& a = f_[pending[k].attacker];
        ApplyHit(a, f_[1 - pending[k].attacker], *pending[k].move, pending[k].hitIndex, pending[k].area, false,
                 static_cast<float>(a.facing));
    }
    for (int i = 0; i < 2; ++i) {
        if (f_[i].state == FState::Attack && f_[i].move != nullptr && f_[i].move->isThrow) {
            TryThrow(i);
        }
    }
}

void Battle::DealDamage(Fighter& attacker, Fighter& d, int damage, bool allowKo) {
    (void)attacker;
    if (d.redHp < d.hp) {
        d.redHp = static_cast<float>(d.hp);
    }
    d.hp -= damage;
    d.redDelay = 40;
    if (d.hp <= 0) {
        // トレーニングと、K.O. のあとに当たった攻撃では倒れない
        if (setup_.mode == BattleMode::Training || !allowKo || phase_ != Phase::Fight) {
            d.hp = 1;
        } else {
            d.hp = 0;
            Knockout(d, d.x >= attacker.x ? 1.0f : -1.0f);
        }
    }
}

void Battle::Knockout(Fighter& d, float dir) {
    d.knockedOut = true;
    d.SetState(FState::AirHit);
    d.vy = 10.0f;
    d.vx = 4.0f * dir;
    d.y = std::max(d.y, 1.0f);
    d.crouching = false;
    Shake(30, 10.0f);
}

void Battle::ApplyHit(Fighter& a, Fighter& d, const MoveDef& m, int hitIndex, const Box& area, bool projectile, float dir) {
    if (!projectile) {
        a.hitsDone = hitIndex + 1;
    }
    const float cx = (area.x0 + area.x1) * 0.5f;
    const float cy = (area.y0 + area.y1) * 0.5f;

    // ガード
    if (d.CanBlock(m.guard)) {
        d.SetState(FState::BlockStun);
        d.stun = m.blockstun;
        d.crouching = d.history.Dir() <= 3;
        d.guarding = true;
        d.hitstop = m.hitstop;
        if (!projectile) {
            a.hitstop = m.hitstop;
            a.moveConnected = true;
        }
        d.pushVel = m.pushBlock * dir;
        d.pushOwner = projectile ? -1 : a.side;
        a.AddMeter(m.meterGain / 2);
        d.AddMeter(m.meterGain / 4);
        if (m.chip > 0) {
            DealDamage(a, d, m.chip, true);
        }
        AddEffect(Effect::BlockSpark, cx, cy, m.level >= LvHeavy ? 1.3f : 1.0f, Rgb{ 140, 200, 255 }, 12);
        Sound::Play(Sound::Block);
        return;
    }

    // アーマー（1回だけ耐える）
    if (d.ArmorActive() && m.level != LvSuper) {
        --d.armorLeft;
        DealDamage(a, d, m.damage, true);
        d.hitstop = m.hitstop;
        if (!projectile) {
            a.hitstop = m.hitstop;
            a.moveConnected = true;
        }
        d.flash = 3;
        AddEffect(Effect::HitSpark, cx, cy, 1.0f, Rgb{ 255, 150, 80 }, 14);
        AddPopup(d.side, "アーマー", Rgb{ 255, 160, 80 });
        Sound::Play(Sound::HitHeavy);
        return;
    }

    const bool counter = d.state == FState::Attack && d.move != nullptr && d.move->level != LvThrow &&
                         d.moveFrame < d.move->startup + d.move->active;
    float scale = ComboScale(d.comboCount, m.minScale);
    if (counter) {
        scale *= 1.2f;
    }
    const int dmg = std::max(1, static_cast<int>(std::lround(m.damage * scale)));
    if (d.comboCount == 0) {
        d.comboDamage = 0;
    }
    ++d.comboCount;
    d.comboDamage += dmg;
    comboShow_[a.side] = d.comboCount;
    comboTimer_[a.side] = 80;
    lastComboDamage_[a.side] = d.comboDamage;

    const bool wasAir = d.IsAirborne();
    const HitKind kind = (m.hits > 1 && hitIndex == m.hits - 1) ? m.lastHitKind : m.hitKind;
    DealDamage(a, d, dmg, true);
    if (!d.knockedOut) {
        if (kind == HitKind::Launch || wasAir) {
            d.SetState(FState::AirHit);
            d.vy = kind == HitKind::Launch ? m.launchVy : 6.0f;
            d.vx = (kind == HitKind::Launch ? m.launchVx : 2.5f) * dir;
            d.y = std::max(d.y, 1.0f);
            d.crouching = false;
        } else if (kind == HitKind::Knockdown) {
            d.SetState(FState::AirHit);
            d.vy = 5.0f;
            d.vx = 3.0f * dir;
            d.y = std::max(d.y, 1.0f);
            d.crouching = false;
        } else {
            d.SetState(FState::HitStun);
            d.stun = m.hitstun + (counter ? 4 : 0);
            d.pushVel = m.pushHit * dir;
            d.pushOwner = projectile ? -1 : a.side;
        }
    }
    d.flash = 4;
    d.guarding = false;
    int stop = m.hitstop + (counter ? 4 : 0);
    if (d.knockedOut) {
        stop = 24;
    }
    d.hitstop = stop;
    if (!projectile) {
        a.hitstop = stop;
        a.moveConnected = true;
        a.moveHit = true;
    }
    a.AddMeter(m.meterGain);
    d.AddMeter(m.meterGain / 3);

    const bool heavy = m.level >= LvHeavy || counter;
    AddEffect(Effect::HitSpark, cx, cy, heavy ? 1.5f : (m.level == LvMedium ? 1.1f : 0.8f), Rgb{ 255, 230, 140 }, heavy ? 18 : 12);
    if (m.level == LvLight) {
        Sound::Play(Sound::HitLight);
    } else if (m.level == LvMedium) {
        Sound::Play(Sound::HitMedium);
    } else {
        Sound::Play(Sound::HitHeavy);
    }
    if (heavy) {
        Shake(8, 4.0f);
    }
    if (counter) {
        AddPopup(a.side, "カウンター", Rgb{ 255, 220, 80 });
    }
}

void Battle::TryThrow(int i) {
    Fighter& a = f_[i];
    Fighter& d = f_[1 - i];
    const MoveDef& m = *a.move;
    if (a.moveConnected || a.moveFrame < m.startup || a.moveFrame >= m.startup + m.active) {
        return;
    }
    const float gap = std::fabs(a.x - d.x) - a.HalfWidth() - d.HalfWidth();
    if (gap > m.throwRange * a.Scale()) {
        return;
    }
    if ((d.x - a.x) * a.facing < -5.0f) {
        return;
    }
    const bool ok = d.CanBeThrown() || (m.superGrab && d.state == FState::HitStun && !d.IsAirborne());
    if (!ok) {
        return;
    }
    const int dir = a.history.Dir();
    a.moveConnected = true;
    a.throwSuper = m.superGrab;
    a.backThrow = !m.superGrab && (dir == 1 || dir == 4 || dir == 7);
    a.SetState(FState::Throwing);
    // 投げの技の情報（ダメージ）は、投げている間も使うので取っておく
    a.moveSlot = static_cast<int>(&m - a.def->moves);
    d.SetState(FState::Thrown);
    d.vx = 0.0f;
    d.vy = 0.0f;
    d.pushVel = 0.0f;
    d.crouching = false;
    d.guarding = false;
    Sound::Play(Sound::Grab);
}

void Battle::UpdateThrows() {
    for (int i = 0; i < 2; ++i) {
        Fighter& a = f_[i];
        Fighter& d = f_[1 - i];
        if (a.state != FState::Throwing) {
            continue;
        }
        const int t = a.stateFrame;
        const MoveDef& m = a.def->moves[a.moveSlot >= 0 && a.moveSlot < M_Count ? a.moveSlot : M_Throw];
        const float front = a.HalfWidth() + d.HalfWidth() + 4.0f;

        if (!a.throwSuper) {
            const int hitFrame = 24;
            if (t < hitFrame && d.state == FState::Thrown) {
                if (t <= 8 && d.ThrowTechInput()) {
                    // 投げ抜け
                    a.SetState(FState::Idle);
                    d.SetState(FState::Idle);
                    a.pushVel = -9.0f * a.facing;
                    d.pushVel = 9.0f * a.facing;
                    a.pushOwner = -1;
                    d.pushOwner = -1;
                    d.y = 0.0f;
                    AddEffect(Effect::BlockSpark, (a.x + d.x) * 0.5f, 110.0f, 1.4f, Rgb{ 255, 255, 255 }, 14);
                    AddPopup(d.side, "投げ抜け", Rgb{ 200, 230, 255 });
                    Sound::Play(Sound::Block);
                    continue;
                }
                if (!a.backThrow || t < 10) {
                    d.x = a.x + a.facing * front;
                    d.y = 0.0f;
                } else {
                    const float u = (t - 10) / 14.0f;
                    d.x = a.x + a.facing * front * (1.0f - 2.0f * u);
                    d.y = std::sin(u * kPi) * 90.0f;
                }
            }
            if (t == hitFrame && d.state == FState::Thrown) {
                const float away = d.x >= a.x ? 1.0f : -1.0f;
                d.comboCount = 1;
                d.comboDamage = m.damage;
                DealDamage(a, d, m.damage, true);
                if (!d.knockedOut) {
                    d.SetState(FState::AirHit);
                    d.vy = 6.0f;
                    d.vx = 4.0f * away;
                    d.y = std::max(d.y, 1.0f);
                }
                d.flash = 4;
                d.hitstop = 10;
                a.hitstop = 10;
                a.AddMeter(m.meterGain);
                AddEffect(Effect::HitSpark, d.x, 60.0f, 1.4f, Rgb{ 255, 230, 140 }, 16);
                Sound::Play(Sound::HitHeavy);
                Shake(10, 6.0f);
            }
            if (t >= 34) {
                a.SetState(FState::Idle);
            }
        } else {
            const int slamFrame = 38;
            if (t < slamFrame && d.state == FState::Thrown) {
                d.x = a.x + a.facing * front;
                if (t < 30) {
                    d.y = 130.0f * (t / 30.0f);
                } else {
                    d.y = 130.0f * (1.0f - (t - 30) / 8.0f);
                }
            }
            if (t == slamFrame && d.state == FState::Thrown) {
                const float scale = std::max(m.minScale, ComboScale(d.comboCount, m.minScale));
                const int dmg = static_cast<int>(std::lround(m.damage * scale));
                if (d.comboCount == 0) {
                    d.comboDamage = 0;
                }
                ++d.comboCount;
                d.comboDamage += dmg;
                comboShow_[a.side] = d.comboCount;
                comboTimer_[a.side] = 80;
                lastComboDamage_[a.side] = d.comboDamage;
                d.y = 0.0f;
                DealDamage(a, d, dmg, true);
                if (!d.knockedOut) {
                    d.SetState(FState::AirHit);
                    d.vy = 7.0f;
                    d.vx = 2.0f * a.facing;
                    d.y = 1.0f;
                }
                d.flash = 4;
                d.hitstop = 16;
                a.hitstop = 16;
                AddEffect(Effect::HitSpark, d.x, 20.0f, 2.2f, Rgb{ 255, 200, 120 }, 24);
                AddEffect(Effect::Dust, d.x, 0.0f, 2.0f, Rgb{ 170, 140, 110 }, 30);
                Sound::Play(Sound::Shockwave);
                Sound::Play(Sound::HitHeavy);
                Shake(24, 10.0f);
            }
            if (t >= 66) {
                a.SetState(FState::Idle);
            }
        }
    }
}

void Battle::AddEffect(Effect::Type type, float x, float y, float size, const Rgb& color, int life, const char* text) {
    Effect e;
    e.type = type;
    e.x = x;
    e.y = y;
    e.size = size;
    e.color = color;
    e.life = life;
    e.text = text;
    effects_.push_back(e);
}

void Battle::AddPopup(int side, const char* text, const Rgb& color) {
    const Fighter& f = f_[side];
    AddEffect(Effect::Popup, f.x, f.def->height * f.Scale() + 30.0f, 1.0f, color, 50, text);
}

void Battle::Shake(int frames, float power) {
    shakeFrames_ = std::max(shakeFrames_, frames);
    shakePower_ = shakeFrames_ == frames ? power : std::max(shakePower_, power);
}

void Battle::UpdateCamera() {
    const float half = Config::ScreenWidth * 0.5f;
    const float mid = (f_[0].x + f_[1].x) * 0.5f;
    camX_ = Clampf(mid, half, Config::StageWidth - half);
}
