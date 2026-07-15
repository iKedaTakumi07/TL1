#include "Object3d.h"
#include "../base/DirectXCommon.h"
#include "../base/SrvManager.h"
#include "../base/TextureManager.h"
#include "../scene/SceneManager.h"
#include "Camera.h"
#include "LightManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <cassert>
#include <fstream>
#include <numbers>
#ifdef USE_IMGUI
#include "../externals/imgui/imgui.h"
#endif // USE_IMGUI
#include <algorithm>
#include <cmath>

ModelData Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
    ModelData modelData; // 構築するmodelData

    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;

    size_t lastSlash = filePath.find_last_of("/\\");
    std::string modelDirectory = (lastSlash != std::string::npos) ? filePath.substr(0, lastSlash) : directoryPath;

    const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_Triangulate);
    assert(scene->HasMeshes());

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals());
        assert(mesh->HasTextureCoords(0));

        modelData.vertices.resize(mesh->mNumVertices); // 最初に頂点数分のメモリを確保しておく
        // メッシュ頂点データ
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            aiVector3D& position = mesh->mVertices[vertexIndex];
            aiVector3D& normal = mesh->mNormals[vertexIndex];
            aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

            // 右手->左手
            modelData.vertices[vertexIndex].position = { -position.x, position.y, position.z, 1.0f };
            modelData.vertices[vertexIndex].normal = { -normal.x, normal.y, normal.z };
            modelData.vertices[vertexIndex].texcoord = { texcoord.x, texcoord.y };
        }

        // skinCluster構築
        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            // jointごとの格納領域
            aiBone* bone = mesh->mBones[boneIndex];
            std::string jointName = bone->mName.C_Str();
            JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

            // inverseBindePoseMatrixの抽出
            aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
            aiVector3D scale, translate;
            aiQuaternion rotate;
            bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
            Matrix4x4 bindPoseMatirx = MakeAffineMatrix({ scale.x, scale.y, scale.z }, { rotate.x, -rotate.y, -rotate.z, rotate.w }, { -translate.x, translate.y, translate.z });
            jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatirx);

            // weight情報取り出し
            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId });
            }
        }

        // Face解析
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);

            // vertex解析
            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                modelData.indices.push_back(vertexIndex);
            }
        }
    }

    // マテリアる
    for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        aiMaterial* material = scene->mMaterials[materialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
            aiString texttureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &texttureFilePath);

            std::string texName = texttureFilePath.C_Str();
            modelData.material.textureFilePath = modelDirectory + "/" + texName;
        }
    }

    modelData.rootNode = ReadNode(scene->mRootNode);

    return modelData;
}

Node Object3d::ReadNode(aiNode* node)
{
    Node result;
    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate);
    result.transfrom.scale = { scale.x, scale.y, scale.z };
    result.transfrom.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w }; // x軸反転、回転方向が逆なので軸を反転させる
    result.transfrom.translate = { -translate.x, translate.y, translate.z }; // x軸反転
    result.localMatrix = MakeAffineMatrix(result.transfrom.scale, result.transfrom.rotate, result.transfrom.translate);

    result.name = node->mName.C_Str(); // node名を格納
    result.childrem.resize(node->mNumChildren); // 子の数だけ確保

    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        // 再帰関数
        result.childrem[childIndex] = ReadNode(node->mChildren[childIndex]);
    }

    return result;
}

void Object3d::Initialize()
{
    this->object3dCommon = Object3dCommon::GetInstance();

    this->winApp_ = WinApp::GetInstance();

    this->camera = object3dCommon->GetDefaultCamera();

    transform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    cameraTransform = { { 1.0f, 1.0f, 1.0f }, { 0.3f, 0.0f, 0.0f }, { 0.0f, 4.0f, -10.0f } };

    TransMatrixResourceInitialize();
    cameraDataResourceInitialize();
}

void Object3d::Update()
{
    // 1. アニメーションの更新
    if (animator_) {
        float deltaTime = SceneManager::GetInstance()->GetDeltaTime();
        animator_->Update(deltaTime);
    }

    // 2. ワールド行列の計算
    Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    if (model) {
        Matrix4x4 localMatrix = MakeIdentity4x4();
        if (animator_) {
            // アニメーターからルートジョイントの行列をもらう
            localMatrix = animator_->GetSkeleton().joints[animator_->GetSkeleton().root].skeletonSpaceMatrix;
        } else {
            localMatrix = model->GetModelData().rootNode.localMatrix;
        }
        worldMatrix = Multiply(localMatrix, worldMatrix);
    }

    // 3. WVPなどの転送用行列計算
    Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
    transformationMatrixData->WVP = worldViewProjectionMatrix;
    transformationMatrixData->world = worldMatrix;
    transformationMatrixData->worldInverseTranspose = Transpose(Inverse(worldMatrix));
}

