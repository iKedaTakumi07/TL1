#pragma once
#include "../externals/DirectXTex/d3dx12.h"

#include "../../externals/DirectXTex/DirectXTex.h"
#include "Math.h"
#include "WinApp.h"
#include <array>
#include <cassert>
#include <chrono>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <wrl.h>

class DirectXCommon {
public:
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle() const;

    // rtvを獲得するためのやつ
    uint32_t AllocateRTVIndex()
    {
        assert(nextRtvIndex_ < kMaxRTVCount); // 16個超えたらエラー
        return nextRtvIndex_++;
    }

    // シェーダーのコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);
    // バッフアリソースの生成
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizwInBytes);
    // テクスチャリソースの生成
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
    // テクスチャデータの転送
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);

    // 初期化
    void Initialize();

    void PreDraw();

    void PostDraw();
    void FlushCommandQueue();

    // getter
    ID3D12Device* GetDevice() const
    {
        return device.Get();
    };
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
    HANDLE GetfenceEvent() { return fenceEvent; }
    ID3D12Resource* GetDepthStencilResource() { return depthStencilResource.Get(); }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
    // オフスクリーンレンダリング用テクスチャの生成（汎用化）
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, Vector4& clearColor);

    size_t GetSwapChainResourcesNum() const { return swapChainResources.size(); }

    // 最大SRV
    static const uint32_t kMaxSRVCount;

private:
    // WindowsAPI
    WinApp* winApp_ = nullptr;

    void deviceInitialize();
    // DirectX12デバイス
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    // DXGIファクトリ-
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

    void CommonInitialize();
    // コマンドキュー
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    // コマンドリストを生成する
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    // コマンドアロケータを生成する
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    void swapChainInitialize();
    // スワップチェーンを生成する
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};

    void DepthBufferInitialize();
    // Resourceの設定
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

    void DescriptorInitialize();
    uint32_t desriptorSizeSRV;
    uint32_t desriptorSizeRTV;
    uint32_t desriptorSizeDSV;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescripotrHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

    void rtvInitialize();
    // SwapChainResource
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};

    // ディスクリプタテーブル用の数
    static inline const int kMaxRTVCount = 16;
    uint32_t nextRtvIndex_ = 2;

    // RTVを二つ作るのでディスクリプタを2つ用意[更新]オフスクリーンように増やした
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[kMaxRTVCount];
    static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);
    static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

    void DepthStencilInitialize();

    void fenceInitialize();
    // 初期値0でfenceを作る
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent;

    void viewportInitialize();
    // びゅーポート
    D3D12_VIEWPORT viewport {};

    void scissorRectInitialize();
    // シザー矩形
    D3D12_RECT scissorRect {};

    void dxcCompilerInitialize();
    // dxcCompilerを初期化
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

    void ImguiInitialize();

    // FPS固定初期化
    void InitializeFixFPS();
    void UpdateFixFPS();
    // 記録時間FPS固定用
    std::chrono::steady_clock::time_point reference_;
};