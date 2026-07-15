#pragma once
#include "../base/Math.h"
#include "../externals/DirectXTex/DirectXTex.h"
#include <assimp/scene.h>
#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

class WinApp;
class Object3dCommon;
#include "Animator.h"
#include "Model.h"
#include <optional>
#include <vector>
class Camera;

class Object3d {
public:
    Matrix4x4 MakeIdentity4x4()
    {
        Matrix4x4 num;
        num = { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
        return num;
    }

public:
    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
    static Node ReadNode(aiNode* node);

    // 初期化
    void Initialize();

    void Update();

    void DrawImGui(const std::string& label);

#ifdef USE_IMGUI
    // 内部のアニメーターに骨の描画を依頼する
    void DrawSkeleton();
#endif // USE_IMGUI

    // 更新
    void Draw();

    // Setter
    void SetModel(Model* model);
    void SetScale(const Vector3& scale) { transform.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform.translate = translate; }
    void SetModel(const std::string& filePath);
    void SetCamera(Camera* camera) { this->camera = camera; }

    void SetEnableLighting(bool enable)
    {
        if (model && model->GetmaterialData()) {
            model->GetmaterialData()->enableEnvironmentMap = enable ? 1 : 0;
        }
    }
    void SetEnvironmentCoefficient(float coefficient)
    {
        if (model && model->GetmaterialData()) {
            model->GetmaterialData()->evnironmentCoefficient = coefficient;
            model->SetMaterialDataEvnironmentCoefficient(coefficient);
        }
    }

    // アニメーション命令
    void LoadAnimation(const std::string& dir, const std::string& file, const std::string& name)
    {
        if (animator_)
            animator_->LoadAnimation(dir, file, name);
    }
    void PlayAnimation(const std::string& name, bool loop = true)
    {
        if (animator_)
            animator_->PlayAnimation(name, loop);
    }
    void StopAnimation()
    {
        if (animator_)
            animator_->StopAnimation();
    }

    // 外部からボーン情報を覗きたい時用のゲッター
    Animator* GetAnimator() const { return animator_.get(); }

    // Getter
    const Vector3& GetScale() const { return transform.scale; }
    const Vector3& GetRotate() const { return transform.rotate; }
    const Vector3& GetTranslate() const { return transform.translate; }

private:
    void TransMatrixResourceInitialize();
    void cameraDataResourceInitialize();

    Object3dCommon* object3dCommon = nullptr;
    WinApp* winApp_ = nullptr;
    Model* model = nullptr;
    Camera* camera = nullptr;

    // バッファリソース
    TransformationMatrix* transformationMatrixData;
    // model用のtransformMatrix用のリソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;

    Microsoft::WRL::ComPtr<ID3D12Resource> CameraDataResourceModel;
    CameraForGPU* CameraForGPUData = nullptr;

    // アニメーションこれ一本
    std::unique_ptr<Animator> animator_ = nullptr;

    Transform transform = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    Transform cameraTransform;
};
