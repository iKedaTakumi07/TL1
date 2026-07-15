#pragma once
#include "../../base/DirectXCommon.h"
#include <wrl.h>
#include <memory>

class Camera;

class SkyBoxCommon {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class SkyBoxCommon;
    };

    // passkeyを受け取るコンストラクタ
    explicit SkyBoxCommon(ConstructorKey) { }

    // Singleton 取得
    static SkyBoxCommon* GetInstance();

    // 初期化
    void Initialize(DirectXCommon* dxcommon);

    // 共通描画設定
    void PrepareObjectDraw();

    // get
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    Camera* GetDefaultCamera() const { return defaultCamera_; }

    // set
    void SetDefaultCamera(Camera* camera) { this->defaultCamera_ = camera; }

    // コピー禁止
    SkyBoxCommon(const SkyBoxCommon&) = delete;
    SkyBoxCommon& operator=(const SkyBoxCommon&) = delete;

private:
    friend struct std::default_delete<SkyBoxCommon>;

    // コンストラクタ・デストラクタは private
    // SkyBoxCommon() = default;
    ~SkyBoxCommon() = default;

private:
    Camera* defaultCamera_ = nullptr;

    // ルートシグネチャの作成
    void RootSignatureInitialize(DirectXCommon* dxcommon);

    // グラフィックスパイプラインの生成
    void graphicsPipelineInitialize(DirectXCommon* dxcommon);

    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
};