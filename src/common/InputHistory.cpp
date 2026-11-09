#include "pch.h"
#include "InputHistory.h"

void InputHistory::Clear() {
    for (Entry& e : entries_) {
        e = Entry();
    }
    head_ = 0;
    count_ = 0;
    consumed_ = serial_;
}

void InputHistory::Push(const Pad& pad, int facing, bool frozen) {
    Pad p = pad;
    p.Clean();
    const bool forward = facing >= 0 ? p.right : p.left;
    const bool back = facing >= 0 ? p.left : p.right;
    int dir = 5;
    if (p.down) {
        dir = back ? 1 : (forward ? 3 : 2);
    } else if (p.up) {
        dir = back ? 7 : (forward ? 9 : 8);
    } else {
        dir = back ? 4 : (forward ? 6 : 5);
    }

    unsigned held = 0;
    for (int i = 0; i < BtnCount; ++i) {
        if (p.btn[i]) {
            held |= 1u << i;
        }
    }
    const unsigned prevHeld = count_ > 0 ? entries_[head_].held : 0u;

    head_ = (head_ + 1) % Size;
    if (count_ < Size) {
        ++count_;
    }
    Entry& e = entries_[head_];
    e.dir = dir;
    e.held = held;
    e.pressed = held & ~prevHeld;
    e.frozen = frozen;
    e.serial = ++serial_;
}

const InputHistory::Entry& InputHistory::At(int ago) const {
    const int index = ((head_ - ago) % Size + Size) % Size;
    return entries_[index];
}

int InputHistory::Dir(int ago) const {
    if (ago >= count_) {
        return 5;
    }
    return At(ago).dir;
}

bool InputHistory::Held(Btn b) const {
    if (count_ == 0) {
        return false;
    }
    return (At(0).held & (1u << b)) != 0;
}

bool InputHistory::Pressed(Btn b, int frames) const {
    int counted = 0;
    for (int ago = 0; ago < count_; ++ago) {
        const Entry& e = At(ago);
        if (e.serial <= consumed_) {
            return false;
        }
        if ((e.pressed & (1u << b)) != 0) {
            return true;
        }
        if (!e.frozen && ++counted >= frames) {
            return false;
        }
    }
    return false;
}

void InputHistory::Consume() {
    consumed_ = serial_;
}

bool InputHistory::Motion(const int* seq, int count, int window) const {
    int index = count - 1;
    int counted = 0;
    for (int ago = 0; ago < count_; ++ago) {
        const Entry& e = At(ago);
        if (!e.frozen && ++counted > window) {
            return false;
        }
        if (index == count - 1) {
            // 最後の方向は、その方向に「入れ直した」瞬間（1つ前が別の方向）を探す
            if (e.dir != seq[index]) {
                // 最後の方向は、ボタンを押す直前（8フレーム以内）に入れたものに限る
                if (counted > 8) {
                    return false;
                }
                continue;
            }
            const bool changed = (ago + 1 >= count_) || At(ago + 1).dir != seq[index];
            if (!changed) {
                continue;
            }
            if (e.serial <= consumed_) {
                return false;
            }
            --index;
            if (index < 0) {
                return true;
            }
            continue;
        }
        if (e.dir == seq[index]) {
            --index;
            if (index < 0) {
                return true;
            }
        }
    }
    return false;
}

bool InputHistory::DoubleTap(int dir) const {
    if (count_ < 3 || At(0).dir != dir || At(1).dir == dir) {
        return false;
    }
    if (At(0).serial <= consumed_) {
        return false;
    }
    int counted = 0;
    for (int ago = 1; ago < count_; ++ago) {
        const Entry& e = At(ago);
        if (!e.frozen && ++counted > 12) {
            return false;
        }
        if (e.dir == dir) {
            return true;
        }
        if (e.dir != 5) {
            return false;
        }
    }
    return false;
}
