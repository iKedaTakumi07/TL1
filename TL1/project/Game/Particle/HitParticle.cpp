#include "HitParticle.h"
#include "../../Engine/3d/CPUParticle/CPUParticleManager.h"
#include "../../Engine/base/Math.h"

void HitParticle::Initialize()
{
    // 事前に読み込ませるため多分?
    CPUParticleManager::getInstance()->CreateParticleGroup("circle2", "resources/circle2.png", ParticleMeshType::kPlane);
    CPUParticleManager::getInstance()->CreateParticleGroup("circle3", "resources/circle3.png", ParticleMeshType::kPlane);
}

void HitParticle::NewTransform()
{
    Transform pos;
    // ゲーム作り始めたら削除(弾完成後)
    pos.scale = { 1.0f, 1.0f, 1.0f };
    pos.rotate = { 0.0f, 0.0f, 0.0f };
    pos.translate = { -4.0f, 2.0f, 0.0f };

    Pos.push_back(pos);
}

void HitParticle::NewParticle(const Transform& emitterTransform)
{
    EmitterParam HitParticleCircle2;
    HitParticleCircle2.maxRotate = { 0.0f, 0.0f, std::numbers::pi_v<float> };
    HitParticleCircle2.minRotate = { 0.0f, 0.0f, -std::numbers::pi_v<float> };
    HitParticleCircle2.maxScale = { 0.02f, 0.4f, 1.0f };
    HitParticleCircle2.minScale = { 0.02f, 0.2f, 1.0f };
    HitParticleCircle2.SetStartColor({ 1.0f, 1.0f, 0.5f, 1.0f });
    HitParticleCircle2.SetEndColor({ 1.0f, 1.0f, 1.0f, 0.0f });
    HitParticleCircle2.SetVelocity({ 0.0f, 0.0f, 0.0f });
    HitParticleCircle2.SetLifeTime(0.5f);
    CPUParticleManager::getInstance()->Emit("circle2", emitterTransform, 4, HitParticleCircle2);
    EmitterParam HitParticleCircle3;
    HitParticleCircle3.maxRotate = { 0.0f, 0.0f, 0.0f };
    HitParticleCircle3.minRotate = { 0.0f, 0.0f, 0.0f };
    HitParticleCircle3.SetScale({ 0.02f, 0.02f, 0.02f });
    HitParticleCircle3.maxVelocity = { 2.0f, 2.0f, 2.0f };
    HitParticleCircle3.minVelocity = { -2.0f, -2.0f, -2.0f };
    HitParticleCircle3.SetStartColor({ 1.0f, 1.0f, 0.0f, 1.0f });
    HitParticleCircle3.SetEndColor({ 1.0f, 1.0f, 0.0f, 0.0f });
    HitParticleCircle3.SetLifeTime(0.7f);
    CPUParticleManager::getInstance()->Emit("circle3", emitterTransform, 16, HitParticleCircle3);
}

void HitParticle::Update()
{
    // 試験的テスト(後で消す)
    time += 1.0f / 60.0f;
    if (3.0f < time) {
        for (auto& pos : Pos) {
            NewParticle(pos);
            time = 0.0f;
        }
    }
}

void HitParticle::Draw()
{
    // パーティクルの全体描画実行あるため空。
}
