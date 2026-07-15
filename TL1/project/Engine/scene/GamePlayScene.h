#pragma once
#include "BaseScene.h"
#include <list>
#include <memory>
#include <vector>

#include "../2d/Sprite.h"
#include "../3d/CPUParticle/ParticleEmitter.h"
#include "../audio/Sound.h"
class Model;
class Object3d;
class Skybox;

class GamePlayScene : public BaseScene {
public:
    GamePlayScene();
    ~GamePlayScene();

public:
    // 初期化
    void Initialize() override;

    // 終了
    void Finalize() override;

    // 毎フレーム更新
    void Update() override;

    // 描画
    void Draw() override;

private:
    std::unique_ptr<Skybox> skydox;

    // 3dモデル
    std::unique_ptr<Model> model;
    std::unique_ptr<Model> model2;
    std::unique_ptr<Object3d> object3d;
    std::unique_ptr<Object3d> object3d2;

    // スプライト
    std::list<std::unique_ptr<Sprite>> sprites;

    // パーティクル
    std::unique_ptr<ParticleEmitter> particleEmitter;
    std::unique_ptr<ParticleEmitter> particleEmitterPlane;
    // 音声データ
    Sound fanfare;
    Sound clearSe;
};
