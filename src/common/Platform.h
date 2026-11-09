#pragma once
#include <string>
#include <vector>
#include "Pad.h"

// OSごとに違う部分（入力・ファイルの場所・画面への表示）を、ゲーム本体から同じ形で使うための窓口。
// 実装は src/windows/Platform_windows.cpp と src/android/Platform_android.cpp にあり、
// ビルドする側（Visual Studio のプロジェクト、Android の CMakeLists.txt）がどちらか一方だけを含める。

// 1フレーム分の入力。座標はゲームの画面（Config::ScreenWidth × Config::ScreenHeight）上の位置
struct InputState {
    int x = -1;                  // 指やマウスの位置。指が触れていないとき（タッチ）は画面の外になる
    int y = -1;
    bool primaryDown = false;    // 主な操作が続いている（Windows：左ボタン、Android：指1本）
    bool secondaryDown = false;  // 2つ目の操作が続いている（Windows：右ボタン、Android：指2本）
};

// 画面に触れている指1本の位置（ゲームの画面上の座標。拡大表示の余白に触れたときは画面の外の値になる）
struct TouchPoint {
    int x = 0;
    int y = 0;
};

namespace Platform {
    // 今のフレームの入力を読む（1フレームに1回呼ぶ）。メニューのタップ・クリックに使う
    InputState PollInput();

    // 画面に触れているすべての指（Windows：マウスの左ボタンを押している間だけ1つ）。仮想パッドに使う
    std::vector<TouchPoint> GetTouches();

    // キーボードの入力。set 0：1P用のキー、1：2P用のキー。キーボードを使わないOSでは何も押していない状態を返す
    Pad ReadKeyboard(int set);

    // 対戦中に画面上の仮想パッド（スティックとボタン）を表示して使うか
    bool UsesTouchControls();

    // 1台で2人対戦ができるか（Android はスマホ1台を1人で持つ前提なのでできない）
    bool SupportsLocalVersus();

    // メニューに「おわる」を出すか（Android はホームボタンなどで閉じるので出さない）
    bool CanQuitFromMenu();

    // セーブデータを書き込むファイルのパス（Windows：作業フォルダ、Android：アプリ専用の内部ストレージ）
    std::string SaveFilePath(const char* fileName);

    // 1フレームの描画の前後に呼ぶ。描き終わったら画面に表示する
    void BeginDraw();
    void EndDraw();
}
