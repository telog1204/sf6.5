#pragma once
#include <memory>
#include "Scene.h"

// ゲーム本体。RunGame（GameLoop.cpp）が Update を1秒に Config::LogicFps 回、Draw を表示のたびに呼ぶ。両OSで共通。
// 画面（Scene）の切り替えと、そのときのフェードを受け持つ
class Game {
public:
    Game();
    ~Game();

    // 入力を読み、ゲームを1フレーム（1/60秒）進める
    void Update();

    // ゲームの画面（Config::ScreenWidth × Config::ScreenHeight）に描く
    void Draw();

    // 「おわる」が選ばれたか
    bool WantsQuit() const { return app_.quit; }

private:
    AppContext app_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Scene> pending_;
    int fadeOut_ = 0;
    int fadeIn_ = 0;
};
