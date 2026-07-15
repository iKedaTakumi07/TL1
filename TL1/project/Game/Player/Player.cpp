#include "Player.h"
#include "../../Engine/3d/ModelManager.h"
#include "../../Engine/3d/Object3d.h"
#include "../../Engine/base/TextureManager.h"

void Player::Initialize(Camera* camaer)
{
    TextureManager::getInstance()->LoadTexture("resources/1x1white.png");
    ModelManager::GetInstance()->LoadModel("Player.obj");

    object3d = std::make_unique<Object3d>();
    object3d->Initialize();
    object3d->SetCamera(camaer);

    model = std::make_unique<Model>();
    model->Initialize("resources", "terrain.obj");
    // model->SetEvnTexturefilePath(skydox->GetTextureFilePath());
    object3d->SetModel(model.get());
}

void Player::Update()
{
    object3d->Update();
    object3d->DrawImGui("Player");
}

void Player::Draw()
{
    object3d->Draw();
}
