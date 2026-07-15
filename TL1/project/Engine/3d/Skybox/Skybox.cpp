#include "Skybox.h"
#include "../../3d/Camera.h"
#include "../../base/TextureManager.h"
#include "SkyBoxCommon.h"

void Skybox::Initialize(std::string texturefilePath)
{
    SkyBoxCommon_ = SkyBoxCommon::GetInstance();

    texturefilePath_ = texturefilePath;

    // 頂点データ
    VertexResourceInitialize();
    MaterialResourceInitialize();
    TransMatrixResourceInitialize();
}

void Skybox::Update()
{
    Matrix4x4 worldMatrix = MakeIdentity4x4();

    // 2. View行列（カメラの回転だけを反映し、平行移動を無効化する）
    Matrix4x4 viewMatrix = camera->GetViewMatrix();
    viewMatrix.m[3][0] = 0.0f; // X移動をリセット
    viewMatrix.m[3][1] = 0.0f; // Y移動をリセット
    viewMatrix.m[3][2] = 0.0f; // Z移動をリセット

    // 3. Projection行列（遠近投影行列を使用）
    Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

    // 合成：WVP
    transformationMatrixData->WVP = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
    transformationMatrixData->world = worldMatrix;
}

void Skybox::Draw()
{
    // skyBoxの描画
    // VertexBufferView
    SkyBoxCommon::GetInstance()->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
    // IndexBufferView
    SkyBoxCommon::GetInstance()->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

    // マテリアルCBufferの場所を設定
    SkyBoxCommon::GetInstance()->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    // wvp用のCBufferの場所を設定
    SkyBoxCommon::GetInstance()->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

    SkyBoxCommon::GetInstance()->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::getInstance()->GetSrvHandelGPU(texturefilePath_));

    // 描画
    SkyBoxCommon::GetInstance()->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void Skybox::VertexResourceInitialize()
{
    vertexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(SkyboxVertexData) * 24);
    // リソースの先端のアドレスから使う
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    // 使用するサイズ
    vertexBufferView.SizeInBytes = sizeof(SkyboxVertexData) * 24;
    // 1ツ当たりのサイズ
    vertexBufferView.StrideInBytes = sizeof(SkyboxVertexData);

    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    // インデックスリソースにデータを書き込む
    indexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 36);
    // リソースの先頭のアドレスから使う
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    // 使用するリソースのサイズはインデックス6つ分のサイズ
    indexBufferView.SizeInBytes = sizeof(uint32_t) * 36;
    // インデックスはuint32_Tとする
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // インデックスリソースにデータを書きこむ
    indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

    vertexData[0].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData[0].texcoord = { 1.0f, 1.0f, 1.0f };
    vertexData[1].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData[1].texcoord = { 1.0f, 1.0f, -1.0f };
    vertexData[2].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData[2].texcoord = { 1.0f, -1.0f, 1.0f };
    vertexData[3].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    vertexData[3].texcoord = { 1.0f, -1.0f, -1.0f };

    // 左面 (X-)
    vertexData[4].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData[4].texcoord = { -1.0f, 1.0f, -1.0f };
    vertexData[5].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData[5].texcoord = { -1.0f, 1.0f, 1.0f };
    vertexData[6].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData[6].texcoord = { -1.0f, -1.0f, -1.0f };
    vertexData[7].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData[7].texcoord = { -1.0f, -1.0f, 1.0f };

    // 前面 (Z+)
    vertexData[8].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData[8].texcoord = { -1.0f, 1.0f, 1.0f };
    vertexData[9].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData[9].texcoord = { 1.0f, 1.0f, 1.0f };
    vertexData[10].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData[10].texcoord = { -1.0f, -1.0f, 1.0f };
    vertexData[11].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData[11].texcoord = { 1.0f, -1.0f, 1.0f };

    // 後面 (Z-)
    vertexData[12].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData[12].texcoord = { 1.0f, 1.0f, -1.0f };
    vertexData[13].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData[13].texcoord = { -1.0f, 1.0f, -1.0f };
    vertexData[14].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    vertexData[14].texcoord = { 1.0f, -1.0f, -1.0f };
    vertexData[15].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData[15].texcoord = { -1.0f, -1.0f, -1.0f };

    // 上面 (Y+)
    vertexData[16].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData[16].texcoord = { -1.0f, 1.0f, -1.0f };
    vertexData[17].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData[17].texcoord = { 1.0f, 1.0f, -1.0f };
    vertexData[18].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData[18].texcoord = { -1.0f, 1.0f, 1.0f };
    vertexData[19].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData[19].texcoord = { 1.0f, 1.0f, 1.0f };

    // 下面 (Y-)
    vertexData[20].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData[20].texcoord = { -1.0f, -1.0f, 1.0f };
    vertexData[21].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData[21].texcoord = { 1.0f, -1.0f, 1.0f };
    vertexData[22].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData[22].texcoord = { -1.0f, -1.0f, -1.0f };
    vertexData[23].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    vertexData[23].texcoord = { 1.0f, -1.0f, -1.0f };

    // インデックス設定 (ループで全6面分生成)
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t vOff = i * 4;
        uint32_t iOff = i * 6;
        indexData[iOff] = vOff;
        indexData[iOff + 1] = vOff + 1;
        indexData[iOff + 2] = vOff + 2;
        indexData[iOff + 3] = vOff + 1;
        indexData[iOff + 4] = vOff + 3;
        indexData[iOff + 5] = vOff + 2;
    }
}

void Skybox::MaterialResourceInitialize()
{
    // Sprite用のマテリアルリソースを作る
    materialResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Material));
    // mapして書き込み
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    // 今回は白を書き込んでみる
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    materialData->enableEnvironmentMap = false;
    materialData->uvTransform = MakeIdentity4x4();
}

void Skybox::TransMatrixResourceInitialize()
{
    transformationMatrixResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

    // 書き込むためのアドレス取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
    // 単位行列を書き込む
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->world = MakeIdentity4x4();
}
