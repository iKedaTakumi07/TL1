#pragma once
#include "StringUtility.h"
#include "../externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

class DirectXCommon;
class SrvManager;

class TextureManager {
public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class TextureManager;
    };

    // passkeyを受け取るコンストラクタ
    explicit TextureManager(ConstructorKey) { }

    static TextureManager* getInstance();

    void Finalize();

    // 初期化
    void Initialize(DirectXCommon* DirectXCollision, SrvManager* srvManager);

    // テクスチャファイルの読み込み
    void LoadTexture(const std::string& filePath);

    // SRVインデクスの開始番号
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);
    // テクスチャ番号からGPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandelGPU(const std::string& filePath);

    uint32_t GetSrvIndex(const std::string& filePath);

    const DirectX::TexMetadata& GetMetadata(const std::string& filePath);

    TextureManager(TextureManager&) = delete;
    TextureManager& operator=(TextureManager&) = delete;

private:
    ~TextureManager() = default;

    friend struct std::default_delete<TextureManager>;

private:
    static std::unique_ptr<TextureManager> instance;
    SrvManager* srvManager = nullptr;
    DirectXCommon* dxCommon = nullptr;

    // テクスチャデータ
    struct TextureData {
        std::string filePath;
        DirectX::TexMetadata metadata;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t srvIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
    };

    // テクスチャデータ
    std::unordered_map<std::string, TextureData> textureDatas;

    static uint32_t kSRVIndexTop;
};
