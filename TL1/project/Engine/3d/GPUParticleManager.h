#pragma once
#include "../base/Math.h"
#include <d3d12.h>
#include <list>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <wrl.h>

class DirectXCommon;
class SrvManager;
class Camera;
class WinApp;

class GPUParticleManager {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class GPUParticleManager;
    };

    // passkeyを受け取るコンストラクタ
    explicit GPUParticleManager(ConstructorKey) { }

    static GPUParticleManager* getInstance();

    // 初期化
    void Initialize(DirectXCommon* DirectXCollision, SrvManager* srvManager);

    void Update();

    void Draw();

    // set
    void SetDefaultCamera(Camera* camera) { this->Camera_ = camera; }

    // CSInitialize
    void PrepareCSInitialize();
    void CSInitialize();

    // CSUpdate
    void PrepareCSUpdate();
    void CSUpdate();

    GPUParticleManager(GPUParticleManager&) = delete;
    GPUParticleManager& operator=(GPUParticleManager&) = delete;

private:
    friend struct std::default_delete<GPUParticleManager>;
    ~GPUParticleManager() = default;

private:
    void RootSignatureInitialize(DirectXCommon* dxcommon);
    void graphicsPipelineInitialize(DirectXCommon* dxcommon);

    // CS初期化用パイプライン
    void CSRootSignatureInitialize(DirectXCommon* dxcommon);
    void CSPipelineInitialize(DirectXCommon* dxcommon);

    // CS更新用パイプライン
    void CSUpdateRootSignatureInitialize(DirectXCommon* dxcommon);
    void CSUpdatePipelineInitialize(DirectXCommon* dxcommon);

    void ResourceInitialize();

private:
    SrvManager* srvManager = nullptr;
    DirectXCommon* dxCommon = nullptr;
    WinApp* winApp_ = nullptr;

    Camera* Camera_ = nullptr;

    // 各リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource;

    uint32_t instancingSrvIndex = 0;
    uint32_t textureSrvIndex = 0;

    // IA
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView { };
    VertexData* vertexData = nullptr;

    // Sphere
    Microsoft::WRL::ComPtr<ID3D12Resource> SphereResource;
    EmitterSphere* eitterSphere = nullptr;

    // VS/PS
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

    // 初期化用CS
    Microsoft::WRL::ComPtr<ID3D12RootSignature> csInitRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> csInitPipelineState;

    // 更新用CS
    Microsoft::WRL::ComPtr<ID3D12RootSignature> csUpdateRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> csUpdatePipelineState;

    //  パーティクル構造体
    Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;

    // カウンター
    Microsoft::WRL::ComPtr<ID3D12Resource> freeCounterResource;

    // random
    Microsoft::WRL::ComPtr<ID3D12Resource> randomResource;
    PreFrame* PreFrameData_ = nullptr;

    static std::unique_ptr<GPUParticleManager> instance;
    int kMaxParticles = 1024;
};
