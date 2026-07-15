#pragma once
#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "../../base/Math.h"

class SkyBoxCommon;
class Camera;

class WinApp;
class TextureManager;

class Skybox {
public:
    // 初期化
    void Initialize(std::string texturefilePath);

    // 更新
    void Update();

    // 更新
    void Draw();

    // Set
    void SetCamera(Camera* camera) { this->camera = camera; }

    // Get
    const std::string& GetTextureFilePath() const { return texturefilePath_; }

private:
    void VertexResourceInitialize();
    void MaterialResourceInitialize();
    void TransMatrixResourceInitialize();

private:
    SkyBoxCommon* SkyBoxCommon_ = nullptr;
    Camera* camera = nullptr;

    SkyboxVertexData* vertexData = nullptr;

    // Skybox用のマテリアルリソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
    // バッファリソース
    Material* materialData;

    // 頂点バッファビューを作る
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView {};
    // Skybox用の頂点リソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

    // バッファリソース
    TransformationMatrix* transformationMatrixData;

    // Skybox用のtransformMatrix用のリソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;

    uint32_t textureIndex_ = 0;
    uint32_t* indexData = nullptr;
    std::string texturefilePath_;
};
