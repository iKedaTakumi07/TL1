#pragma once
#include "AbstractSceneFactory.h"
#include <memory>

#include <chrono>
class BaseScene;

class SceneManager {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class SceneManager;
    };

    // passkeyを受け取るコンストラクタ
    explicit SceneManager(ConstructorKey);

    // Singleton 取得
    static SceneManager* GetInstance();

    // 終了
    void Finalize();

    // 更新
    void Update();

    // 描画
    void Draw();

    // get
    float GetDeltaTime() const { return deltaTime_; }

public:
    /// <summary>
    /// 次シーン予約
    /// </summary>
    /// <param name="sceneName"></param>
    void ChangeScene(const std::string& sceneName);

    // 次のシーン予約
    void SetNextScene(std::unique_ptr<BaseScene> nextScene) { nextScene_ = std::move(nextScene); }
    void SetSceneFactory(std::unique_ptr<AbstractSceneFactory> SceneFactory) { sceneFactory_ = std::move(SceneFactory); }

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

private:
    friend struct std::default_delete<SceneManager>;
    ~SceneManager() = default;

private:
    std::unique_ptr<BaseScene> nextScene_ = nullptr;
    std::unique_ptr<BaseScene> scene_ = nullptr;
    std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;

    std::chrono::steady_clock::time_point lastTime_; // 前フレームの時刻
    float deltaTime_ = 0.0f; // 前フレームからの経過時間（秒）
};
