#include "pch.h"
#include "Scene.h"
#include <DxLib.h>
#include "Config.h"
#include "Sound.h"
#include "Stage.h"
#include "Text.h"
#include "Ui.h"

// タイトル・モード選択・操作説明・キャラクター選択・結果の画面

namespace {
    const unsigned kBlack = 0x000000u;

    unsigned White() {
        return GetColor(255, 255, 255);
    }

    unsigned Gold() {
        return GetColor(255, 210, 90);
    }

    unsigned Gray() {
        return GetColor(150, 150, 160);
    }

    Pose IdlePose(int frame) {
        return BlendPose(GetPose(P_Stand), GetPose(P_Stand2), 0.5f + 0.5f * std::sin(frame * 0.09f));
    }

    const char* LevelName(int level) {
        static const char* names[] = { "よわい", "ふつう", "つよい" };
        return names[Clampi(level, 0, 2)];
    }

    // 画面左上の「もどる」（タッチ用）
    constexpr int BackX0 = 12;
    constexpr int BackY0 = 12;
    constexpr int BackX1 = 120;
    constexpr int BackY1 = 52;

    void DrawBackButton() {
        Ui::Panel(BackX0, BackY0, BackX1, BackY1, 180);
        Text::Draw(Text::Medium, (BackX0 + BackX1) / 2, BackY0 + 8, "もどる", White(), kBlack, Text::AlignCenter);
    }

    bool BackTapped(const AppContext& app) {
        int x = 0;
        int y = 0;
        return app.input.Tapped(x, y) && Ui::Inside(x, y, BackX0, BackY0, BackX1, BackY1);
    }

    // ===================== タイトル =====================
    class TitleScene : public Scene {
    public:
        void Update(AppContext& app) override {
            ++frame_;
            int x = 0;
            int y = 0;
            if (frame_ > 20 && (app.input.Menu().Confirm() || app.input.Tapped(x, y))) {
                Sound::Play(Sound::Decide);
                app.next = MakeModeSelectScene();
            }
        }

        void Draw(const AppContext& app) const override {
            (void)app;
            DrawStage(frame_ / 300 % 2, 300.0f + std::sin(frame_ * 0.004f) * 150.0f, frame_);
            Ui::Dim(110);

            // 2人のキャラ
            Ui::DrawCharacter(0, 230.0f, 560.0f, 1.45f, 1, BlendPose(IdlePose(frame_), GetPose(P_Straight), 0.15f), frame_);
            Ui::DrawCharacter(1, 580.0f, 560.0f, 1.35f, -1, IdlePose(frame_ + 30), frame_);

            // タイトル
            const float glow = 0.5f + 0.5f * std::sin(frame_ * 0.05f);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(60 + 60 * glow));
            DrawOvalAA(400.0f, 150.0f, 260.0f, 70.0f, 48, GetColor(255, 140, 60), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            Text::Draw(Text::Huge, 400, 92, Config::GameTitle, GetColor(255, 230, 150), GetColor(120, 30, 10), Text::AlignCenter);
            Text::Draw(Text::Medium, 400, 190, Config::GameTitleRoman, White(), kBlack, Text::AlignCenter);
            Text::Draw(Text::Small, 400, 222, "― 風と岩の格闘伝 ―", GetColor(220, 220, 230), kBlack, Text::AlignCenter);

            if ((frame_ / 30) % 2 == 0) {
                const char* msg = Platform::UsesTouchControls() ? "画面をタップしてスタート" : "PRESS START（Enter / J / パッドの A）";
                Text::Draw(Text::Medium, 400, 470, msg, White(), kBlack, Text::AlignCenter);
            }
        }

    private:
        int frame_ = 0;
    };

    // ===================== モード選択 =====================
    class ModeSelectScene : public Scene {
    public:
        enum Item { ItemCpu, ItemVersus, ItemTraining, ItemHowTo, ItemQuit, ItemCount };

