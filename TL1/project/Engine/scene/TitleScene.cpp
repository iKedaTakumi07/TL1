#include "TitleScene.h"
#include "SceneManager.h"

#include "../base/PostProcess.h"

#include <random>

#include "../3d/Camera.h"
#include "../base/WinApp.h"

#include "../2d/SpriteCommon.h"
#include "../base/TextureManager.h"

#include "../3d/Model.h"
#include "../3d/ModelManager.h"
#include "../3d/Object3d.h"
#include "../3d/Object3dCommon.h"

#include "../3d/Skybox/SkyBoxCommon.h"
#include "../3d/Skybox/Skybox.h"

#include "../3d/CPUParticle/CPUParticleManager.h"
#include "../3d/CPUParticle/ParticleEmitter.h"
#include "../3d/GPUParticleManager.h"

#include "../io/Input.h"

#include "../../Game/Particle/HitParticle.h"
#include "../../Game/Particle/LaserParticle.h"
#include "math.h"

TitleScene::TitleScene()
{
}

TitleScene::~TitleScene() = default;

void TitleScene::Initialize()
{
    TextureManager::getInstance()->LoadTexture("resources/rostock_laage_airport_4k.dds");
    TextureManager::getInstance()->LoadTexture("resources/uvChecker.png");
    TextureManager::getInstance()->LoadTexture("resources/grass.png");
    TextureManager::getInstance()->LoadTexture("resources/AnimatedCube_BaseColor.png");
    TextureManager::getInstance()->LoadTexture("resources/AnimatedCube_MetallicRoughness.png");
    TextureManager::getInstance()->LoadTexture("resources/simpleSkin/uvChecker.png");
    TextureManager::getInstance()->LoadTexture("resources/human/white.png");

    ModelManager::GetInstance()->LoadModel("axis.obj");
    ModelManager::GetInstance()->LoadModel("terrain.obj");
    ModelManager::GetInstance()->LoadModel("plane.gltf");
    ModelManager::GetInstance()->LoadModel("AnimatedCube.gltf");
    ModelManager::GetInstance()->LoadModel("simpleSkin/simpleSkin.gltf");
    ModelManager::GetInstance()->LoadModel("human/walk.gltf");
    ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");

    skydox = std::make_unique<Skybox>();
    // skydox->Initialize("resources/rostock_laage_airport_4k.dds");

    object3d = std::make_unique<Object3d>();
    object3d->Initialize();
    object3d->SetCamera(BaseScene::GetCamera());

    model = std::make_unique<Model>();
    model->Initialize("resources", "terrain.obj");
    // model->SetEvnTexturefilePath(skydox->GetTextureFilePath());
    object3d->SetModel(model.get());

    object3d_2 = std::make_unique<Object3d>();
    object3d_2->Initialize();
    object3d_2->SetCamera(BaseScene::GetCamera());

    model_2 = std::make_unique<Model>();
    model_2->Initialize("resources", "human/walk.gltf");
    object3d_2->SetScale({ 100.0f, 100.0f, 100.0f });
    object3d_2->SetRotate({ -1.5f, 3.14f, 0.0f });
    // model_2->SetEvnTexturefilePath(skydox->GetTextureFilePath());
    object3d_2->SetModel(model_2.get()); // スケルトンもセットで構築
    object3d_2->LoadAnimation("resources", "human/walk.gltf", "walk");
    object3d_2->LoadAnimation("resources", "human/sneakWalk.gltf", "sneakWalk");
    object3d_2->PlayAnimation("sneakWalk", true);

    CPUParticleManager::getInstance()->CreateParticleGroup("pori", "resources/circle2.png", ParticleMeshType::kPlane);
    CPUParticleManager::getInstance()->CreateParticleGroup("circle3", "resources/circle3.png", ParticleMeshType::kPlane);
    CPUParticleManager::getInstance()->CreateParticleGroup("Plane", "resources/uvChecker.png", ParticleMeshType::kPlane);
    CPUParticleManager::getInstance()->CreateParticleGroup("gradationLine", "resources/gradationLine.png", ParticleMeshType::kRing);
    CPUParticleManager::getInstance()->CreateParticleGroup("Cylinder", "resources/gradationLine.png", ParticleMeshType::kCylinder);

    // ポストエフェクトのON/OFFならこれ。
    PostProcess::GetInstance()->SetEnableBoxFilter(false);
    PostProcess::GetInstance()->SetKernelSizeBoxFilter(7);

    // 板ポリ
    Transform emitter { };
    emitter.translate = { 0.0f, 2.0f, 0.0f };
    emitter.rotate = { 0.0f, 0.0f, 1.0f };
    emitter.scale = { 0.05f, 1.0f, 1.0f };
    EmitterParam fireParam;

    // particleEmitter2 = std::make_unique<ParticleEmitter>("gradationLine", emitter, 0.8f, 3, true);
    // fireParam.maxRotate = { std::numbers::pi_v<float>, std::numbers::pi_v<float>, 0.0f };
    // fireParam.minRotate = { -std::numbers::pi_v<float>, -std::numbers::pi_v<float>, 0.0f };
    // fireParam.maxScale = { 1.0f, 1.0f, 1.0f };
    // fireParam.minScale = { 1.0f, 0.4f, 1.0f };
    // fireParam.SetStartColor({ 1.0f, 1.0f, 0.5f, 1.0f });
    // fireParam.SetEndColor({ 1.0f, 1.0f, 1.0f, 0.0f });
    // fireParam.SetVelocity({ 0.0f, 0.0f, 0.0f });
    // fireParam.SetLifeTime(1.2f);
    // particleEmitter2->SetParam(fireParam);

    // [アップデート予定]パーティクルPS閾値をCBuffer経由で設定できるようにする
    /*emitter.translate = { 0.0f, 0.0f, 0.0f };
    particleEmitter4 = std::make_unique<ParticleEmitter>("Cylinder", emitter, 10.0f, 1, false);
    fireParam.SetScale({ 1.0f, 1.0f, 1.0f });
    fireParam.SetRotate({ 0.0f, 0.0f, 0.0f });
    fireParam.SetStartColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    fireParam.SetEndColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    fireParam.SetVelocity({ 0.0f, 0.0f, 0.0f });
    fireParam.SetLifeTime(10.0f);
    fireParam.isInfinite = true;
    particleEmitter4->SetParam(fireParam);
    ParticleManager::getInstance()->SetGroupScrollSpeed("Cylinder", { 0.2f, 0.0f });*/

    laserTest = std::make_unique<LaserParticle>();
    laserTest->Initialize();
    laserTest->NewTransform();

    hitParticle = std::make_unique<HitParticle>();
    hitParticle->Initialize();
    hitParticle->NewTransform();
}

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{

    Input* input = GetInput();
    Camera* camera = GetCamera();

    // skydox->SetCamera(camera);

    if (input->TriggerKey(DIK_F1)) {
        SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
    }

    // skydox->Update();

    object3d->Update();
    object3d_2->Update();

    // particleEmitter2->Update();
    // particleEmitter4->Update();

    // 5秒ごとに生成(軽いテストなう)
    if (testTimer >= 5.0f) {
        testTimer = 0.0f;
        laserTest->NewTransform();
    }
    float deltaTime = SceneManager::GetInstance()->GetDeltaTime();
    testTimer += deltaTime;

    laserTest->Update();
    hitParticle->Update();

    // IMGUI
    object3d->DrawImGui("Terrain");
    object3d_2->DrawImGui("Plane");
}

void TitleScene::Draw()
{
    //
    // モデルデータ
    //
    object3d->Draw();
    object3d_2->Draw();

#ifdef USE_IMGUI
    Object3dCommon::GetInstance()->PreLineObjectDraw();
    object3d_2->DrawSkeleton();
#endif // USE_IMGUI

    SkyBoxCommon::GetInstance()->PrepareObjectDraw();
    // skydox->Draw();

    SpriteCommon::GetInstance()->PrepareSpriteDraw();

    CPUParticleManager::getInstance()->Draw();

    GPUParticleManager::getInstance()->Draw();
}
