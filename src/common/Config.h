#pragma once

// ゲーム全体の設定値。ゲームの調整に使う値はこのファイルにまとめる。
namespace Config {
    // ウィンドウのタイトル（Windows版）。Android版のアプリ名は android/app/src/main/res/values/strings.xml
    constexpr const char* WindowTitle = "sf6.5";

    // ゲーム内のタイトル表示
    constexpr const char* GameTitle = "拳嵐";
    constexpr const char* GameTitleRoman = "KENRAN";

    // ゲームの画面の大きさ。Windows版はこの大きさのウィンドウ、Android版はこれを端末の画面に拡大して表示する
    constexpr int ScreenWidth = 800;
    constexpr int ScreenHeight = 600;

    // ウィンドウの移動中などで1フレームが長引いたとき、時間が一気に進まないようにする上限（現実の秒）
    constexpr double MaxFrameSeconds = 0.1;

    // 背景色（Android版で、ゲームの画面の外側に余った部分もこの色で塗る）
    constexpr int BackgroundR = 0;
    constexpr int BackgroundG = 0;
    constexpr int BackgroundB = 0;

    // ゲームを1秒に何回進めるか。格闘ゲームの技の速さ（発生・硬直）はこの単位（フレーム）で作る
    constexpr int LogicFps = 60;
    // 描画が遅れたとき、1回の表示までに進めるフレームの上限（これを超えた分は捨てる）
    constexpr int MaxLogicStepsPerDraw = 4;

    // 対戦
    constexpr int RoundsToWin = 2;          // 何本先取で勝ちか
    constexpr int RoundSeconds = 99;        // 1ラウンドの制限時間（秒）
    constexpr float StageWidth = 1400.0f;   // ステージの横幅（ワールド座標）
    constexpr float GroundScreenY = 530.0f; // 地面を描く画面上の高さ
    constexpr float MaxFighterDistance = 690.0f; // 2人が離れられる最大の距離（画面に2人とも収まるように）
    constexpr int MeterMax = 1000;          // 超必殺技ゲージの最大値（満タンで1回使える）
}
