#pragma once

// ステージの背景。0：夕焼けの河原、1：夜の採石場
constexpr int StageCount = 2;
const char* StageName(int stage);

// cameraLeft はワールド座標で画面の左端がどこか。遠くのものほどゆっくり動かして奥行きを出す
void DrawStage(int stage, float cameraLeft, int frame);
