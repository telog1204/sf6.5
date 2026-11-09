#include "pch.h"
#include "Fighter.h"
#include <DxLib.h>
#include "Battle.h"
#include "Config.h"
#include "Sound.h"

namespace {
    constexpr int PreJumpFrames = 4;
    constexpr int JumpLandFrames = 3;
    constexpr int KnockdownFrames = 38;
    constexpr int WakeUpFrames = 14;
    constexpr int BackDashInvulnFrames = 8;
    constexpr int ButtonBuffer = 5;       // 先行入力を受け付けるフレーム数
    constexpr int CancelExtra = 8;        // 攻撃判定が消えたあともキャンセルできるフレーム数
    constexpr int ThrowProtectFrames = 6;

    const int kQcf[] = { 2, 3, 6 };
    const int kDp[] = { 6, 2, 3 };
    const int kQcb[] = { 2, 1, 4 };
    const int kSuper[] = { 2, 3, 6, 2, 3, 6 };
}

void Fighter::Setup(const CharacterDef& d, int s) {
    def = &d;
    side = s;
    roundWins = 0;
    meter = 0;
}

void Fighter::ResetRound(float px, int f) {
    x = px;
    y = 0.0f;
    vx = 0.0f;
    vy = 0.0f;
    pushVel = 0.0f;
    pushOwner = -1;
    facing = f;
    hp = def->maxHp;
    redHp = static_cast<float>(hp);
    redDelay = 0;
    state = FState::Idle;
    stateFrame = 0;
    move = nullptr;
    moveSlot = -1;
    moveFrame = 0;
    moveConnected = false;
    moveHit = false;
    hitsDone = 0;
    armorLeft = 0;
    crouching = false;
    guarding = false;
    threatened = false;
    airAttackUsed = false;
    jumpDir = 0;
    stun = 0;
    hitstop = 0;
    throwProtect = 0;
    flash = 0;
    knockedOut = false;
    comboCount = 0;
    comboDamage = 0;
    idleFrames = 0;
    history.Clear();
    renderPoseReady_ = false;
    UpdateRenderPose();
}

void Fighter::RecordInput(const Pad& pad, bool frozen) {
    history.Push(pad, facing, frozen);
}

void Fighter::SetState(FState s) {
    state = s;
    stateFrame = 0;
    if (s != FState::Attack) {
        move = nullptr;
        moveSlot = -1;
    }
}

void Fighter::AddMeter(int amount) {
    meter = Clampi(meter + amount, 0, Config::MeterMax);
}

bool Fighter::IsActionable() const {
    return state == FState::Idle || state == FState::Walk || state == FState::Crouch;
}

bool Fighter::IsInvulnerable() const {
    switch (state) {
    case FState::AirHit:
    case FState::Knockdown:
    case FState::WakeUp:
    case FState::Throwing:
    case FState::Thrown:
    case FState::Win:
    case FState::Lose:
        return true;
    case FState::BackDash:
        return stateFrame < BackDashInvulnFrames;
    case FState::Attack:
        return move != nullptr && move->invulnFrom >= 0 && moveFrame >= move->invulnFrom && moveFrame <= move->invulnTo;
    default:
        break;
    }
    return knockedOut;
}

bool Fighter::CanBeThrown() const {
    if (IsAirborne() || IsInvulnerable() || throwProtect > 0) {
        return false;
    }
    switch (state) {
    case FState::Idle:
    case FState::Walk:
    case FState::Crouch:
    case FState::PreJump:
    case FState::Land:
    case FState::Dash:
    case FState::BackDash:
    case FState::Attack:
        return true;
    default:
        return false;
    }
}

bool Fighter::CanBlock(Guard g) const {
    if (g == Guard::Unblockable || IsAirborne()) {
        return false;
    }
    if (!(state == FState::Idle || state == FState::Walk || state == FState::Crouch || state == FState::BlockStun)) {
        return false;
    }
    const int d = history.Dir();
    if (!(d == 1 || d == 4 || d == 7)) {
        return false;
    }
    const bool low = d == 1;
    if (g == Guard::Low && !low) {
        return false;
    }
    if (g == Guard::Overhead && low) {
        return false;
    }
    return true;
}

