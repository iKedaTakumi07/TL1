#pragma once
#include "../../externals/DirectXTex/DirectXTex.h"
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl.h>

class DirectXCommon;
class SrvManager;

class OffscreenSurface {
public:
    // 初期化
    void Initialize(DirectXCommon* dxcommon, SrvManager* srvManager, uint32_t rtvIndex);

    // ハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() { return srvHandleGPU; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSRVHandle() const { return depthSrvHandleGPU; }

    // 引数を追加して、シーン描画とポストプロセス描画を区別できるようにする
    void PreDraw(bool isSceneDraw = false); // 描画先をこのテクスチャに切り替える
    void PostDraw(); // 描画を終了し、テクスチャとして使える状態にする

    void TransitionDepthToShaderResource();
    void TransitionDepthToWritable();

private:
    // メンバ変数として保持しておく必要があるもの
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;

    uint32_t rtvIndex;
    uint32_t srvIndex;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;

    uint32_t depthSrvIndex;
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandleGPU;
};
