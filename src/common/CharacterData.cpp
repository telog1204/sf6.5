#include "pch.h"
#include "CharacterData.h"

namespace {
    Box B(float x0, float y0, float x1, float y1) {
        Box b;
        b.x0 = x0;
        b.y0 = y0;
        b.x1 = x1;
        b.y1 = y1;
        return b;
    }

    // 通常技の基本形
    MoveDef Normal(const char* name, MoveLevel lv, PoseId p0, PoseId p1, PoseId p2,
                   int startup, int active, int recovery, Box hit, int damage, int hitstun, int blockstun) {
        MoveDef m;
        m.name = name;
        m.level = lv;
        m.pose0 = p0;
        m.pose1 = p1;
        m.pose2 = p2;
        m.startup = startup;
        m.active = active;
        m.recovery = recovery;
        m.hit = hit;
        m.damage = damage;
        m.hitstun = hitstun;
        m.blockstun = blockstun;
        m.hitstop = lv == LvLight ? 6 : (lv == LvMedium ? 8 : 10);
        m.pushHit = lv == LvLight ? 4.5f : (lv == LvMedium ? 5.5f : 6.5f);
        m.pushBlock = m.pushHit + 1.5f;
        m.meterGain = lv == LvLight ? 20 : (lv == LvMedium ? 30 : 40);
        return m;
    }

    MoveDef Crouching(MoveDef m, bool low) {
        m.crouching = true;
        if (low) {
            m.guard = Guard::Low;
        }
        return m;
    }

    MoveDef Air(MoveDef m) {
        m.air = true;
        m.guard = Guard::Overhead;
        m.recovery = 0;
        return m;
    }

    MoveDef Throw(int damage, float range) {
        MoveDef m;
        m.name = "投げ";
        m.level = LvThrow;
        m.isThrow = true;
        m.startup = 4;
        m.active = 2;
        m.recovery = 22;
        m.damage = damage;
        m.throwRange = range;
        m.guard = Guard::Unblockable;
        m.pose0 = P_GrabReach;
        m.pose1 = P_GrabReach;
        m.pose2 = P_Stand;
        m.meterGain = 40;
        return m;
    }

    MoveDef Special(const char* name, PoseId p0, PoseId p1, PoseId p2, int startup, int active, int recovery,
                    Box hit, int damage, int chip, int hitstun, int blockstun) {
        MoveDef m;
        m.name = name;
        m.level = LvSpecial;
        m.pose0 = p0;
        m.pose1 = p1;
        m.pose2 = p2;
        m.startup = startup;
        m.active = active;
        m.recovery = recovery;
        m.hit = hit;
        m.damage = damage;
        m.chip = chip;
        m.hitstun = hitstun;
        m.blockstun = blockstun;
        m.hitstop = 10;
        m.pushHit = 6.0f;
        m.pushBlock = 8.0f;
        m.meterGain = 50;
        return m;
    }