        ModeSelectScene() {
            count_ = Platform::CanQuitFromMenu() ? ItemCount : ItemQuit;
        }

        void Update(AppContext& app) override {
            ++frame_;
            const MenuNav& nav = app.input.Menu();
            if (nav.MoveY() != 0) {
                cursor_ = (cursor_ + nav.MoveY() + count_) % count_;
                Sound::Play(Sound::Cursor);
            }
            if (cursor_ == ItemCpu && nav.MoveX() != 0) {
                app.setup.cpuLevel = Clampi(app.setup.cpuLevel + nav.MoveX(), 0, 2);
                Sound::Play(Sound::Cursor);
            }
            int tx = 0;
            int ty = 0;
            bool decide = nav.Confirm();
            if (app.input.Tapped(tx, ty)) {
                if (Ui::Inside(tx, ty, BackX0, BackY0, BackX1, BackY1)) {
                    Sound::Play(Sound::Cancel);
                    app.next = MakeTitleScene();
                    return;
                }
                for (int i = 0; i < count_; ++i) {
                    if (Ui::Inside(tx, ty, X0, ItemY(i), X1, ItemY(i) + ItemH)) {
                        cursor_ = i;
                        if (i == ItemCpu && tx >= X1 - 190) {
                            // 強さの ◀ ▶
                            app.setup.cpuLevel = Clampi(app.setup.cpuLevel + (tx < X1 - 95 ? -1 : 1), 0, 2);
                            Sound::Play(Sound::Cursor);
                        } else {
                            decide = true;
                        }
                    }
                }
            }
            if (nav.Cancel()) {
                Sound::Play(Sound::Cancel);
                app.next = MakeTitleScene();
                return;
            }
            if (!decide) {
                return;
            }
            switch (cursor_) {
            case ItemCpu:
                Sound::Play(Sound::Decide);
                app.next = MakeCharacterSelectScene(BattleMode::Cpu);
                break;
            case ItemVersus:
                if (!Platform::SupportsLocalVersus()) {
                    Sound::Play(Sound::Cancel);
                    break;
                }
                Sound::Play(Sound::Decide);
                app.next = MakeCharacterSelectScene(BattleMode::Versus);
                break;
            case ItemTraining:
                Sound::Play(Sound::Decide);
                app.next = MakeCharacterSelectScene(BattleMode::Training);
                break;
            case ItemHowTo:
                Sound::Play(Sound::Decide);
                app.next = MakeHowToScene();
                break;
            case ItemQuit:
                app.quit = true;
                break;
            default:
                break;
            }
        }

        void Draw(const AppContext& app) const override {
            DrawStage(0, 500.0f, frame_);
            Ui::Dim(150);
            Text::Draw(Text::Large, 400, 40, "モードを選ぶ", Gold(), kBlack, Text::AlignCenter);
            DrawBackButton();

            static const char* labels[] = { "CPU と対戦", "2人で対戦", "トレーニング", "操作方法", "おわる" };
            static const char* descs[] = {
                "CPU と2本先取で戦います。左右で CPU の強さを変えられます。",
                "1台の PC で2人で戦います（2P はキーボードの矢印キー側か2台目のパッド）。",
                "相手を自由に動かせない状態で、技やコンボを練習します。",
                "キーボード・パッド・タッチの操作と、システムの説明です。",
                "ゲームを終わります。",
            };
            for (int i = 0; i < count_; ++i) {
                const int y = ItemY(i);
                const bool sel = i == cursor_;
                Ui::Panel(X0, y, X1, y + ItemH, 200, sel);
                const bool enabled = !(i == ItemVersus && !Platform::SupportsLocalVersus());
                Text::Draw(Text::Medium, X0 + 24, y + 12, labels[i], enabled ? (sel ? Gold() : White()) : Gray(), kBlack);
                if (i == ItemCpu) {
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "◀  %s  ▶", LevelName(app.setup.cpuLevel));
                    Text::Draw(Text::Medium, X1 - 95, y + 12, buf, sel ? Gold() : White(), kBlack, Text::AlignCenter);
                }
                if (i == ItemVersus && !enabled) {
                    Text::Draw(Text::Small, X1 - 16, y + 16, "スマホ1台では遊べません", Gray(), kBlack, Text::AlignRight);
                }
            }
            Ui::Panel(60, 500, 740, 560, 200);
            Text::Draw(Text::Small, 400, 522, descs[cursor_], White(), kBlack, Text::AlignCenter);
        }

