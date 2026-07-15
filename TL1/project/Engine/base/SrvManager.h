#pragma once
#include <d3d12.h>
#include <stdint.h>

#include <chrono>
#include <dxgi1_6.h>
#include <wrl.h>
#include "../../externals/DirectXTex/DirectXTex.h"

class DirectXCommon;

class SrvManager {
public:
    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    uint32_t Allocate();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

    void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DirectX::TexMetadata metadata, UINT MipLevels);
    void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

    void PreDraw();

    void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

    // getter
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetdescriptorHeap() const { return descriptorHeap; };

private:
    DirectXCommon* directXCommon = nullptr;

    // 次使用するやつ
    uint32_t useIndex = 0;

    // 最大SRV数
    static const uint32_t kMaxSRVCount;
    // SRV用のデスクリプタサイズ
    uint32_t descriptorSize;
    // SRV用デスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
};
