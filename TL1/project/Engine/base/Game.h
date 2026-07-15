#pragma once
#include <vector>

#include "../audio/Sound.h"
#include "../scene/SceneManager.h"
#include "Framework.h"

class Sprite;
class Object3d;
class Model;

class GamePlayScene;

class ParticleEmitter;

class Game : public Framework {
public:
    // 初期化
    void Initialize() override;

    // 更新
    void Update() override;

    // 描画
    void Draw() override;

    // 終了
    void Finalize() override;

private:
    ///
    /// その他
};
