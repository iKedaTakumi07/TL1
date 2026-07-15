#include "LaserParticle.h"
#include "../../Engine/3d/CPUParticle/CPUParticleManager.h"
#include "../../Engine/base/Math.h"

void LaserParticle::Initialize()
{
    // 事前に読み込ませるため多分?
    CPUParticleManager::getInstance()->CreateParticleGroup("laser", "resources/laser.png", ParticleMeshType::kPlane);
}

void LaserParticle::NewTransform()
{
    Transform pos;
    // ゲーム作り始めたら削除(弾完成後)
    pos.scale = { 1.0f, 1.0f, 1.0f };
    pos.rotate = { 0.0f, 0.0f, 0.0f };
    pos.translate = { 0.0f, 2.0f, -2.0f };

    Pos.push_back(pos);
}

void LaserParticle::NewParticle(const Transform& emitterTransform)
{
    EmitterParam laserfireParam;
    for (int i = 0; i < 3; ++i) {
        float rotY = std::numbers::pi_v<float> / 2.0f;
        float rotZ = (std::numbers::pi_v<float> / 3.0f) * (float)i;

        laserfireParam.SetRotate({ 0.0f, rotY, rotZ });
        laserfireParam.SetScale({ 1.0f, 0.5f, 1.0f });
        laserfireParam.SetStartColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        laserfireParam.SetEndColor({ 1.0f, 1.0f, 1.0f, 0.0f });
        laserfireParam.SetVelocity({ 0.0f, 0.0f, 0.0f }); // 残像なのでその場に固定
        laserfireParam.SetLifeTime(0.6f);

        // えせトレイル
        CPUParticleManager::getInstance()->Emit("laser", emitterTransform, 1, laserfireParam);
    }
}

void LaserParticle::Update()
{
    // 試験的用(プレイヤー弾作成後削除)
    float speed = 30.0f;
    for (auto& pos_ : Pos) {
        pos_.translate.z += speed * (1.0f / 60.0f);
        NewParticle(pos_);
    }

    std::erase_if(Pos, [](const Transform& pos_) {
        return pos_.translate.z >= 50.0f;
    });
}

void LaserParticle::Draw()
{
    // パーティクルの全体描画実行あるため空。
}
