#pragma once
#include "../base/DirectXCommon.h"
#include "wrl.h"
#include <memory>

class SpriteCommon {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class SpriteCommon;
    };

    // passkeyを受け取るコンストラクタ
    explicit SpriteCommon(ConstructorKey) { }
    // Singleton 取得
    static SpriteCommon* GetInstance();

    // 初期化
    void Initialize(DirectXCommon* dxcommon);

    DirectXCommon* GetDxCommon() const { return dxCommon_; }

    // 共通描画設定
    void PrepareSpriteDraw();

    SpriteCommon(const SpriteCommon&) = delete;
    SpriteCommon& operator=(const SpriteCommon&) = delete;

private:
    friend struct std::default_delete<SpriteCommon>;

    // SpriteCommon() = default;
    ~SpriteCommon() = default;

private:
    // ルートシグネチャの作成
    void RootSignatureInitialize(DirectXCommon* dxcommon);

    // グラフィックスパイプラインの生成
    void graphicsPipelineInitialize(DirectXCommon* dxcommon);

    DirectXCommon* dxCommon_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
};
