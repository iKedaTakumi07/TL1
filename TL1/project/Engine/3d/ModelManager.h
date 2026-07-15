#pragma once
#include <map>
#include <memory>
#include <string>

#include "Model.h"

class DirectXCommon;
class ModelCommon;

class ModelManager {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class ModelManager;
    };

    // passkeyを受け取るコンストラクタ
    explicit ModelManager(ConstructorKey) { }

    /// <summary>
    /// モデルファイルの読み込み
    /// </summary>
    /// <param name="filePath">モデルファイルのパス</param>
    void LoadModel(const std::string& filePath);

    /// <summary>
    /// モデルの検索
    /// </summary>
    /// <param name="filePath">モデルファイルのパス</param>
    /// <returns></returns>
    Model* FindModel(const std::string& filePath);

    // シングルトンインスタンスの取得
    static ModelManager* GetInstance();
    // 終了
    void Finalize();

    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

private:
    // ModelManager() = default;
    ~ModelManager() = default;

    friend struct std::default_delete<ModelManager>;

private:
    // モデルデータ
    std::map<std::string, std::unique_ptr<Model>> models;
    ModelCommon* modelCommon = nullptr;

    static std::unique_ptr<ModelManager> instance;
    static bool finalized;
};
