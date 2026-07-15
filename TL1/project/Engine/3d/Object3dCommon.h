#pragma once
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include <memory>
class Camera;

class Object3dCommon {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class Object3dCommon;
    };

    // passkeyを受け取るコンストラクタ
    explicit Object3dCommon(ConstructorKey) { }

    // Singleton 取得
    static Object3dCommon* GetInstance();

    // 初期化
    void Initialize(DirectXCommon* dxcommon, SrvManager* srvManager);

    // 共通描画設定
    void PrepareObjectDraw();
    // スキンモデル用
    void PrepareSkinObjectDraw();
    // CS
    void PrepareCSObjectDraw();
#ifdef USE_IMGUI
    void PreLineObjectDraw();
#endif // USE_IMGUI

    // get
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    SrvManager* GetSrvManager() const { return srvManager_; }
    Camera* GetDefaultCamera() const { return defaultCamera_; }

    // set
    void SetDefaultCamera(Camera* camera) { this->defaultCamera_ = camera; }

    // コピー禁止
    Object3dCommon(const Object3dCommon&) = delete;
    Object3dCommon& operator=(const Object3dCommon&) = delete;

private:
    friend struct std::default_delete<Object3dCommon>;

    // コンストラクタ・デストラクタは private
    // Object3dCommon() = default;
    ~Object3dCommon() = default;

private:
    Camera* defaultCamera_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    // ルートシグネチャの作成
    void RootSignatureInitialize(DirectXCommon* dxcommon);

    // グラフィックスパイプラインの生成
    void graphicsPipelineInitialize(DirectXCommon* dxcommon);

    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

    // スキンモデル
    void SkinRootSignatureInitialize(DirectXCommon* dxcommon);
    void SkinPipelineInitialize(DirectXCommon* dxcommon);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> skinRootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> skinPipelineState = nullptr;

    // Cs用パイプライン
    void CSRootSignatureInitialize(DirectXCommon* dxcommon);
    void CSPipelineInitialize(DirectXCommon* dxcommon);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> csRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> csPipelineState;

    // ラインパイプラインの生成
    void LinePipelineInitialize(DirectXCommon* dxcommon);

    // lineDraw用
    Microsoft::WRL::ComPtr<ID3D12RootSignature> lineRootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> linePipelineState = nullptr;
};