bool Fighter::ArmorActive() const {
    return state == FState::Attack && move != nullptr && armorLeft > 0 && move->armorFrom >= 0 &&
           moveFrame >= move->armorFrom && moveFrame <= move->armorTo;
}

bool Fighter::ThrowTechInput() const {
    return history.Pressed(BtnL, 3) && history.Pressed(BtnM, 3);
}

bool Fighter::InCancelWindow() const {
    return state == FState::Attack && move != nullptr && moveConnected && !move->air &&
           move->level <= LvSpecial && moveFrame <= move->startup + move->active + CancelExtra;
}

bool Fighter::StrongestPressed(int frames, int& level) const {
    if (history.Pressed(BtnH, frames)) {
        level = 2;
        return true;
    }
    if (history.Pressed(BtnM, frames)) {
        level = 1;
        return true;
    }
    if (history.Pressed(BtnL, frames)) {
        level = 0;
        return true;
    }
    return false;
}

void Fighter::StartMove(int slot, Battle& battle) {
    move = &def->moves[slot];
    moveSlot = slot;
    moveFrame = 0;
    moveConnected = false;
    moveHit = false;
    hitsDone = 0;
    state = FState::Attack;
    stateFrame = 0;
    guarding = false;
    crouching = move->crouching;
    if (!move->air) {
        vx = 0.0f;
    }
    armorLeft = move->armorFrom >= 0 ? 1 : 0;
    history.Consume();
    if (move->level == LvSuper) {
        meter -= Config::MeterMax;
        battle.OnSuperStart(*this);
    } else if (move->level == LvSpecial) {
        AddMeter(10);
    }
}

bool Fighter::TrySpecial(Battle& battle, bool superOnly) {
    int level = 0;
    const bool attack = StrongestPressed(ButtonBuffer, level);
    const bool shortcut = history.Pressed(BtnSP, ButtonBuffer);
    if (!attack && !shortcut) {
        return false;
    }
    const int d = history.Dir();
    if (meter >= Config::MeterMax) {
        if ((attack && history.Motion(kSuper, 6, 34)) || (shortcut && d <= 3)) {
            const MoveDef& s = def->moves[M_Super];
            if (!(s.projectile && battle.HasProjectile(side))) {
                StartMove(M_Super, battle);
                return true;
            }
        }
    }
    if (superOnly) {
        return false;
    }
    int slot = -1;
    if (attack) {
        bool dp = history.Motion(kDp, 3, 16);
        bool qcf = history.Motion(kQcf, 3, 16);
        const bool qcb = history.Motion(kQcb, 3, 16);
        if (dp && qcf) {
            // ↓↘→ と →↓↘ の両方に当てはまるときは、最後に前を入れていれば ↓↘→ とみなす
            if (d == 6) {
                dp = false;
            } else {
                qcf = false;
            }
        }
        if (dp) {
            slot = M_DpL + level;
        } else if (qcf) {
            slot = M_QcfL + level;
        } else if (qcb) {
            slot = M_QcbL + level;
        }
    }
    if (slot < 0 && shortcut) {
        // 必殺ショートカット：前＝→↓↘、後ろ＝↓↙←、中立＝↓↘→（強さは中）
        if (d == 6 || d == 9) {
            slot = M_DpM;
        } else if (d == 4 || d == 7) {
            slot = M_QcbM;
        } else if (d == 5 || d == 8) {
            slot = M_QcfM;
        }
    }
    if (slot < 0) {
        return false;
    }
    if (def->moves[slot].projectile && battle.HasProjectile(side)) {
        return false;
    }
    StartMove(slot, battle);
    return true;
}

bool Fighter::TryCancel(Battle& battle) {
    const MoveDef& m = *move;
    if (m.level == LvSpecial) {
        return TrySpecial(battle, true);
    }
    if (TrySpecial(battle, false)) {
        return true;
    }
    int level = 0;
    if (!StrongestPressed(ButtonBuffer, level)) {
        return false;
    }
    // チェーン：弱→弱、弱→中→強のように、より強い通常技へつなげる
    if (level > m.level || (m.level == LvLight && level == LvLight)) {
        const int d = history.Dir();
        StartMove((d <= 3 ? M_2L : M_5L) + level, battle);
        return true;
    }
    return false;
}

