#pragma once
#include "../base/Math.h"
#include <d3d12.h>
#include <wrl.h>
class ModelCommon;

class Model {
public:
    // 初期化
    void Initialize(const std::string& directorypath, const std::string& filename);

    void Draw();

    // スキンモデル用
    void Draw(const SkinCluster& skinCluster);
    // CS用
    void CSDraw(const SkinCluster& skinCluster);

    // Get
    Material* GetmaterialData() { return materialData; }
    const ModelData& GetModelData() const { return modelData; }

    // Set
    void SetEvnTexturefilePath(std::string texturefilePath) { texturefilePath_ = texturefilePath; }
    void SetMaterialDataEvnironmentCoefficient(float num) { materialData->evnironmentCoefficient = num; }

    SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData,
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize);

private:
    void VertexResourceInitialize();
    void IndexResourceInitialize();
    void MaterialResourceInitialize();
    void SkinningInformationResourceInitialize();

    ModelCommon* modelCommon_ = nullptr;

    ModelData modelData;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView { };
    VertexData* vertexData = nullptr;

    // インデックス
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
    D3D12_INDEX_BUFFER_VIEW indexBufferView { };
    uint32_t* indexData = nullptr;

    // Model用のマテリアルリソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
    // バッファリソース
    Material* materialData;
    std::string texturefilePath_;

    Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformationResource_;
};
