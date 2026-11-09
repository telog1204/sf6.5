#include "pch.h"
#include "Scene.h"
#include <DxLib.h>
#include "Config.h"
#include "Sound.h"
#include "Text.h"
#include "Ui.h"

// 対戦の画面。Battle を進め、ポーズメニューとコマンド表を出す

namespace {
    const unsigned kBlack = 0x000000u;

    class BattleScene : public Scene {
    public:
        explicit BattleScene(const BattleSetup& setup) : setup_(setup) {
            battle_.Start(setup);
        }

        bool UsesVirtualPad() const override { return !paused_; }

        void Update(AppContext& app) override {
            ++frame_;
            if (paused_) {
                UpdatePause(app);
                return;
            }
            const MenuNav& nav = app.input.Menu();
            if (frame_ > 10 && (nav.Pressed(BtnStart) || nav.Pressed(BtnBack))) {
                paused_ = true;
                cursor_ = 0;
                showCommands_ = false;
                Sound::Play(Sound::Cursor);
                return;
            }
            const bool separate = setup_.mode == BattleMode::Versus;
            battle_.Step(app.input.PlayerPad(0, separate), app.input.PlayerPad(1, separate));
            if (battle_.IsFinished()) {
                app.next = MakeResultScene(setup_, battle_.Winner());
            }
        }

        void Draw(const AppContext& app) const override {
            battle_.Draw();
            if (!paused_) {
                if (Platform::UsesTouchControls()) {
                    app.input.Virtual().Draw();
                }
                return;
            }
            Ui::Dim(150);
            if (showCommands_) {
                DrawCommands();
                return;
            }
            const std::vector<Item> items = Items();
            const int h = 46;
            const int top = 300 - static_cast<int>(items.size()) * h / 2;
            Text::Draw(Text::Large, 400, top - 64, "ポーズ", GetColor(255, 210, 90), kBlack, Text::AlignCenter);
            for (size_t i = 0; i < items.size(); ++i) {
                const int y = top + static_cast<int>(i) * h;
                const bool sel = static_cast<int>(i) == cursor_;
                Ui::Panel(X0, y, X1, y + h - 8, 210, sel);
                Text::Draw(Text::Medium, 400, y + 7, ItemLabel(items[i]).c_str(), sel ? GetColor(255, 210, 90) : GetColor(255, 255, 255),
                           kBlack, Text::AlignCenter);
            }
            if (setup_.mode == BattleMode::Training) {
                Text::Draw(Text::Small, 400, top + static_cast<int>(items.size()) * h + 6, "左右で設定を切り替え",
                           GetColor(200, 200, 210), kBlack, Text::AlignCenter);
            }
        }

    private:
        enum class Item { Resume, Dummy, Boxes, Reset, Commands, CharSelect, Title };
        static constexpr int X0 = 230;
        static constexpr int X1 = 570;

        std::vector<Item> Items() const {
            std::vector<Item> items;
            items.push_back(Item::Resume);
            if (setup_.mode == BattleMode::Training) {
                items.push_back(Item::Dummy);
                items.push_back(Item::Boxes);
                items.push_back(Item::Reset);
            }
            items.push_back(Item::Commands);
            items.push_back(Item::CharSelect);
            items.push_back(Item::Title);
            return items;
        }

        std::string ItemLabel(Item item) const {
            static const char* dummyNames[] = { "立ち", "しゃがみ", "ガード", "CPU" };
            switch (item) {
            case Item::Resume:
                return "対戦にもどる";
            case Item::Dummy:
                return std::string("相手の動き：◀ ") + dummyNames[static_cast<int>(battle_.dummyMode)] + " ▶";
            case Item::Boxes:
                return std::string("判定の表示：◀ ") + (battle_.showBoxes ? "あり" : "なし") + " ▶";
            case Item::Reset:
                return "位置をリセット";
            case Item::Commands:
                return "コマンド表";
            case Item::CharSelect:
                return "キャラクター選択へ";
            case Item::Title:
                return "タイトルへ";
            }
            return "";
        }

        void Toggle(Item item, int dir) {
            if (item == Item::Dummy) {
                const int n = static_cast<int>(DummyMode::Count);
                battle_.dummyMode = static_cast<DummyMode>((static_cast<int>(battle_.dummyMode) + dir + n) % n);
                Sound::Play(Sound::Cursor);
            } else if (item == Item::Boxes) {
                battle_.showBoxes = !battle_.showBoxes;
                Sound::Play(Sound::Cursor);
            }
        }