    private:
        static constexpr int X0 = 200;
        static constexpr int X1 = 600;
        static constexpr int ItemH = 50;
        static int ItemY(int i) { return 110 + i * 64; }

        int cursor_ = 0;
        int count_ = ItemCount;
        int frame_ = 0;
    };

    // ===================== 操作方法 =====================
    class HowToScene : public Scene {
    public:
        void Update(AppContext& app) override {
            ++frame_;
            const MenuNav& nav = app.input.Menu();
            int tx = 0;
            int ty = 0;
            if (BackTapped(app) || nav.Cancel()) {
                Sound::Play(Sound::Cancel);
                app.next = MakeModeSelectScene();
                return;
            }
            int move = nav.MoveX();
            if (nav.Confirm()) {
                move = 1;
            }
            if (app.input.Tapped(tx, ty)) {
                move = tx < Config::ScreenWidth / 2 ? -1 : 1;
            }
            if (move != 0) {
                const int next = Clampi(page_ + move, 0, PageCount - 1);
                if (next != page_) {
                    page_ = next;
                    Sound::Play(Sound::Cursor);
                } else if (move > 0 && page_ == PageCount - 1) {
                    Sound::Play(Sound::Cancel);
                    app.next = MakeModeSelectScene();
                }
            }
        }

        void Draw(const AppContext& app) const override {
            (void)app;
            DrawStage(1, 400.0f, frame_);
            Ui::Dim(170);
            DrawBackButton();
            static const char* titles[] = { "操作（キーボード・パッド）", "操作（スマホ）", "システム", "キャラクターの技" };
            Text::Draw(Text::Large, 400, 20, titles[page_], Gold(), kBlack, Text::AlignCenter);
            Ui::Panel(30, 76, 770, 540, 210);

            int y = 92;
            auto line = [&y](const char* head, const char* body) {
                Text::Draw(Text::Small, 50, y, head, GetColor(255, 210, 90), kBlack);
                y += Ui::DrawWrapped(Text::Small, 230, y, 520, body, GetColor(255, 255, 255)) + 4;
            };
            switch (page_) {
            case 0:
                line("キーボード 1P", "移動 W A S D ／ 弱 J ／ 中 K ／ 強 L ／ 必殺 I ／ ポーズ Enter ／ 戻る Esc");
                line("キーボード 2P", "移動 矢印キー ／ 弱 テンキー1（または ,） ／ 中 テンキー2（または .） ／ 強 テンキー3（または /） ／ 必殺 テンキー0（または 右Shift） ／ ポーズ テンキーEnter");
                line("ゲームパッド", "移動 十字キー・左スティック ／ 弱 X ／ 中 Y ／ 強 B・RB ／ 必殺 A・LB ／ ポーズ START ／ 戻る BACK");
                line("メニュー", "決定：弱・必殺・Enter（パッドの X・A） ／ 戻る：強・Esc（パッドの B）。マウスでクリックしても選べます。");
                line("2人対戦", "1P はキーボードの WASD 側か1台目のパッド、2P は矢印キー側か2台目のパッドで操作します。CPU 戦とトレーニングでは、どの機器でも 1P を操作できます。");
                break;
            case 1:
                line("移動", "画面の左下のスティックを指でなぞります。上でジャンプ、下でしゃがみ、相手と反対側でガード。");
                line("攻撃", "右下の 弱・中・強 ボタン。弱と中を同時に押すと投げ。");
                line("必殺", "必殺ボタンと方向で必殺技が出ます（下の「システム」を参照）。コマンド入力でも出せます。");
                line("ポーズ", "画面の上の「II」ボタン。");
                line("メニュー", "項目を直接タップして選びます。");
                break;
            case 2:
                line("ガード", "相手と反対の方向に入れるとガード。下段の技はしゃがみガード（↙）、ジャンプ攻撃は立ちガード（←）でないと防げません。");
                line("ダッシュ", "前を2回で前ダッシュ、後ろを2回でバックステップ（出始めは無敵）。");
                line("投げ", "近くで 弱＋中。後ろを入れながらだと逆側に投げます。投げられた直後に 弱＋中 で投げ抜け。");
                line("チェーン", "当たった（ガードされた）通常技は、より強い通常技で取り消せます。例：しゃがみ弱 → しゃがみ中 → 立ち強");
                line("キャンセル", "通常技が当たった瞬間に必殺技のコマンドを入れると、必殺技につながります。必殺技は超必殺技でキャンセルできます。");
                line("超必殺技", "ゲージが満タン（SUPER）のとき ↓↘→↓↘→＋攻撃。ゲージは攻撃を当てる・受けると増えます。");
                line("ショートカット", "必殺ボタン＋方向：中立＝↓↘→の技、前＝→↓↘の技、後ろ＝↓↙←の技、下＝超必殺技。");
                line("カウンター", "相手の技の出始めに当てると、ダメージとのけぞりが増えます。");
                break;
            default:
                for (int c = 0; c < CharacterCount(); ++c) {
                    const CharacterDef& d = GetCharacter(c);
                    char head[64];
                    std::snprintf(head, sizeof(head), "%s（%s）", d.name, d.title);
                    Text::Draw(Text::Medium, 50, y, head, Gold(), kBlack);
                    y += 32;
                    for (const auto& cmd : d.commands) {
                        if (cmd.command[0] == '\0') {
                            continue;
                        }
                        Text::Draw(Text::Small, 70, y, cmd.command, GetColor(200, 230, 255), kBlack);
                        Text::Draw(Text::Small, 300, y, cmd.name, White(), kBlack);
                        y += 24;
                    }
                    y += 12;
                }
                break;
            }
            char buf[64];
            std::snprintf(buf, sizeof(buf), "◀  %d / %d  ▶", page_ + 1, PageCount);
            Text::Draw(Text::Medium, 400, 552, buf, White(), kBlack, Text::AlignCenter);
        }

    private:
        static constexpr int PageCount = 4;
        int page_ = 0;
        int frame_ = 0;
    };

    // ===================== キャラクター選択 =====================
    class CharacterSelectScene : public Scene {
    public:
        explicit CharacterSelectScene(BattleMode mode) : mode_(mode) {}

        void Update(AppContext& app) override {
            ++frame_;
            if (startTimer_ > 0) {
                if (--startTimer_ == 0) {
                    BattleSetup s = app.setup;
                    s.mode = mode_;
                    s.character[0] = cursor_[0];
                    s.character[1] = cursor_[1];
                    s.stage = GetCharacter(cursor_[1]).stage;
                    app.setup = s;
                    app.next = MakeBattleScene(s);
                }
                return;
            }
            if (mode_ == BattleMode::Versus) {
                UpdateVersus(app);
            } else {
                UpdateSingle(app);
            }
        }

        void Draw(const AppContext& app) const override {
            (void)app;
            DrawStage(GetCharacter(cursor_[step_ == 0 && mode_ != BattleMode::Versus ? 0 : 1]).stage, 500.0f, frame_);
            Ui::Dim(140);
            DrawBackButton();
            const char* title = "キャラクターを選ぶ";
            if (mode_ != BattleMode::Versus) {
                title = step_ == 0 ? "使うキャラクターを選ぶ" : (mode_ == BattleMode::Cpu ? "CPU のキャラクターを選ぶ" : "練習相手を選ぶ");
            }
            Text::Draw(Text::Large, 400, 18, title, Gold(), kBlack, Text::AlignCenter);

            for (int c = 0; c < CharacterCount(); ++c) {
                const CharacterDef& d = GetCharacter(c);
                const int x0 = CardX(c);
                const int x1 = x0 + CardW;
                const bool focus = (cursor_[0] == c && (mode_ == BattleMode::Versus || step_ == 0)) ||
                                   (cursor_[1] == c && (mode_ == BattleMode::Versus || step_ == 1));
                Ui::Panel(x0, CardY0, x1, CardY1, 190, focus);
                Text::Draw(Text::Large, x0 + 20, CardY0 + 12, d.name, White(), kBlack);
                Text::Draw(Text::Small, x0 + 22, CardY0 + 58, d.title, GetColor(200, 220, 255), kBlack);
                const char* statNames[] = { "パワー", "スピード", "リーチ" };
                const int stats[] = { d.statPower, d.statSpeed, d.statRange };
                for (int k = 0; k < 3; ++k) {
                    const int sy = CardY0 + 90 + k * 24;
                    Text::Draw(Text::Small, x0 + 22, sy, statNames[k], White(), kBlack);
                    for (int s = 0; s < 5; ++s) {
                        const unsigned col = s < stats[k] ? Gold() : GetColor(60, 60, 70);
                        DrawBox(x0 + 110 + s * 22, sy + 3, x0 + 128 + s * 22, sy + 15, col, TRUE);
                    }
                }
                const bool chosen = (cursor_[0] == c && (locked_[0] || step_ > 0)) || (cursor_[1] == c && locked_[1]);
                const Pose pose = chosen ? GetPose(P_Win) : IdlePose(frame_ + c * 20);
                Ui::DrawCharacter(c, static_cast<float>(x0 + CardW / 2), static_cast<float>(CardY1 - 14), 1.2f, c == 0 ? 1 : -1, pose, frame_);

                // カーソルの札
                for (int p = 0; p < 2; ++p) {
                    const bool show = mode_ == BattleMode::Versus || (p == 0) || step_ == 1;
                    if (!show || cursor_[p] != c) {
                        continue;
                    }
                    const char* tag = p == 0 ? "1P" : (mode_ == BattleMode::Versus ? "2P" : (mode_ == BattleMode::Cpu ? "CPU" : "相手"));
                    const unsigned col = p == 0 ? GetColor(230, 60, 60) : GetColor(60, 120, 230);
                    const int tx = p == 0 ? x0 + 12 : x1 - 92;
                    DrawBox(tx, CardY1 - 44, tx + 80, CardY1 - 12, col, TRUE);
                    Text::Draw(Text::Medium, tx + 40, CardY1 - 41, tag, White(), kBlack, Text::AlignCenter);
                    const bool isLocked = mode_ == BattleMode::Versus ? locked_[p] : (p == 0 && step_ > 0);
                    if (isLocked) {
                        Text::Draw(Text::Small, tx + 40, CardY1 - 64, "決定", Gold(), kBlack, Text::AlignCenter);
                    }
                }
            }

            // 説明
            const int focusChar = (mode_ != BattleMode::Versus && step_ == 1) ? cursor_[1] : cursor_[0];
            Ui::Panel(40, 476, 760, 584, 200);
            Ui::DrawWrapped(Text::Small, 58, 488, 684, GetCharacter(focusChar).style, White());
            const char* help = Platform::UsesTouchControls() ? "カードをタップして決定" :
                               (mode_ == BattleMode::Versus ? "1P・2P それぞれ左右で選んで決定" : "左右で選んで決定（弱・Enter）、戻る（強・Esc）");
            Text::Draw(Text::Small, 400, 558, help, GetColor(200, 200, 210), kBlack, Text::AlignCenter);
        }

    private:
        static constexpr int CardW = 330;
        static constexpr int CardY0 = 70;
        static constexpr int CardY1 = 466;
        static int CardX(int c) { return c == 0 ? 50 : 420; }

        int TappedCard(const AppContext& app) const {
            int tx = 0;
            int ty = 0;
            if (!app.input.Tapped(tx, ty)) {
                return -1;
            }
            for (int c = 0; c < CharacterCount(); ++c) {
                if (Ui::Inside(tx, ty, CardX(c), CardY0, CardX(c) + CardW, CardY1)) {
                    return c;
                }
            }
            return -1;
        }

        void UpdateSingle(AppContext& app) {
            const MenuNav& nav = app.input.Menu();
            if (BackTapped(app) || nav.Cancel()) {
                Sound::Play(Sound::Cancel);
                if (step_ == 0) {
                    app.next = MakeModeSelectScene();
                } else {
                    step_ = 0;
                }
                return;
            }
            int& cur = cursor_[step_];
            if (nav.MoveX() != 0) {
                cur = (cur + nav.MoveX() + CharacterCount()) % CharacterCount();
                Sound::Play(Sound::Cursor);
            }
            bool decide = nav.Confirm();
            const int tapped = TappedCard(app);
            if (tapped >= 0) {
                cur = tapped;
                decide = true;
            }
            if (!decide) {
                return;
            }
            Sound::Play(Sound::Decide);
            if (step_ == 0) {
                step_ = 1;
                cursor_[1] = 1 - cursor_[0];
            } else {
                startTimer_ = 40;
            }
        }

        void UpdateVersus(AppContext& app) {
            if (BackTapped(app)) {
                Sound::Play(Sound::Cancel);
                app.next = MakeModeSelectScene();
                return;
            }
            for (int p = 0; p < 2; ++p) {
                const MenuNav& nav = app.input.PlayerMenu(p);
                if (nav.Cancel()) {
                    Sound::Play(Sound::Cancel);
                    if (locked_[p]) {
                        locked_[p] = false;
                    } else if (!locked_[0] && !locked_[1]) {
                        app.next = MakeModeSelectScene();
                        return;
                    }
                }
                if (locked_[p]) {
                    continue;
                }
                if (nav.MoveX() != 0) {
                    cursor_[p] = (cursor_[p] + nav.MoveX() + CharacterCount()) % CharacterCount();
                    Sound::Play(Sound::Cursor);
                }
                if (nav.Confirm()) {
                    locked_[p] = true;
                    Sound::Play(Sound::Decide);
                }
            }
            // マウスでも選べるように：まだ決めていない方のカーソルを動かして決定する
            const int tapped = TappedCard(app);
            if (tapped >= 0) {
                const int p = !locked_[0] ? 0 : (!locked_[1] ? 1 : -1);
                if (p >= 0) {
                    cursor_[p] = tapped;
                    locked_[p] = true;
                    Sound::Play(Sound::Decide);
                }
            }
            if (locked_[0] && locked_[1]) {
                startTimer_ = 40;
            }
        }

        BattleMode mode_;
        int cursor_[2] = { 0, 1 };
        bool locked_[2] = { false, false };
        int step_ = 0;
        int startTimer_ = 0;
        int frame_ = 0;
    };

    // ===================== 結果 =====================
    class ResultScene : public Scene {
    public:
        ResultScene(const BattleSetup& setup, int winner) : setup_(setup), winner_(winner) {}

        void Update(AppContext& app) override {
            ++frame_;
            if (frame_ < 30) {
                return;
            }
            const MenuNav& nav = app.input.Menu();
            if (nav.MoveY() != 0) {
                cursor_ = (cursor_ + nav.MoveY() + ItemCount) % ItemCount;
                Sound::Play(Sound::Cursor);
            }
            bool decide = nav.Confirm();
            int tx = 0;
            int ty = 0;
            if (app.input.Tapped(tx, ty)) {
                for (int i = 0; i < ItemCount; ++i) {
                    if (Ui::Inside(tx, ty, X0, ItemY(i), X1, ItemY(i) + 44)) {
                        cursor_ = i;
                        decide = true;
                    }
                }
            }
            if (!decide) {
                return;
            }
            Sound::Play(Sound::Decide);
            if (cursor_ == 0) {
                app.next = MakeBattleScene(setup_);
            } else if (cursor_ == 1) {
                app.next = MakeCharacterSelectScene(setup_.mode);
            } else {
                app.next = MakeTitleScene();
            }
        }

        void Draw(const AppContext& app) const override {
            (void)app;
            DrawStage(setup_.stage, 300.0f + frame_ * 0.3f, frame_);
            Ui::Dim(130);
            if (winner_ < 0) {
                Text::Draw(Text::Huge, 400, 60, "DRAW", Gold(), kBlack, Text::AlignCenter);
                Ui::DrawCharacter(setup_.character[0], 260.0f, 470.0f, 1.3f, 1, GetPose(P_Lose), frame_);
                Ui::DrawCharacter(setup_.character[1], 540.0f, 470.0f, 1.3f, -1, GetPose(P_Lose), frame_);
            } else {
                const CharacterDef& d = GetCharacter(setup_.character[winner_]);
                const bool cpuWon = setup_.mode == BattleMode::Cpu && winner_ == 1;
                const char* head = cpuWon ? "YOU LOSE" : (setup_.mode == BattleMode::Cpu ? "YOU WIN" : (winner_ == 0 ? "1P WIN" : "2P WIN"));
                Text::Draw(Text::Huge, 400, 30, head, cpuWon ? GetColor(140, 180, 255) : Gold(), kBlack, Text::AlignCenter);
                const Pose pose = BlendPose(GetPose(P_Stand), GetPose(P_Win), Clampf(frame_ / 20.0f, 0.0f, 1.0f));
                Ui::DrawCharacter(setup_.character[winner_], 220.0f, 500.0f, 1.6f, 1, pose, frame_);
                Ui::Panel(400, 150, 770, 300, 200);
                Text::Draw(Text::Large, 420, 160, d.name, White(), kBlack);
                Ui::DrawWrapped(Text::Medium, 420, 215, 330, d.winQuote, GetColor(255, 240, 200));
            }
            static const char* labels[] = { "もう一度", "キャラクター選択", "タイトルへ" };
            for (int i = 0; i < ItemCount; ++i) {
                const bool sel = i == cursor_;
                Ui::Panel(X0, ItemY(i), X1, ItemY(i) + 44, 200, sel);
                Text::Draw(Text::Medium, (X0 + X1) / 2, ItemY(i) + 10, labels[i], sel ? Gold() : White(), kBlack, Text::AlignCenter);
            }
        }

    private:
        static constexpr int ItemCount = 3;
        static constexpr int X0 = 470;
        static constexpr int X1 = 730;
        static int ItemY(int i) { return 340 + i * 56; }

        BattleSetup setup_;
        int winner_;
        int cursor_ = 0;
        int frame_ = 0;
    };
}

std::unique_ptr<Scene> MakeTitleScene() {
    return std::unique_ptr<Scene>(new TitleScene());
}

std::unique_ptr<Scene> MakeModeSelectScene() {
    return std::unique_ptr<Scene>(new ModeSelectScene());
}

std::unique_ptr<Scene> MakeHowToScene() {
    return std::unique_ptr<Scene>(new HowToScene());
}

std::unique_ptr<Scene> MakeCharacterSelectScene(BattleMode mode) {
    return std::unique_ptr<Scene>(new CharacterSelectScene(mode));
}

std::unique_ptr<Scene> MakeResultScene(const BattleSetup& setup, int winner) {
    return std::unique_ptr<Scene>(new ResultScene(setup, winner));
}
