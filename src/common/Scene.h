#pragma once
#include <memory>
#include "Battle.h"
#include "Input.h"

struct AppContext;

// 画面（タイトル・メニュー・対戦など）。Game が今の画面の Update と Draw を毎フレーム呼ぶ
class Scene {
public:
    virtual ~Scene() = default;
    virtual void Update(AppContext& app) = 0;
    virtual void Draw(const AppContext& app) const = 0;
    // タッチの端末で、画面上の仮想パッドを使う画面か
    virtual bool UsesVirtualPad() const { return false; }
};

// すべての画面で共有するもの
struct AppContext {
    InputSystem input;
    BattleSetup setup;            // 最後に選んだ対戦の設定（もう一度遊ぶときに使う）
    int frame = 0;
    bool quit = false;
    std::unique_ptr<Scene> next;  // 次の画面。入れると Game がフェードして切り替える
};

std::unique_ptr<Scene> MakeTitleScene();
std::unique_ptr<Scene> MakeModeSelectScene();
std::unique_ptr<Scene> MakeHowToScene();
std::unique_ptr<Scene> MakeCharacterSelectScene(BattleMode mode);
std::unique_ptr<Scene> MakeBattleScene(const BattleSetup& setup);
std::unique_ptr<Scene> MakeResultScene(const BattleSetup& setup, int winner);