void Fighter::Neutral(Battle& battle) {
    const int d = history.Dir();
    guarding = false;
    if (TrySpecial(battle, false)) {
        return;
    }
    if (ThrowTechInput()) {
        StartMove(M_Throw, battle);
        return;
    }
    int level = 0;
    if (StrongestPressed(ButtonBuffer, level)) {
        StartMove((d <= 3 ? M_2L : M_5L) + level, battle);
        return;
    }
    if (d >= 7) {
        SetState(FState::PreJump);
        jumpDir = d == 7 ? -1 : (d == 9 ? 1 : 0);
        crouching = false;
        return;
    }
    if (history.DoubleTap(6)) {
        SetState(FState::Dash);
        history.Consume();
        crouching = false;
        return;
    }
    if (history.DoubleTap(4)) {
        SetState(FState::BackDash);
        history.Consume();
        crouching = false;
        return;
    }
    if (d <= 3) {
        if (state != FState::Crouch) {
            SetState(FState::Crouch);
        }
        crouching = true;
        guarding = d == 1 && threatened;
        vx = 0.0f;
        return;
    }
    crouching = false;
    if (d == 6) {
        if (state != FState::Walk) {
            SetState(FState::Walk);
        }
        vx = def->walkF * facing;
    } else if (d == 4) {
        if (threatened) {
            if (state != FState::Idle) {
                SetState(FState::Idle);
            }
            guarding = true;
            vx = 0.0f;
        } else {
            if (state != FState::Walk) {
                SetState(FState::Walk);
            }
            vx = -def->walkB * facing;
        }
    } else {
        if (state != FState::Idle) {
            SetState(FState::Idle);
        }
        vx = 0.0f;
    }
}

bool Fighter::ApplyAirPhysics() {
    vy -= def->gravity;
    y += vy;
    x += vx;
    if (y <= 0.0f) {
        y = 0.0f;
        vy = 0.0f;
        return true;
    }
    return false;
}

void Fighter::EnterLand(int lag, Battle& battle) {
    SetState(FState::Land);
    landLag = lag;
    vx = 0.0f;
    vy = 0.0f;
    y = 0.0f;
    crouching = false;
    battle.OnLand(*this, false);
}

void Fighter::EndMove() {
    const int d = history.Dir();
    SetState(d <= 3 ? FState::Crouch : FState::Idle);
    crouching = d <= 3;
    vx = 0.0f;
}

void Fighter::EndStun() {
    const int d = history.Dir();
    SetState(d <= 3 ? FState::Crouch : FState::Idle);
    crouching = d <= 3;
    comboCount = 0;
    throwProtect = ThrowProtectFrames;
}

void Fighter::EnterKnockdown() {
    SetState(FState::Knockdown);
    vx = 0.0f;
    vy = 0.0f;
    y = 0.0f;
    crouching = false;
    comboCount = 0;
}

