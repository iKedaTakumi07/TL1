#include "GamePlayScene.h"
#include "SceneManager.h"

#include "../base/WinApp.h"

#include "../2d/SpriteCommon.h"
#include "../base/LevelLoader.h"
#include "../base/TextureManager.h"

#include "../3d/Model.h"
#include "../3d/ModelManager.h"
#include "../3d/Object3d.h"
#include "../3d/Object3dCommon.h"

#include "../3d/CPUParticle/CPUParticleManager.h"
#include "../3d/CPUParticle/ParticleEmitter.h"

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
    TextureManager::getInstance()->LoadTexture("resources/scene/1x1white.png");
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
    ModelManager::GetInstance()->LoadModel("scene/sceneBuild_A.obj");
    ModelManager::GetInstance()->LoadModel("scene/sceneBuild_B.obj");
    ModelManager::GetInstance()->LoadModel("scene/sceneBuild_C.obj");
    ModelManager::GetInstance()->LoadModel("scene/sceneBuild_D.obj");
    ModelManager::GetInstance()->LoadModel("scene/sceneGround.obj");

    modelA = std::make_unique<Model>();
    modelA->Initialize("resources/scene", "sceneBuild_A.obj");
    modelA->SetEvnTexturefilePath(skydox->GetTextureFilePath());

    modelB = std::make_unique<Model>();
    modelB->Initialize("resources/scene", "sceneBuild_B.obj");
    modelB->SetEvnTexturefilePath(skydox->GetTextureFilePath());

    modelC = std::make_unique<Model>();
    modelC->Initialize("resources/scene", "sceneBuild_C.obj");
    modelC->SetEvnTexturefilePath(skydox->GetTextureFilePath());

    modelD = std::make_unique<Model>();
    modelD->Initialize("resources/scene", "sceneBuild_D.obj");
    modelD->SetEvnTexturefilePath(skydox->GetTextureFilePath());

    modelG = std::make_unique<Model>();
    modelG->Initialize("resources/scene", "sceneGround.obj");
    modelG->SetEvnTexturefilePath(skydox->GetTextureFilePath());

    LevelData* levelData = LevelLoader::Load("resources/scene/", "scene", ".json");
    if (levelData) {
        for (auto& objectData : levelData->objects) {

            Model* targetModel = nullptr;

            // 名前（ファイル名）に応じて割り当てるモデルを決定
            if (objectData.fileName == "BuildA") {
                targetModel = modelA.get();
            } else if (objectData.fileName == "BuildB") {
                targetModel = modelB.get();
            } else if (objectData.fileName == "BuildC") {
                targetModel = modelD.get();
            } else if (objectData.fileName == "BuildD") {
                targetModel = modelC.get();
            } else if (objectData.fileName == "Ground") {
                targetModel = modelG.get();
            }

            // モデルが確定したらオブジェクトを生成
            if (targetModel) {
                auto staticObj = std::make_unique<Object3d>();
                staticObj->Initialize();
                staticObj->SetCamera(BaseScene::GetCamera());
                staticObj->SetModel(targetModel);
                staticObj->SetRotate(objectData.rotation);
                staticObj->SetScale(objectData.scaling);
                staticObj->SetTranslate(objectData.translation);

                staticObjects_.push_back(std::move(staticObj));
            }
        }

        delete levelData; // メモリ解放
    }

    // パーティクル
    CPUParticleManager::getInstance()->CreateParticleGroup("pori", "resources/circle.png", ParticleMeshType::kPlane);
    CPUParticleManager::getInstance()->CreateParticleGroup("Plane", "resources/uvChecker.png", ParticleMeshType::kPlane);

    // 板ポリ
    Transform emitter { };
    emitter.translate = { 0.0f, 0.0f, 0.0f };
    emitter.rotate = { 0.0f, 0.0f, 0.0f };
    emitter.scale = { 1.0f, 1.0f, 1.0f };
    // particleEmitter = std::make_unique<ParticleEmitter>("pori", emitter, 1.0f, 3,true);

    Transform emitterPlane { };
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

    for (auto& object : staticObjects_) {
        object->Update();
    }

    // particleEmitter->Update();
    particleEmitterPlane->Update();
}

void GamePlayScene::Draw()
{

    Object3dCommon::GetInstance()->PrepareObjectDraw();

    //
    // モデルデータ
    //

    for (auto& object : staticObjects_) {
        object->Draw();
    }

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