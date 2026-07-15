#include "Model.h"
#include "../base/TextureManager.h"
#include "ModelCommon.h"
#include "Object3d.h"

void Model::Initialize(const std::string& directorypath, const std::string& filename)
{
    modelCommon_ = ModelCommon::GetInstance();

    modelData = Object3d::LoadObjFile(directorypath, filename);

    // 頂点データ初期化
    VertexResourceInitialize();
    IndexResourceInitialize();
    MaterialResourceInitialize();
    SkinningInformationResourceInitialize(); // CS用
}

void Model::Draw()
{
    modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

    modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

    modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::getInstance()->GetSrvHandelGPU(modelData.material.textureFilePath));

    if (!texturefilePath_.empty()) {
        modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(7, TextureManager::getInstance()->GetSrvHandelGPU(texturefilePath_));
    } else {
        // Skyboxがない場合は、既にロードされている適当なテクスチャをダミーとしてバインドしてエラーを防ぐ
        modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(7, TextureManager::getInstance()->GetSrvHandelGPU("resources/rostock_laage_airport_4k.dds"));
    }

    modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
}

void Model::Draw(const SkinCluster& skinCluster)
{
    modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &skinCluster.outputVerticesBufferView);

    modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

    modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::getInstance()->GetSrvHandelGPU(modelData.material.textureFilePath));

    if (!texturefilePath_.empty()) {
        modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(7, TextureManager::getInstance()->GetSrvHandelGPU(texturefilePath_));
    } else {
        // Skyboxがない場合は、既にロードされている適当なテクスチャをダミーとしてバインドしてエラーを防ぐ
        modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(7, TextureManager::getInstance()->GetSrvHandelGPU("resources/rostock_laage_airport_4k.dds"));
    }

    modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
}

void Model::CSDraw(const SkinCluster& skinCluster)
{
    auto cmdList = modelCommon_->GetDxCommon()->GetCommandList();

    // b0
    cmdList->SetComputeRootConstantBufferView(0, skinningInformationResource_->GetGPUVirtualAddress());

    // t0
    cmdList->SetComputeRootShaderResourceView(1, skinCluster.paletteResource->GetGPUVirtualAddress());

    // t1
    cmdList->SetComputeRootShaderResourceView(2, vertexResource->GetGPUVirtualAddress());

    // t2
    cmdList->SetComputeRootShaderResourceView(3, skinCluster.influenceResource->GetGPUVirtualAddress());

    // u0
    cmdList->SetComputeRootUnorderedAccessView(4, skinCluster.outputVerticesResource->GetGPUVirtualAddress());

    // 2. コンピュートシェーダーを実行 (スレッドグループ数を計算)
    // HLSL側が [numthreads(1024, 1, 1)] なので、1024で割る（切り上げ処理）
    uint32_t numVertices = static_cast<uint32_t>(modelData.vertices.size());
    uint32_t groupCount = (numVertices + 1023) / 1024;

    cmdList->Dispatch(groupCount, 1, 1);
}

void Model::VertexResourceInitialize()
{
    vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
    // リソースの先端のアドレスから使う
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    // 使用するサイズ
    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
    // 1ツ当たりのサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::IndexResourceInitialize()
{
    size_t sizeInBytes = sizeof(uint32_t) * modelData.indices.size();

    // バッファリソースの作成
    indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeInBytes);

    // インデックスバッファビュー
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = UINT(sizeInBytes);
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // GPUマッピング
    indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
    std::memcpy(indexData, modelData.indices.data(), sizeInBytes);

    indexResource->Unmap(0, nullptr);
}

void Model::MaterialResourceInitialize()
{
    // model用のマテリアルリソースを作る
    materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
    // mapして書き込み
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    // 今回は白を書き込んでみる
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData->enableEnvironmentMap = false;
    materialData->shininess = 20.0f;
    materialData->uvTransform = MakeIdentity4x4();
    materialData->evnironmentCoefficient = 0.0f;
}

void Model::SkinningInformationResourceInitialize()
{
    skinningInformationResource_ = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t));

    uint32_t* mappedData = nullptr;
    skinningInformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

    *mappedData = static_cast<uint32_t>(modelData.vertices.size());

    skinningInformationResource_->Unmap(0, nullptr);
}