void Fighter::Step(Fighter& opp, Battle& battle, bool controls) {
    (void)opp;
    ++stateFrame;
    if (throwProtect > 0) {
        --throwProtect;
    }
    if (flash > 0) {
        --flash;
    }

    switch (state) {
    case FState::Idle:
    case FState::Walk:
    case FState::Crouch:
        if (controls) {
            Neutral(battle);
        } else if (state != FState::Idle && state != FState::Crouch) {
            SetState(FState::Idle);
            vx = 0.0f;
        }
        if (state == FState::Walk) {
            x += vx;
        }
        break;
    case FState::PreJump:
        if (stateFrame >= PreJumpFrames) {
            SetState(FState::Air);
            vy = def->jumpVy;
            vx = jumpDir * def->jumpVx * facing;
            airAttackUsed = false;
            y = 0.01f;
        }
        break;
    case FState::Air:
        if (controls && !airAttackUsed) {
            int level = 0;
            if (StrongestPressed(ButtonBuffer, level)) {
                StartMove(M_JL + level, battle);
                airAttackUsed = true;
                StepAttack(battle, false);
                break;
            }
        }
        if (ApplyAirPhysics()) {
            EnterLand(JumpLandFrames, battle);
        }
        break;
    case FState::Land:
        if (stateFrame >= landLag) {
            SetState(FState::Idle);
            if (controls) {
                Neutral(battle);
            }
        }
        break;
    case FState::Dash: {
        const float t = static_cast<float>(stateFrame) / def->dashFrames;
        x += def->dashSpeed * (1.0f - t) * facing;
        if (stateFrame >= def->dashFrames) {
            SetState(FState::Idle);
        }
        break;
    }
    case FState::BackDash: {
        const float t = static_cast<float>(stateFrame) / def->backDashFrames;
        x -= def->backDashSpeed * (1.0f - t) * facing;
        if (stateFrame >= def->backDashFrames) {
            SetState(FState::Idle);
        }
        break;
    }
    case FState::Attack:
        StepAttack(battle, controls);
        break;
    case FState::HitStun:
        if (--stun <= 0) {
            EndStun();
        }
        break;
    case FState::BlockStun:
        crouching = history.Dir() <= 3;
        if (--stun <= 0) {
            EndStun();
        }
        break;
    case FState::AirHit:
        if (ApplyAirPhysics()) {
            EnterKnockdown();
            battle.OnLand(*this, true);
        }
        break;
    case FState::Knockdown:
        if (!knockedOut && stateFrame >= KnockdownFrames) {
            SetState(FState::WakeUp);
        }
        break;
    case FState::WakeUp:
        if (stateFrame >= WakeUpFrames) {
            SetState(FState::Idle);
            throwProtect = ThrowProtectFrames;
        }
        break;
    case FState::Throwing:
    case FState::Thrown:
    case FState::Win:
    case FState::Lose:
        break;
    }

    if (state != FState::Throwing && state != FState::Thrown) {
        x += pushVel;
    }
    pushVel *= 0.8f;
    if (std::fabs(pushVel) < 0.15f) {
        pushVel = 0.0f;
        pushOwner = -1;
    }

    if (IsActionable()) {
        ++idleFrames;
    } else {
        idleFrames = 0;
    }

    if (redHp > hp) {
        if (redDelay > 0) {
            --redDelay;
        } else {
            redHp = std::max(static_cast<float>(hp), redHp - def->maxHp * 0.006f);
        }
    } else {
        redHp = static_cast<float>(hp);
    }
}

void Fighter::StepAttack(Battle& battle, bool controls) {
    const MoveDef* m = move;
    if (m == nullptr) {
        SetState(FState::Idle);
        return;
    }
    if (controls && InCancelWindow()) {
        if (TryCancel(battle)) {
            return;
        }
    }
    // 弱攻撃の出始めに中攻撃が重なったら投げにする（弱＋中の同時押しの受付）
    if (controls && (moveSlot == M_5L || moveSlot == M_2L) && moveFrame <= 2 && ThrowTechInput()) {
        StartMove(M_Throw, battle);
        return;
    }

    ++moveFrame;
    if (m->moveSpeed != 0.0f && moveFrame >= m->moveFrom && moveFrame <= m->moveTo && !(m->stopOnHit && moveConnected)) {
        x += m->moveSpeed * facing;
    }
    if (moveFrame == m->riseFrame) {
        vy = m->riseVy;
        vx = m->riseVx * facing;
        y = std::max(y, 0.01f);
    }
    if (moveFrame == m->startup) {
        if (m->projectile) {
            battle.SpawnProjectile(*this, *m);
        } else if (!m->isThrow) {
            Sound::Play(Sound::Whiff);
        }
    }

    const bool airborne = y > 0.0f || vy > 0.0f;
    if (airborne) {
        if (ApplyAirPhysics()) {
            if (m->air || m->untilLanding) {
                EnterLand(m->air ? JumpLandFrames : m->landingLag, battle);
                return;
            }
        }
    }

    const int total = m->startup + m->active + m->recovery;
    if (m->air) {
        if (moveFrame >= total) {
            SetState(FState::Air);
            airAttackUsed = true;
        }
        return;
    }
    if (m->untilLanding) {
        if (!airborne && moveFrame >= total) {
            EndMove();
        }
        return;
    }
    if (moveFrame >= total) {
        EndMove();
    }
}

Box Fighter::ToWorld(const Box& local) const {
    const float s = Scale();
    Box b;
    if (facing >= 0) {
        b.x0 = x + local.x0 * s;
        b.x1 = x + local.x1 * s;
    } else {
        b.x0 = x - local.x1 * s;
        b.x1 = x - local.x0 * s;
    }
    b.y0 = y + local.y0 * s;
    b.y1 = y + local.y1 * s;
    return b;
}

