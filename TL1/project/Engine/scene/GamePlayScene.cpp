#include "GamePlayScene.h"
#include "SceneManager.h"

#include "../base/WinApp.h"

#include "../2d/SpriteCommon.h"
#include "../base/TextureManager.h"

#include "../3d/Model.h"
#include "../3d/ModelManager.h"
#include "../3d/Object3d.h"
#include "../3d/Object3dCommon.h"

#include "../3d/CPUParticle/ParticleEmitter.h"
#include "../3d/CPUParticle/CPUParticleManager.h"

#include "../3d/Skybox/SkyBoxCommon.h"
#include "../3d/Skybox/Skybox.h"

#include "../io/Input.h"

#include "math.h"

void GamePlayScene::Finalize()
{
    fanfare.Unload();
    clearSe.Unload();
}

GamePlayScene::GamePlayScene()
{
}

GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize()
{

    TextureManager::getInstance()->LoadTexture("resources/uvChecker.png");
    TextureManager::getInstance()->LoadTexture("resources/monsterBall.png");
    TextureManager::getInstance()->LoadTexture("resources/rostock_laage_airport_4k.dds");

    skydox = std::make_unique<Skybox>();
    skydox->Initialize("resources/rostock_laage_airport_4k.dds");

    for (uint32_t i = 0; i < 1; ++i) {
        auto sprite = std::make_unique<Sprite>();

        if (i % 2 == 0) {
            sprite->Initialize("resources/uvChecker.png");
            sprite->SetPosition(Vector2(100.0f, 100.0f));
        } else {
            sprite->Initialize("resources/monsterBall.png");
        }

        sprites.push_back(std::move(sprite));
    }

    ModelManager::GetInstance()->LoadModel("Plane.obj");
    ModelManager::GetInstance()->LoadModel("axis.obj");

    object3d = std::make_unique<Object3d>();
    object3d->Initialize();
    object3d->SetCamera(BaseScene::GetCamera());

    model = std::make_unique<Model>();
    model->Initialize("resources", "plane.obj");
    model->SetEvnTexturefilePath(skydox->GetTextureFilePath());
    object3d->SetModel(model.get());

    object3d2 = std::make_unique<Object3d>();
    object3d2->Initialize();
    object3d2->SetCamera(BaseScene::GetCamera());

    model2 = std::make_unique<Model>();
    model2->Initialize("resources", "axis.obj");
    model2->SetEvnTexturefilePath(skydox->GetTextureFilePath());
    object3d2->SetModel(model2.get());

    CPUParticleManager::getInstance()->CreateParticleGroup("pori", "resources/circle.png", ParticleMeshType::kPlane);
    CPUParticleManager::getInstance()->CreateParticleGroup("Plane", "resources/uvChecker.png", ParticleMeshType::kPlane);

    // 板ポリ
    Transform emitter {};
    emitter.translate = { 0.0f, 0.0f, 0.0f };
    emitter.rotate = { 0.0f, 0.0f, 0.0f };
    emitter.scale = { 1.0f, 1.0f, 1.0f };
    // particleEmitter = std::make_unique<ParticleEmitter>("pori", emitter, 1.0f, 3,true);

    Transform emitterPlane {};
    emitterPlane.translate = { 4.0f, 4.0f, 0.0f };
    emitterPlane.rotate = { 0.0f, 0.0f, 0.0f };
    emitterPlane.scale = { 1.0f, 1.0f, 1.0f };
    particleEmitterPlane = std::make_unique<ParticleEmitter>("Plane", emitterPlane, 1.0f, 5, true);

    fanfare.SoundLoadFile("resources/fanfare.wav");
    clearSe.SoundLoadFile("resources/stage.mp3");

    // 音がうるさいので停止中
    // Audio::GetInstance()->Play(fanfare);
    // Audio::GetInstance()->Play(clearSe);
}

void GamePlayScene::Update()
{
    Input* input = GetInput();
    Camera* camera = GetCamera();

    if (input->TriggerKey(DIK_F1)) {
        SceneManager::GetInstance()->ChangeScene("TITLE");
    }

    for (auto& sprite : sprites) {
        sprite->Update();
    }

    skydox->SetCamera(camera);
    skydox->Update();

    object3d->Update();

    object3d2->Update();
    Vector3 rotate2 = object3d2->GetRotate();

    object3d2->SetRotate(rotate2);

    // particleEmitter->Update();
    particleEmitterPlane->Update();
}

void GamePlayScene::Draw()
{

    Object3dCommon::GetInstance()->PrepareObjectDraw();

    //
    // モデルデータ
    //
    object3d->Draw();
    object3d2->Draw();

    SkyBoxCommon::GetInstance()->PrepareObjectDraw();
    // skydox->Draw();

    //
    // 2d/スプライト
    //
    SpriteCommon::GetInstance()->PrepareSpriteDraw();

    for (auto& sprite : sprites) {
        sprite->Draw();
    }

    CPUParticleManager::getInstance()->Draw();
}