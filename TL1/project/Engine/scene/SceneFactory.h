#pragma once
#include "AbstractSceneFactory.h"

class Input;
class Camera;

class SceneFactory : public AbstractSceneFactory {
public:
    SceneFactory(Input* input, Camera* camera);

    /// <summary>
    /// シーン生成。
    /// </summary>
    /// <param name="sceneName">シーン名</param>
    /// <returns></returns>
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;

private:
    Input* input_ = nullptr;
    Camera* camera_ = nullptr;
};