int Fighter::Hurtboxes(Box out[2]) const {
    if (IsInvulnerable()) {
        return 0;
    }
    const float w = HalfWidth();
    const float h = def->height;
    Box body;
    body.x0 = x - w;
    body.x1 = x + w;
    if (IsAirborne()) {
        body.y0 = y + 20.0f;
        body.y1 = y + h * 0.85f;
    } else if (crouching) {
        body.y0 = 0.0f;
        body.y1 = h * 0.66f;
    } else {
        body.y0 = 0.0f;
        body.y1 = h;
    }
    out[0] = body;
    int count = 1;
    // 攻撃中は、手足を伸ばした部分にもやられ判定がある（空振りに反撃できるように）
    if (state == FState::Attack && move != nullptr && move->hit.Valid() && moveFrame >= move->startup - 1) {
        Box ext = ToWorld(move->hit);
        const float shrink = 8.0f;
        if (facing >= 0) {
            ext.x1 -= shrink;
        } else {
            ext.x0 += shrink;
        }
        if (ext.Valid()) {
            out[count++] = ext;
        }
    }
    return count;
}

bool Fighter::Hitbox(Box& out, int& hitIndex) const {
    if (state != FState::Attack || move == nullptr || !move->hit.Valid()) {
        return false;
    }
    const MoveDef& m = *move;
    if (moveFrame < m.startup || moveFrame >= m.startup + m.active) {
        return false;
    }
    const int idx = m.hits > 1 ? (moveFrame - m.startup) / std::max(1, m.hitInterval) : 0;
    if (idx >= m.hits || idx < hitsDone) {
        return false;
    }
    out = ToWorld(m.hit);
    hitIndex = idx;
    return true;
}

Box Fighter::PushBox() const {
    const float w = HalfWidth();
    const float h = def->height;
    Box b;
    b.x0 = x - w;
    b.x1 = x + w;
    if (IsAirborne()) {
        b.y0 = y + 70.0f;
        b.y1 = y + h * 0.8f;
    } else if (crouching) {
        b.y0 = 0.0f;
        b.y1 = h * 0.6f;
    } else {
        b.y0 = 0.0f;
        b.y1 = h * 0.75f;
    }
    return b;
}