    // ===== カザネ：風の拳士。飛び道具と対空技で間合いを支配するバランス型 =====
    CharacterDef MakeKazane() {
        CharacterDef c;
        c.name = "カザネ";
        c.title = "風を継ぐ拳士";
        c.style = "飛び道具「風刃」で牽制し、跳んできた相手を「昇旋脚」で落とす。基本に忠実なバランス型";
        c.winQuote = "風は止まらない。あなたの拳も、まだ伸びるはず。";
        c.maxHp = 1000;
        c.width = 50.0f;
        c.height = 170.0f;
        c.walkF = 4.2f;
        c.walkB = 3.4f;
        c.jumpVy = 15.5f;
        c.jumpVx = 4.6f;
        c.gravity = 0.82f;
        c.dashSpeed = 14.0f;
        c.dashFrames = 16;
        c.backDashSpeed = 12.0f;
        c.backDashFrames = 18;
        c.statPower = 3;
        c.statSpeed = 4;
        c.statRange = 4;
        c.stage = 0;
        c.effect = Rgb{ 120, 230, 255 };

        BodyStyle& b = c.body;
        b.look = Look::Kazane;
        b.scale = 1.0f;
        b.limb = 10.0f;
        b.torso = 22.0f;
        b.headR = 14.0f;
        b.skin = Rgb{ 240, 200, 165 };
        b.top = Rgb{ 70, 130, 205 };
        b.pants = Rgb{ 40, 60, 110 };
        b.belt = Rgb{ 245, 245, 240 };
        b.hair = Rgb{ 35, 30, 40 };
        b.accent = Rgb{ 225, 50, 60 };
        b.glove = Rgb{ 200, 60, 60 };

        MoveDef* m = c.moves;
        m[M_5L] = Normal("立ち弱", LvLight, P_Stand, P_Jab, P_Stand, 3, 2, 7, B(44, 122, 82, 150), 30, 12, 9);
        m[M_5M] = Normal("立ち中", LvMedium, P_ChargeBack, P_Straight, P_Stand, 5, 3, 12, B(48, 112, 92, 142), 55, 17, 13);
        m[M_5H] = Normal("立ち強", LvHeavy, P_Stand, P_HighKick, P_Stand, 8, 3, 19, B(46, 114, 102, 154), 85, 22, 16);
        m[M_2L] = Crouching(Normal("しゃがみ弱", LvLight, P_Crouch, P_LowKick, P_Crouch, 3, 2, 7, B(52, 2, 92, 28), 25, 12, 9), true);
        m[M_2M] = Crouching(Normal("しゃがみ中", LvMedium, P_Crouch, P_LowKick2, P_Crouch, 6, 3, 12, B(56, 0, 100, 26), 50, 17, 12), true);
        m[M_2H] = Crouching(Normal("足払い", LvHeavy, P_Crouch, P_Sweep, P_Crouch, 9, 3, 22, B(60, 0, 106, 22), 80, 20, 14), true);
        m[M_2H].hitKind = HitKind::Knockdown;
        m[M_JL] = Air(Normal("ジャンプ弱", LvLight, P_JumpUp, P_AirKnee, P_JumpFall, 3, 8, 0, B(22, 80, 58, 114), 40, 14, 10));
        m[M_JM] = Air(Normal("ジャンプ中", LvMedium, P_JumpUp, P_AirPunch, P_JumpFall, 5, 5, 0, B(38, 90, 76, 124), 55, 16, 12));
        m[M_JH] = Air(Normal("ジャンプ強", LvHeavy, P_JumpUp, P_AirKick, P_JumpFall, 7, 6, 0, B(52, 44, 102, 82), 80, 18, 14));
        m[M_Throw] = Throw(120, 40.0f);

        // 風刃（↓↘→＋攻撃）：飛び道具。強いほど速い
        for (int i = 0; i < 3; ++i) {
            MoveDef s = Special("風刃", P_ChargeBack, P_PalmThrust, P_Stand, 12, 2, 28 + i * 2, Box(), 60, 8, 20, 16);
            s.projectile = true;
            s.projX = 62.0f;
            s.projY = 118.0f;
            s.projSpeed = 5.5f + i * 2.25f;
            s.projLife = 200;
            s.projW = 26.0f;
            s.projH = 20.0f;
            s.hitstop = 8;
            s.pushHit = 5.5f;
            s.pushBlock = 7.0f;
            m[M_QcfL + i] = s;
        }

        // 昇旋脚（→↓↘＋攻撃）：出始め無敵の対空技。ガードされると大きな隙
        for (int i = 0; i < 3; ++i) {
            MoveDef s = Special("昇旋脚", P_Crouch, P_RiseKick, P_JumpFall, 2 + i, 10, 0, B(0, 80, 54, 205), 90 + i * 15, 12, 30, 18);
            s.riseFrame = s.startup;
            s.riseVy = 10.0f + i * 2.0f;
            s.riseVx = 2.0f + i * 0.5f;
            s.untilLanding = true;
            s.landingLag = 16;
            s.invulnFrom = 0;
            s.invulnTo = s.startup + 1 + i * 2;
            s.hitKind = HitKind::Launch;
            s.launchVy = 11.0f;
            s.launchVx = 3.0f;
            if (i == 2) {
                s.hits = 2;
                s.hitInterval = 5;
                s.damage = 70;
                s.hitKind = HitKind::Normal;
                s.lastHitKind = HitKind::Launch;
                s.hitstun = 30;
            }
            m[M_DpL + i] = s;
        }

        // 疾走掌（↓↙←＋攻撃）：踏み込んで両手で突く。強いほど遠くまで進む
        for (int i = 0; i < 3; ++i) {
            MoveDef s = Special("疾走掌", P_DashF, P_PalmThrust, P_Stand, 10 + i * 2, 4, 20, B(46, 96, 92, 140), 80 + i * 10, 10, 22, 12);
            s.moveSpeed = 9.0f + i * 2.0f;
            s.moveFrom = 1;
            s.moveTo = s.startup + 1;
            s.stopOnHit = true;
            s.pushBlock = 9.0f;
            if (i == 2) {
                s.hitKind = HitKind::Knockdown;
            }
            m[M_QcbL + i] = s;
        }

        // 超必殺技 嵐舞（↓↘→↓↘→＋攻撃）：突進して連打を浴びせる
        {
            MoveDef s = Special("嵐舞", P_DashF, P_Straight, P_HighKick, 6, 21, 26, B(20, 50, 100, 160), 45, 10, 30, 14);
            s.level = LvSuper;
            s.hits = 7;
            s.hitInterval = 3;
            s.lastHitKind = HitKind::Launch;
            s.launchVy = 12.0f;
            s.launchVx = 5.0f;
            s.moveSpeed = 13.0f;
            s.moveFrom = 1;
            s.moveTo = 22;
            s.stopOnHit = true;
            s.invulnFrom = 0;
            s.invulnTo = 12;
            s.minScale = 0.5f;
            s.hitstop = 5;
            s.meterGain = 0;
            s.pushHit = 2.0f;
            s.pushBlock = 3.0f;
            m[M_Super] = s;
        }

        c.commands[0] = { "↓↘→ ＋ 攻撃", "風刃（飛び道具）" };
        c.commands[1] = { "→↓↘ ＋ 攻撃", "昇旋脚（無敵対空）" };
        c.commands[2] = { "↓↙← ＋ 攻撃", "疾走掌（突進）" };
        c.commands[3] = { "↓↘→↓↘→ ＋ 攻撃", "超必殺 嵐舞（ゲージ満タン）" };
        c.commands[4] = { "近くで 弱＋中", "投げ（後ろを入れると逆に投げる）" };
        c.commands[5] = { "", "" };
        return c;
    }