void Object3d::DrawImGui(const std::string& label)
{
#ifdef USE_IMGUI
    ImGui::Begin("Objects Control");
    ImGui::PushID(label.c_str());

    if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Indent();

        ImGui::DragFloat3("Translate", &transform.translate.x, 0.01f);
        ImGui::SliderAngle("RotateX", &transform.rotate.x);
        ImGui::SliderAngle("RotateY", &transform.rotate.y);
        ImGui::SliderAngle("RotateZ", &transform.rotate.z);
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("materialData")) {
            ImGui::Indent();
            Material* materialData = model->GetmaterialData();
            bool isLighting = (materialData->enableEnvironmentMap != 0);
            if (ImGui::Checkbox("Enable Lighting (Unlit Texture)", &isLighting)) {
                materialData->enableEnvironmentMap = isLighting ? 1 : 0;
            }
            ImGui::DragFloat("evnironmentCoefficient", &materialData->evnironmentCoefficient, 0.01f);
            model->SetMaterialDataEvnironmentCoefficient(materialData->evnironmentCoefficient);
            ImGui::Unindent();
        }

        ImGui::Unindent();
    }

    ImGui::PopID();
    ImGui::End();
#endif // USE_IMGUI
}

#ifdef USE_IMGUI
void Object3d::DrawSkeleton()
{
    // アニメーター（骨）が存在する場合のみ処理する
    if (animator_) {
        // 1. 描画する前に最新のジョイント行列から線の頂点データを更新
        animator_->UpdateSkeletonLines();

        // 2. Animatorが必要とするDirectXの情報をObject3dから取得
        auto cmdList = object3dCommon->GetDxCommon()->GetCommandList();
        auto wvpAddress = transformationMatrixResource->GetGPUVirtualAddress();

        // 3. アニメーター側の描画関数を呼び出す
        animator_->DrawSkeleton(cmdList, wvpAddress);
    }
}
#endif // USE_IMGUI

void Object3d::Draw()
{
    auto cmdList = object3dCommon->GetDxCommon()->GetCommandList();
    auto lightManager = LightManager::GetInstance();

    // cs
    if (animator_) {
        // CS用パイプラインのセット
        object3dCommon->PrepareCSObjectDraw();

        // CSを実行（頂点バッファを書き換える）
        model->CSDraw(animator_->GetSkinCluster());

        // CSでの書き込み完了を待機するUAVバリアを発行
        D3D12_RESOURCE_BARRIER barrier = { };
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        // UAVとして書き込んだリソースを指定
        barrier.UAV.pResource = animator_->GetSkinCluster().outputVerticesResource.Get();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // CSで計算済みなので通常のオブジェクト用で可。
    object3dCommon->PrepareObjectDraw();

    // 座標/カメラ(分離するかは不明)
    cmdList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(4, CameraDataResourceModel->GetGPUVirtualAddress());

    // ライト関連
    cmdList->SetGraphicsRootConstantBufferView(3, lightManager->GetDirectionalLightAddress());
    cmdList->SetGraphicsRootConstantBufferView(5, lightManager->GetPointLightAddress());
    cmdList->SetGraphicsRootConstantBufferView(6, lightManager->GetSpotLightAddress());

    // モデルを描画
    if (animator_) {
        model->Draw(animator_->GetSkinCluster());
    } else {
        model->Draw();
    }
}

void Object3d::SetModel(Model* model)
{
    this->model = model;

    if (this->model) {
        // モデルがスキンデータを持っていれば、アニメーターコンポーネントを生成
        if (!this->model->GetModelData().skinClusterData.empty()) {
            animator_ = std::make_unique<Animator>();
            animator_->Initialize(object3dCommon, this->model->GetModelData());
        } else {
            // 静的モデルならアニメーターは不要なので解放
            animator_ = nullptr;
        }
    }
}

void Object3d::SetModel(const std::string& filePath)
{
    // モデル検索してセット
    model = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::TransMatrixResourceInitialize()
{
    transformationMatrixResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

    // 書き込むためのアドレス取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
    // 単位行列を書き込む
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->world = MakeIdentity4x4();
    transformationMatrixData->worldInverseTranspose = MakeIdentity4x4();
}

void Object3d::cameraDataResourceInitialize()
{
    // sphere用のマテリアルリソースを作る
    CameraDataResourceModel = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(CameraForGPU));

    // mapして書き込み
    CameraDataResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&CameraForGPUData));
    // 今回は白を書き込んでみる
    CameraForGPUData->worldPosition = cameraTransform.translate;
}
