#pragma once

// 効果音。素材ファイルを使わず、起動時に波形を計算して作る
namespace Sound {
    enum Id {
        HitLight,
        HitMedium,
        HitHeavy,
        Block,
        Whiff,
        Projectile,
        Shockwave,
        SuperStart,
        KO,
        Grab,
        Land,
        Cursor,
        Decide,
        Cancel,
        RoundCall,
        Count
    };

    void Init();
    void Play(Id id);
}
