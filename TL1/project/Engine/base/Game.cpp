#include "Game.h"

#include "D3dResourceLeakChecker.h"

#include "../3d/CPUParticle/CPUParticleManager.h"
#include "../3d/CPUParticle/ParticleEmitter.h"
#include "../3d/Object3dCommon.h"
#include "../io/Input.h"
#include "ImGuiManager.h"
#include "Logger.h"
#include "Math.h"
#include "SrvManager.h"

#include "../audio/Audio.h"
#include "StringUtility.h"
#include "TextureManager.h"

#include "../2d/Sprite.h"
#include "../2d/SpriteCommon.h"
#include "../3d/Model.h"
#include "../3d/ModelCommon.h"
#include "../3d/ModelManager.h"
#include "../3d/Object3d.h"

#include "../scene/BaseScene.h"
#include "../scene/SceneFactory.h"
#include "../scene/SceneManager.h"

void Game::Initialize()
{
    // 基底クラスの初期化処理
    Framework::Initialize();

    SceneManager::GetInstance()->ChangeScene("TITLE");
}

void Game::Update()
{
    Framework::GetImGuiManager()->Begin();

    // update/更新処理
    Framework::Update();

    SceneManager::GetInstance()->Update();
    PostProcess::GetInstance()->Update();

    Framework::GetImGuiManager()->End();
}

void Game::Draw()
{
    Framework::GetSrvManager()->PreDraw(); // srv表示
    Framework::GetOffScreenSurface()->PreDraw(true);
    SceneManager::GetInstance()->Draw(); // メインの描画
    Framework::GetOffScreenSurface()->PostDraw();

    Framework::GetOffScreenSurface()->TransitionDepthToShaderResource();

    // =============================================
    // ポストエフェクトのバケツリレー(Ping-Pong Buffer)
    // =============================================
    PostProcess::GetInstance()->Execute(Framework::GetOffScreenSurface(), Framework::GetOffScreenSurfaceB());

    Framework::GetOffScreenSurface()->TransitionDepthToWritable();

    Framework::GetDirectXCommon()->PreDraw();

    // 最終結果を描画
    PostProcess::GetInstance()->DrawNormal();

    // 実際のcommandListのImGuiの描画コマンドを詰む
    Framework::GetImGuiManager()->Draw();
    Framework::GetDirectXCommon()->PostDraw();
}

void Game::Finalize()
{
    Framework::Finalize();

    SceneManager::GetInstance()->Finalize();
}