        void UpdatePause(AppContext& app) {
            const MenuNav& nav = app.input.Menu();
            int tx = 0;
            int ty = 0;
            const bool tapped = app.input.Tapped(tx, ty);
            if (showCommands_) {
                if (nav.Confirm() || nav.Cancel() || tapped) {
                    showCommands_ = false;
                    Sound::Play(Sound::Cancel);
                }
                return;
            }
            const std::vector<Item> items = Items();
            const int count = static_cast<int>(items.size());
            if (nav.Pressed(BtnStart) || nav.Pressed(BtnBack)) {
                paused_ = false;
                Sound::Play(Sound::Cancel);
                return;
            }
            if (nav.MoveY() != 0) {
                cursor_ = (cursor_ + nav.MoveY() + count) % count;
                Sound::Play(Sound::Cursor);
            }
            if (nav.MoveX() != 0) {
                Toggle(items[cursor_], nav.MoveX());
            }
            bool decide = nav.Confirm();
            if (tapped) {
                const int h = 46;
                const int top = 300 - count * h / 2;
                for (int i = 0; i < count; ++i) {
                    if (Ui::Inside(tx, ty, X0, top + i * h, X1, top + i * h + h - 8)) {
                        cursor_ = i;
                        decide = true;
                    }
                }
            }
            if (!decide) {
                return;
            }
            switch (items[cursor_]) {
            case Item::Resume:
                paused_ = false;
                Sound::Play(Sound::Decide);
                break;
            case Item::Dummy:
            case Item::Boxes:
                Toggle(items[cursor_], 1);
                break;
            case Item::Reset:
                battle_.ResetTraining();
                paused_ = false;
                Sound::Play(Sound::Decide);
                break;
            case Item::Commands:
                showCommands_ = true;
                Sound::Play(Sound::Decide);
                break;
            case Item::CharSelect:
                Sound::Play(Sound::Decide);
                app.next = MakeCharacterSelectScene(setup_.mode);
                break;
            case Item::Title:
                Sound::Play(Sound::Decide);
                app.next = MakeTitleScene();
                break;
            }
        }

        void DrawCommands() const {
            Ui::Panel(30, 30, 770, 570, 230);
            Text::Draw(Text::Large, 400, 40, "コマンド表", GetColor(255, 210, 90), kBlack, Text::AlignCenter);
            for (int p = 0; p < 2; ++p) {
                const CharacterDef& d = GetCharacter(setup_.character[p]);
                const int x = p == 0 ? 50 : 415;
                int y = 100;
                char head[64];
                std::snprintf(head, sizeof(head), "%s %s", p == 0 ? "1P" : "2P", d.name);
                Text::Draw(Text::Medium, x, y, head, p == 0 ? GetColor(255, 140, 140) : GetColor(140, 180, 255), kBlack);
                y += 36;
                for (const auto& cmd : d.commands) {
                    if (cmd.command[0] == '\0') {
                        continue;
                    }
                    Text::Draw(Text::Small, x, y, cmd.command, GetColor(200, 230, 255), kBlack);
                    y += 22;
                    Text::Draw(Text::Small, x + 20, y, cmd.name, GetColor(255, 255, 255), kBlack);
                    y += 30;
                }
            }
            Ui::DrawWrapped(Text::Small, 50, 430, 700,
                            "必殺ボタン＋方向でも出せます（中立：↓↘→の技、前：→↓↘の技、後ろ：↓↙←の技、下：超必殺技）。"
                            "通常技は 弱→中→強 の順につなげられ、当たった瞬間に必殺技でキャンセルできます。",
                            GetColor(230, 230, 230));
            Text::Draw(Text::Small, 400, 540, "決定・戻る・タップで閉じる", GetColor(200, 200, 210), kBlack, Text::AlignCenter);
        }

        BattleSetup setup_;
        Battle battle_;
        bool paused_ = false;
        bool showCommands_ = false;
        int cursor_ = 0;
        int frame_ = 0;
    };
}

std::unique_ptr<Scene> MakeBattleScene(const BattleSetup& setup) {
    return std::unique_ptr<Scene>(new BattleScene(setup));
}
