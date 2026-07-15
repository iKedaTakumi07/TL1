#pragma once
#include "../../Engine/3d/Camera.h"
#include "../../Engine/3d/Model.h"
#include "../../Engine/3d/Object3d.h"
#include "../../Engine/base/Math.h"
#include <memory>

class Player {
public:
    void Initialize(Camera* camaer);

    void Update();

    void Draw();

private:
    Transform transform_; // 座標

    // 3dモデル
    std::unique_ptr<Model> model;
    std::unique_ptr<Object3d> object3d;
};