    // ===== ガンテツ：岩をも砕く大男。アーマー技と超必殺投げで押し切るパワー型 =====
    CharacterDef MakeGantetsu() {
        CharacterDef c;
        c.name = "ガンテツ";
        c.title = "砕けぬ岩鉄";
        c.style = "動きは遅いが一撃が重い。攻撃に耐えて殴り返す「岩砕拳」と、超必殺投げ「剛岩崩」で押し切る";
        c.winQuote = "軽い、軽いぞ！ 鍛え直して出直してこい！";
        c.maxHp = 1150;
        c.width = 64.0f;
        c.height = 192.0f;
        c.walkF = 3.0f;
        c.walkB = 2.6f;
        c.jumpVy = 15.0f;
        c.jumpVx = 4.0f;
        c.gravity = 0.88f;
        c.dashSpeed = 11.0f;
        c.dashFrames = 18;
        c.backDashSpeed = 9.0f;
        c.backDashFrames = 20;
        c.statPower = 5;
        c.statSpeed = 2;
        c.statRange = 3;
        c.stage = 1;
        c.effect = Rgb{ 255, 160, 60 };

        BodyStyle& b = c.body;
        b.look = Look::Gantetsu;
        b.scale = 1.13f;
        b.limb = 14.0f;
        b.torso = 32.0f;
        b.headR = 15.0f;
        b.skin = Rgb{ 205, 150, 110 };
        b.top = Rgb{ 150, 75, 35 };
        b.pants = Rgb{ 55, 50, 45 };
        b.belt = Rgb{ 210, 170, 60 };
        b.hair = Rgb{ 45, 30, 25 };
        b.accent = Rgb{ 150, 155, 165 };
        b.glove = Rgb{ 120, 110, 100 };

        MoveDef* m = c.moves;
        m[M_5L] = Normal("立ち弱", LvLight, P_Stand, P_Jab, P_Stand, 4, 2, 8, B(44, 122, 82, 150), 35, 13, 9);
        m[M_5M] = Normal("立ち中", LvMedium, P_ChargeBack, P_Straight, P_Stand, 7, 3, 14, B(48, 110, 94, 144), 70, 19, 14);
        m[M_5H] = Normal("立ち強", LvHeavy, P_RaiseArms, P_Hammer, P_Stand, 11, 4, 20, B(44, 86, 96, 140), 110, 24, 18);
        m[M_5H].hitstop = 12;
        m[M_2L] = Crouching(Normal("しゃがみ弱", LvLight, P_Crouch, P_LowKick, P_Crouch, 4, 2, 8, B(52, 2, 92, 28), 30, 13, 9), true);
        m[M_2M] = Crouching(Normal("しゃがみ中", LvMedium, P_Crouch, P_LowKick2, P_Crouch, 8, 3, 14, B(56, 0, 100, 26), 60, 18, 13), true);
        m[M_2H] = Crouching(Normal("足払い", LvHeavy, P_Crouch, P_Sweep, P_Crouch, 12, 3, 24, B(60, 0, 106, 22), 100, 20, 14), true);
        m[M_2H].hitKind = HitKind::Knockdown;
        m[M_JL] = Air(Normal("ジャンプ弱", LvLight, P_JumpUp, P_AirKnee, P_JumpFall, 4, 7, 0, B(22, 80, 58, 114), 45, 14, 10));
        m[M_JM] = Air(Normal("ジャンプ中", LvMedium, P_JumpUp, P_AirPunch, P_JumpFall, 6, 5, 0, B(38, 88, 76, 124), 70, 16, 12));
        m[M_JH] = Air(Normal("ジャンプ強", LvHeavy, P_JumpUp, P_AirKick, P_JumpFall, 9, 5, 0, B(50, 42, 102, 82), 100, 18, 14));
        m[M_Throw] = Throw(140, 44.0f);

        // 岩砕拳（↓↘→＋攻撃）：踏み込みながら殴る。出がかりにアーマーがあり、1回だけ攻撃を受けても止まらない
        for (int i = 0; i < 3; ++i) {
            MoveDef s = Special("岩砕拳", P_ChargeBack, P_Straight, P_Stand, 16 + i * 2, 4, 22 + i * 2, B(46, 104, 100, 146), 110 + i * 15, 15, 24, 18);
            s.armorFrom = 3;
            s.armorTo = s.startup - 1;
            s.moveSpeed = 6.0f + i * 1.5f;
            s.moveFrom = 6;
            s.moveTo = s.startup + 2;
            s.stopOnHit = true;
            s.pushBlock = 11.0f;
            if (i == 2) {
                s.hitKind = HitKind::Knockdown;
            }
            m[M_QcfL + i] = s;
        }

        // 鉄槌（→↓↘＋攻撃）：真上に突き上げる対空。出始め無敵
        for (int i = 0; i < 3; ++i) {
            MoveDef s = Special("鉄槌", P_Crouch, P_Uppercut, P_Stand, 5 + i, 6, 26 + i * 2, B(0, 100, 64, 220), 100 + i * 15, 12, 26, 18);
            s.invulnFrom = 0;
            s.invulnTo = s.startup + 1;
            s.hitKind = HitKind::Launch;
            s.launchVy = 12.0f;
            s.launchVx = 2.5f;
            m[M_DpL + i] = s;
        }

        // 地裂波（↓↙←＋攻撃）：地面を叩いて衝撃波を走らせる。下段。強いほど遠くまで届く
        for (int i = 0; i < 3; ++i) {
            MoveDef s = Special("地裂波", P_RaiseArms, P_GroundPound, P_Crouch, 18, 2, 26, Box(), 80, 10, 22, 16);
            s.projectile = true;
            s.groundWave = true;
            s.guard = Guard::Low;
            s.projX = 60.0f;
            s.projY = 0.0f;
            s.projSpeed = 6.0f + i;
            s.projLife = 34 + i * 16;
            s.projW = 24.0f;
            s.projH = 22.0f;
            s.hitKind = HitKind::Knockdown;
            m[M_QcbL + i] = s;
        }

        // 超必殺技 剛岩崩（↓↘→↓↘→＋攻撃）：近くの相手をつかんで叩きつける。ガードできない
        {
            MoveDef s = Throw(380, 70.0f);
            s.name = "剛岩崩";
            s.level = LvSuper;
            s.superGrab = true;
            s.startup = 2;
            s.active = 4;
            s.recovery = 40;
            s.invulnFrom = 0;
            s.invulnTo = 8;
            s.minScale = 0.5f;
            s.meterGain = 0;
            m[M_Super] = s;
        }

        c.commands[0] = { "↓↘→ ＋ 攻撃", "岩砕拳（アーマー突進）" };
        c.commands[1] = { "→↓↘ ＋ 攻撃", "鉄槌（無敵対空）" };
        c.commands[2] = { "↓↙← ＋ 攻撃", "地裂波（下段の衝撃波）" };
        c.commands[3] = { "↓↘→↓↘→ ＋ 攻撃", "超必殺 剛岩崩（つかみ・ゲージ満タン）" };
        c.commands[4] = { "近くで 弱＋中", "投げ（後ろを入れると逆に投げる）" };
        c.commands[5] = { "", "" };
        return c;
    }

    CharacterDef characters[2];
    bool ready = false;

    void Build() {
        characters[0] = MakeKazane();
        characters[1] = MakeGantetsu();
        ready = true;
    }
}

int CharacterCount() {
    return 2;
}

const CharacterDef& GetCharacter(int index) {
    if (!ready) {
        Build();
    }
    return characters[Clampi(index, 0, CharacterCount() - 1)];
}
