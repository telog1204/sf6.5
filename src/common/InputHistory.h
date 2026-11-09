#pragma once
#include "Pad.h"

// キャラ1人分の入力の記録。コマンド技（↓↘→＋攻撃 など）や先行入力の判定に使う。
// 方向はキャラの向きを基準にしたテンキー表記（1：後ろ下、2：下、3：前下、4：後ろ、5：中立、6：前、7：後ろ上、8：上、9：前上）
class InputHistory {
public:
    static constexpr int Size = 96;

    void Clear();

    // 1フレーム分を記録する。facing はキャラの向き（1：右向き、-1：左向き）。
    // frozen はヒットストップ中など、ゲームの時間が止まっているフレーム（受付時間の計算に数えない）
    void Push(const Pad& pad, int facing, bool frozen);

    // ago フレーム前の方向（0 は今のフレーム）
    int Dir(int ago = 0) const;
    bool Held(Btn b) const;

    // まだ使っていない押し下げが、直近 frames フレーム以内にあるか（止まっているフレームは数えない）
    bool Pressed(Btn b, int frames) const;

    // ここまでの押し下げを「使用済み」にする（技を出したときに呼び、同じ入力で2回技が出ないようにする）
    void Consume();

    // 方向の並び seq（例：{2, 3, 6}）が、直近 window フレーム以内にこの順で入力されたか。
    // 最後の方向は、使用済みにした時点より後に新しく入れたものでなければならない
    bool Motion(const int* seq, int count, int window) const;

    // 前（6）または後ろ（4）を素早く2回入れたか（ダッシュ）
    bool DoubleTap(int dir) const;

private:
    struct Entry {
        int dir = 5;
        unsigned held = 0;
        unsigned pressed = 0;
        bool frozen = false;
        long long serial = 0;
    };

    const Entry& At(int ago) const;

    Entry entries_[Size];
    int head_ = 0;   // 一番新しい記録の位置
    int count_ = 0;  // 記録の数（Size まで）
    long long serial_ = 0;
    long long consumed_ = 0;
};
