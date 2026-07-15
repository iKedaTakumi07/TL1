#pragma once
#include "../base/DirectXCommon.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>

class LightManager {
public:
    static LightManager* GetInstance();

    void Initialize();
    void Update();
    void DrawImGui();

    // GPUに渡す用のアドレス取得関数
    D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightAddress() const { return directionalLightResource_->GetGPUVirtualAddress(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetPointLightAddress() const { return pointLightResource_->GetGPUVirtualAddress(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightAddress() const { return spotLightResource_->GetGPUVirtualAddress(); }

    // コピー禁止
    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    // Set
    void SetDirectionalLight(const DirectionalLight& light)
    {
        if (directionalLightData_) {
            *directionalLightData_ = light;
        }
    }

    void SetPointLight(const PointLigth& light)
    {
        if (pointLightData_) {
            *pointLightData_ = light;
        }
    }

    void SetSpotLight(const SpotLigth& light)
    {
        if (spotLightData_) {
            *spotLightData_ = light;
        }
    }

private:
    LightManager() = default;
    ~LightManager() = default;

private:
    void directionalLightInitialize();
    void pointLightInitialize();
    void spotLightInitialize();

    // Directional Light
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;

    // Point Light
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLigth* pointLightData_ = nullptr;

    // Spot Light
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    SpotLigth* spotLightData_ = nullptr;
};