Pose Fighter::TargetPose() const {
    switch (state) {
    case FState::Idle:
        if (guarding) {
            return GetPose(P_Block);
        }
        return BlendPose(GetPose(P_Stand), GetPose(P_Stand2), 0.5f + 0.5f * std::sin(stateFrame * 0.09f));
    case FState::Walk: {
        const float phase = stateFrame * 0.22f * (vx * facing >= 0.0f ? 1.0f : -1.0f);
        return BlendPose(GetPose(P_WalkA), GetPose(P_WalkB), 0.5f + 0.5f * std::sin(phase));
    }
    case FState::Crouch:
        return GetPose(guarding ? P_BlockCrouch : P_Crouch);
    case FState::PreJump:
    case FState::Land:
        return GetPose(P_PreJump);
    case FState::Air:
        if (vy > 4.0f) {
            return GetPose(P_JumpUp);
        }
        return BlendPose(GetPose(P_JumpUp), GetPose(P_JumpFall), Clampf((4.0f - vy) / 10.0f, 0.0f, 1.0f));
    case FState::Dash:
        return GetPose(P_DashF);
    case FState::BackDash:
        return GetPose(P_DashB);
    case FState::Attack: {
        if (move == nullptr) {
            return GetPose(P_Stand);
        }
        const MoveDef& m = *move;
        const Pose& p0 = GetPose(m.pose0);
        const Pose& p1 = GetPose(m.pose1);
        const Pose& p2 = GetPose(m.pose2);
        if (moveFrame < m.startup) {
            const float t = m.startup > 0 ? static_cast<float>(moveFrame) / m.startup : 1.0f;
            if (t < 0.6f) {
                return p0;
            }
            return BlendPose(p0, p1, (t - 0.6f) / 0.4f);
        }
        if (moveFrame < m.startup + m.active) {
            if (m.hits > 2) {
                // 連打の技は、2つのポーズを交互に見せる
                const int idx = (moveFrame - m.startup) / std::max(1, m.hitInterval);
                return (idx % 2 == 0) ? p1 : p2;
            }
            return p1;
        }
        if (m.untilLanding) {
            return BlendPose(p1, p2, Clampf((2.0f - vy) / 8.0f, 0.0f, 1.0f));
        }
        const float r = m.recovery > 0 ? static_cast<float>(moveFrame - m.startup - m.active) / m.recovery : 1.0f;
        if (m.hits > 2) {
            return BlendPose(p1, GetPose(P_Stand), Clampf(r * 1.5f, 0.0f, 1.0f));
        }
        return BlendPose(p1, p2, Clampf(r * 1.5f, 0.0f, 1.0f));
    }
    case FState::HitStun: {
        const Pose& hit = GetPose(crouching ? P_HitCrouch : P_Hit);
        const Pose& base = GetPose(crouching ? P_Crouch : P_Stand);
        return BlendPose(hit, base, Clampf(1.0f - stun / 8.0f, 0.0f, 1.0f));
    }
    case FState::BlockStun:
        return GetPose(crouching ? P_BlockCrouch : P_Block);
    case FState::AirHit:
        if (y < 30.0f && vy < 0.0f) {
            return BlendPose(GetPose(P_AirHit), GetPose(P_Down), Clampf((30.0f - y) / 30.0f, 0.0f, 1.0f));
        }
        return GetPose(P_AirHit);
    case FState::Knockdown:
        return GetPose(P_Down);
    case FState::WakeUp: {
        const float t = static_cast<float>(stateFrame) / WakeUpFrames;
        if (t < 0.5f) {
            return BlendPose(GetPose(P_Down), GetPose(P_Crouch), t * 2.0f);
        }
        return BlendPose(GetPose(P_Crouch), GetPose(P_Stand), (t - 0.5f) * 2.0f);
    }
    case FState::Throwing:
        if (throwSuper) {
            if (stateFrame < 30) {
                return GetPose(P_RaiseArms);
            }
            if (stateFrame < 50) {
                return GetPose(P_GroundPound);
            }
            return BlendPose(GetPose(P_GroundPound), GetPose(P_Stand), (stateFrame - 50) / 20.0f);
        }
        if (stateFrame < 10) {
            return GetPose(P_GrabReach);
        }
        return GetPose(P_ThrowPull);
    case FState::Thrown:
        return IsAirborne() ? GetPose(P_AirHit) : GetPose(P_Hit);
    case FState::Win:
        return BlendPose(GetPose(P_Stand), GetPose(P_Win), Clampf(stateFrame / 12.0f, 0.0f, 1.0f));
    case FState::Lose:
        return knockedOut ? GetPose(P_Down) : GetPose(P_Lose);
    }
    return GetPose(P_Stand);
}

void Fighter::UpdateRenderPose() {
    const Pose target = TargetPose();
    const bool snap = !renderPoseReady_ || (state == FState::Attack && move != nullptr && moveFrame >= move->startup) ||
                      state == FState::HitStun || state == FState::BlockStun;
    if (snap) {
        renderPose_ = target;
        renderPoseReady_ = true;
    } else {
        renderPose_ = BlendPose(renderPose_, target, 0.5f);
    }
}

void Fighter::Draw(float cameraLeft, float groundY, float shakeX, float shakeY, int frame, bool superGlow) const {
    float sx = x - cameraLeft + shakeX;
    const float sy = groundY - y + shakeY;
    if (hitstop > 0 && (state == FState::HitStun || state == FState::BlockStun || state == FState::AirHit)) {
        sx += (hitstop % 2 == 0) ? 3.0f : -3.0f;
    }
    Rgb tint;
    float tintAlpha = 0.0f;
    if (flash > 0) {
        tint = Rgb{ 255, 255, 255 };
        tintAlpha = 0.1f * flash;
    } else if (superGlow) {
        tint = def->effect;
        tintAlpha = 0.3f + 0.15f * std::sin(frame * 0.6f);
    } else if (ArmorActive()) {
        tint = Rgb{ 255, 120, 60 };
        tintAlpha = 0.35f;
    }
    DrawBody(renderPose_, def->body, sx, sy, facing, frame, tint, Clampf(tintAlpha, 0.0f, 0.6f));
}
