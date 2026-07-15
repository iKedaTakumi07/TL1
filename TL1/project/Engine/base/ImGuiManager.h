#pragma once
#include <stdint.h>

#ifdef USE_IMGUI
#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"
#endif // USE_IMGUI

class WinApp;
class DirectXCommon;
class SrvManager;

class ImGuiManager {
public:
    // 初期化
    void Initialize( DirectXCommon* dxCommon, SrvManager* srvManager);

    // IMGUI受付開始
    void Begin();

    // IMGUI受付終了
    void End();

    // 画面絵の描画
    void Draw();

    // 終了
    void Finalize();

private:
    // WindowsAPI
    WinApp* winApp_ = nullptr;

    DirectXCommon* dxCommon_;

    SrvManager* srvManager_;

    uint32_t SrvIndex;
};
