#include "ImGuiManager.h"
#include "WinApp.h"

#include "DirectXCommon.h"
#include "SrvManager.h"

void ImGuiManager::Initialize([[maybe_unused]] DirectXCommon* dxCommon, [[maybe_unused]] SrvManager* srvManager)
{
#ifdef USE_IMGUI

    winApp_ = WinApp::GetInstance();
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    // IMGUIのコンテキスト生成
    ImGui::CreateContext();

    // IMGUIのスタイルを設定
    ImGui::StyleColorsDark();

    // win32初期化
    ImGui_ImplWin32_Init(winApp_->GetHwnd());

    // dx12の初期化情報
    ImGui_ImplDX12_InitInfo initInfo = { };

    initInfo.Device = dxCommon_->GetDevice(); // dx12デバイス
    initInfo.CommandQueue = dxCommon->GetCommandQueue(); // dx12コマンドキュー
    initInfo.NumFramesInFlight = static_cast<int>(dxCommon_->GetSwapChainResourcesNum()); // バックバッファの数
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // RTVのフォーマット
    initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // DSVのフォーマット
    initInfo.SrvDescriptorHeap = srvManager_->GetdescriptorHeap().Get(); // SRVデスクリプタヒープ

    initInfo.UserData = srvManager_;

    // SRV確保用関数の設定
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
        SrvManager* srvManager = static_cast<SrvManager*>(info->UserData);
        uint32_t index = srvManager->Allocate(); // srvManagerからSRV確保
        *out_cpu_handle = srvManager->GetCPUDescriptorHandle(index); // インデックスに対応したCPUハンドル
        *out_gpu_handle = srvManager->GetGPUDescriptorHandle(index); // インデックスに対応したgPUハンドル
    };

    // SRV解放用関数の設定
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_handle) {
        // SrvMnagerに解放機能を作っていないため、ここでは何もしない。
        (void)info;
        (void)out_cpu_handle;
        (void)out_gpu_handle;
    };

    // 初期化実行
    ImGui_ImplDX12_Init(&initInfo);

#endif // USE_IMGUI
}

void ImGuiManager::Begin()
{
#ifdef USE_IMGUI
    // ImGuiフレーム開始
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
#endif // USE_IMGUI
}

void ImGuiManager::End()
{
#ifdef USE_IMGUI
    // 描画前準備
    ImGui::Render();
#endif // USE_IMGUI
}

void ImGuiManager::Draw()
{
#ifdef USE_IMGUI
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // デスクリプタヒープの配列をセットコマンドするコマンド
    ID3D12DescriptorHeap* ppHeaps[] = { srvManager_->GetdescriptorHeap().Get() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    // 描画コマンド
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif // USE_IMGUI
}

void ImGuiManager::Finalize()
{
#ifdef USE_IMGUI
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif // USE_IMGUI
}