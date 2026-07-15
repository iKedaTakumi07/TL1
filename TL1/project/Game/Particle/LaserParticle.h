#pragma once
#include "../../Engine/base/Math.h"
#include <vector>

class LaserParticle {
public:
    // 初期化
    void Initialize();

    // 一時的なテスト用vectorで弾の再現
    void NewTransform();

    // 生成
    void NewParticle(const Transform& emitterTransform);

    // 毎フレーム更新
    void Update();

    // 描画
    void Draw();

private:
    // 弾の座標を取得できない(エンジン作成がある程度完成するまでは)ため代理の座標
    std::vector<Transform> Pos;
};
