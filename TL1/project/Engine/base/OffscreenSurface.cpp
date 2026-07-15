#include "OffscreenSurface.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"

void OffscreenSurface::Initialize(DirectXCommon* dxcommon, SrvManager* srvManager, uint32_t rtvIndex)
{
    dxCommon_ = dxcommon;
    srvManager_ = srvManager;
    this->rtvIndex = rtvIndex;

    // テクスチャリソース作成
    Vector4 clearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
    resource = dxcommon->CreateRenderTextureResource(WinApp::KClientWidth, WinApp::KClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clearColor);

    // RTVの作成
    rtvHandleCPU = dxcommon->GetRTVCPUDescriptorHandle(rtvIndex);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    dxCommon_->GetDevice()->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandleCPU);

    // SRVの作成
    srvIndex = srvManager_->Allocate();
    srvHandleGPU = srvManager_->GetGPUDescriptorHandle(srvIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    dxCommon_->GetDevice()->CreateShaderResourceView(resource.Get(), &srvDesc, srvManager_->GetCPUDescriptorHandle(srvIndex));

    // 深度用のSRVインデックスをSrvManagerから新しく確保
    depthSrvIndex = srvManager_->Allocate();
    depthSrvHandleGPU = srvManager_->GetGPUDescriptorHandle(depthSrvIndex);

    ID3D12Resource* depthResource = dxCommon_->GetDepthStencilResource();

    // depth用のSRVを作る
    D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc {};
    depthTextureSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthTextureSrvDesc.Texture2D.MipLevels = 1;

    dxCommon_->GetDevice()->CreateShaderResourceView(depthResource, &depthTextureSrvDesc, srvManager_->GetCPUDescriptorHandle(depthSrvIndex));

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource.Get();
    // 作成直後は RENDER_TARGET なので、それを PIXEL_SHADER_RESOURCE に変えておく
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    commandList->ResourceBarrier(1, &barrier);

    // コマンドを確定させて実行し、完了を待つ（これでステートが PSR で確定する）
    dxCommon_->FlushCommandQueue();
}

void OffscreenSurface::PreDraw(bool isSceneDraw)
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // バリア
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // メインシーンの描画時のみ深度バッファを結合してクリアする
    if (isSceneDraw) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle();
        commandList->OMSetRenderTargets(1, &rtvHandleCPU, false, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // カラーのクリア
        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandleCPU, clearColor, 0, nullptr);
    } else {
        // ポストプロセス中は深度バッファを使わない(nullptr)
        commandList->OMSetRenderTargets(1, &rtvHandleCPU, false, nullptr);
    }

    // ビューポート・シザーの設定
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)WinApp::KClientWidth, (float)WinApp::KClientHeight, 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, WinApp::KClientWidth, WinApp::KClientHeight };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void OffscreenSurface::PostDraw()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // バリア
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}

void OffscreenSurface::TransitionDepthToShaderResource()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    D3D12_RESOURCE_BARRIER depthBarrier {};
    depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    depthBarrier.Transition.pResource = dxCommon_->GetDepthStencilResource();
    depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &depthBarrier);
}

void OffscreenSurface::TransitionDepthToWritable()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    D3D12_RESOURCE_BARRIER depthReturnBarrier {};
    depthReturnBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    depthReturnBarrier.Transition.pResource = dxCommon_->GetDepthStencilResource();
    depthReturnBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    depthReturnBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    commandList->ResourceBarrier(1, &depthReturnBarrier);
}
