#include "SceneManager.h"
#include "BaseScene.h"
#include <assert.h>

SceneManager::SceneManager(ConstructorKey)
    : lastTime_(std::chrono::steady_clock::now())
    , deltaTime_(0.0f)
{
}

SceneManager* SceneManager::GetInstance()
{
    static std::unique_ptr<SceneManager> instance = std::make_unique<SceneManager>(ConstructorKey());

    return instance.get();
}

void SceneManager::Finalize()
{
    if (scene_) {
        scene_->Finalize();
        scene_.reset();
    }
}

void SceneManager::Update()
{
    auto now = std::chrono::steady_clock::now(); // 1フレームの経過時間を取得
    std::chrono::duration<float> elapsed = now - lastTime_;
    lastTime_ = now;

    deltaTime_ = elapsed.count();

    // 最大値設定
    if (deltaTime_ > 0.1f) {
        deltaTime_ = 0.1f;
    }

    if (nextScene_) {
        if (scene_) {
            scene_->Finalize();
        }

        scene_ = std::move(nextScene_);
        scene_->SetSceneManager(this);

        // シーンの初期化
        scene_->Initialize();
    }

    scene_->Update();
}

void SceneManager::Draw()
{
    scene_->Draw();
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
    assert(sceneFactory_);
    assert(nextScene_ == nullptr);

    // 次シーンを生成。
    nextScene_ = sceneFactory_->CreateScene(sceneName);
}
