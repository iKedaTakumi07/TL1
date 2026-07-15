#pragma once
#include "../base/DirectXCommon.h"
#include <memory>

class ModelCommon {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class ModelCommon;
    };

    // passkeyを受け取るコンストラクタ
    explicit ModelCommon(ConstructorKey) { }

    // Singleton 取得
    static ModelCommon* GetInstance();

    void Initialize(DirectXCommon* dxCommon);

    ModelCommon(const ModelCommon&) = delete;
    ModelCommon& operator=(const ModelCommon&) = delete;
    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
    ~ModelCommon() = default;

    friend struct std::default_delete<ModelCommon>;

private:
    DirectXCommon* dxCommon_ = nullptr;
};
