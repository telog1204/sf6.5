#include "pch.h"
#include "GameLoop.h"
#include <DxLib.h>
#include "Config.h"
#include "Game.h"
#include "Platform.h"

void RunGame() {
    Game game;
    const double step = 1.0 / Config::LogicFps;
    double accumulated = 0.0;
    long long prevCount = GetNowHiPerformanceCount();
    while (ProcessMessage() == 0 && !game.WantsQuit()) {
        const long long count = GetNowHiPerformanceCount();
        double dtSeconds = (count - prevCount) / 1000000.0;
        prevCount = count;
        if (dtSeconds > Config::MaxFrameSeconds) {
            dtSeconds = Config::MaxFrameSeconds;
        }
        // 表示が 60Hz のときは、時間のわずかなずれでゲームが 0 回や 2 回進んでカクつかないよう、1 フレームちょうどとみなす
        if (std::fabs(dtSeconds - step) < 0.002) {
            dtSeconds = step;
        }
        accumulated += dtSeconds;

        // 格闘ゲームの技はフレーム単位で作るので、ゲームは常に 1/60 秒ずつ進める
        int steps = 0;
        while (accumulated >= step && steps < Config::MaxLogicStepsPerDraw) {
            game.Update();
            accumulated -= step;
            ++steps;
        }
        if (steps == Config::MaxLogicStepsPerDraw) {
            accumulated = 0.0;
        }

        Platform::BeginDraw();
        ClearDrawScreen();
        game.Draw();
        Platform::EndDraw();
    }
}
