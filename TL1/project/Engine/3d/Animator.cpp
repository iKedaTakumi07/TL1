#include "Animator.h"
#include "../base/DirectXCommon.h"
#include "../base/SrvManager.h"
#include "../scene/SceneManager.h"
#include "Object3dCommon.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <cassert>
#include <cmath>

void Animator::Initialize(Object3dCommon* object3dCommon, const ModelData& modelData)
{
    object3dCommon_ = object3dCommon;

    // スケルトンとスキンクラスターの初期構築
    skeleton_ = CreateSkeleton(modelData.rootNode);
    auto device = object3dCommon_->GetDxCommon()->GetDevice();
    skinCluster_ = CreateSkinCluster(device, skeleton_, modelData);

#ifdef USE_IMGUI
    InitializeSkeletonBuffer();
#endif
    isSkinClusterDirty_ = true;
}

void Animator::Update(float deltaTime)
{
    if (isAnimating_ && currentAnimation_) {
        animationTime_ += deltaTime;
        if (isLoop_) {
            animationTime_ = std::fmod(animationTime_, currentAnimation_->duration);
        } else {
            if (animationTime_ >= currentAnimation_->duration) {
                animationTime_ = currentAnimation_->duration;
                isAnimating_ = false;
            }
        }
        ApplyAnimation(skeleton_, *currentAnimation_, animationTime_);
        isSkinClusterDirty_ = true;
    }

    if (isSkinClusterDirty_) {
        UpdateSkeleton(skeleton_);
        UpdateSkinCluster(skinCluster_, skeleton_);
        isSkinClusterDirty_ = false;
    }

#ifdef USE_IMGUI
    UpdateSkeletonLines();
#endif // USE_IMGUI
}

void Animator::UpdateSkeleton(Skeleton& skeleton)
{
    // すべてのjointを更新。親がわっ回ので通常ループで処理可
    for (Joint& joint : skeleton.joints) {
        joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
        if (joint.parent) { // 親がいれば親の行列をかける
            joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
        } else {
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}

void Animator::UpdateSkinCluster(SkinCluster& skinCluster, Skeleton& skeleton)
{
    for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
        assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());

        // 仮計算
        Matrix4x4 currentPaletteMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;

        // 比較
        if (AreMatricesEqual(currentPaletteMatrix, skinCluster.lastPaletteMatrices[jointIndex])) {
            continue;
        }

        skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix = currentPaletteMatrix;
        skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(currentPaletteMatrix));
        
        // 過去座標更新
        skinCluster.lastPaletteMatrices[jointIndex] = currentPaletteMatrix;
    }
}

Animation Animator::LoadAinmationFile(const std::string& directoryPath, const std::string& filename)
{
    Animation animation; // 構築するアニメーション

    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
    assert(scene->mNumAnimations != 0);
    aiAnimation* animationAssimp = scene->mAnimations[0]; // 最初のアニメーションだけ採用。//[後日]複数対応予定。
    animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // 時間の単位を秒に変更

    // nodeAnimationを解析
    for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
        aiNodeAnim* nodeAnimatonAssimp = animationAssimp->mChannels[channelIndex];
        NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimatonAssimp->mNodeName.C_Str()];

        // 座標
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimatonAssimp->mNumPositionKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimatonAssimp->mPositionKeys[keyIndex];
            keyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 秒に変換
            keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // 右手->左手

            nodeAnimation.translate.keyframes.push_back(keyframe);
        }

        // 回転
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimatonAssimp->mNumRotationKeys; ++keyIndex) {
            aiQuatKey& keyAssimp = nodeAnimatonAssimp->mRotationKeys[keyIndex];
            keyframeQuaternion keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);

            keyframe.value = {
                keyAssimp.mValue.x,
                -keyAssimp.mValue.y,
                -keyAssimp.mValue.z,
                keyAssimp.mValue.w
            };

            nodeAnimation.rotate.keyframes.push_back(keyframe);
        }

        // 縮尺
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimatonAssimp->mNumScalingKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimatonAssimp->mScalingKeys[keyIndex];
            keyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);

            keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };

            nodeAnimation.scale.keyframes.push_back(keyframe);
        }
    }

    return animation;
}

void Animator::LoadAnimation(const std::string& directoryPath, const std::string& filename, const std::string& animName)
{
    // すでに同じ名前で登録されている場合はスキップ
    if (animation_.find(animName) != animation_.end()) {
        return;
    }
    // アニメーションファイルをロードしてマップに登録
    animation_[animName] = LoadAinmationFile(directoryPath, filename);
}

void Animator::PlayAnimation(const std::string& animName, bool loop)
{
    auto it = animation_.find(animName);
    assert(it != animation_.end() && "指定されたアニメーションはロードされていません。");

    currentAnimation_ = &it->second;
    currentAnimationName_ = animName;
    animationTime_ = 0.0f;
    isAnimating_ = true;
    isLoop_ = loop;
    isSkinClusterDirty_ = true;
}

void Animator::StopAnimation()
{
    isAnimating_ = false;
    currentAnimation_ = nullptr;
    currentAnimationName_ = "";
    animationTime_ = 0.0f;
}
void Animator::ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime)
{
    for (Joint& joint : skeleton.joints) {
        // 対象のjointのanimationがあれば、値の適応を行う。下記のif分はC++17から可能な奴
        if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end()) {
            const NodeAnimation& rootNodeAnimaiton = (*it).second;
            joint.transform.translate = CalculateValue(rootNodeAnimaiton.translate.keyframes, animationTime);
            joint.transform.rotate = CalculateValue(rootNodeAnimaiton.rotate.keyframes, animationTime);
            joint.transform.scale = CalculateValue(rootNodeAnimaiton.scale.keyframes, animationTime);
        }
    }
}

Skeleton Animator::CreateSkeleton(const Node& rootNode)
{
    Skeleton skeleton;
    skeleton.root = CreateJoint(rootNode, { }, skeleton.joints);

    for (const Joint& joint : skeleton.joints) {
        skeleton.jointMap.emplace(joint.name, joint.index);
    }

    return skeleton;
}

int32_t Animator::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
    Joint joint;
    joint.name = node.name;
    joint.localMatrix = node.localMatrix;
    joint.skeletonSpaceMatrix = MakeIdentity4x4();
    joint.transform = node.transfrom;
    joint.index = int32_t(joints.size()); // 現在登録されている数をindex
    joint.parent = parent;
    joints.push_back(joint);
    for (const Node& child : node.childrem) {
        // 子jointを作成,index登録
        int32_t childIndex = CreateJoint(child, joint.index, joints);
        joints[joint.index].childern.push_back(childIndex);
    }
    // 自身のindexを返す
    return joint.index;
}

SkinCluster Animator::CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData)
{
    SkinCluster skinCluster;
    // Palette余用のResource確保
    skinCluster.paletteResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
    WellForGPU* mappedPalette = nullptr;
    skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
    skinCluster.mappedPalette = { mappedPalette, skeleton.joints.size() };
    uint32_t srvIndex = object3dCommon_->GetSrvManager()->Allocate();
    skinCluster.paletteSrvHandel.first = object3dCommon_->GetSrvManager()->GetCPUDescriptorHandle(srvIndex);
    skinCluster.paletteSrvHandel.second = object3dCommon_->GetSrvManager()->GetGPUDescriptorHandle(srvIndex);
    object3dCommon_->GetSrvManager()->CreateSRVforStructuredBuffer(
        srvIndex,
        skinCluster.paletteResource.Get(),
        static_cast<UINT>(skeleton.joints.size()),
        sizeof(WellForGPU));

    // Palette用のsrvを作成
    D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc { };
    paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    paletteSrvDesc.Buffer.FirstElement = 0;
    paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    paletteSrvDesc.Buffer.NumElements = UINT(skeleton.joints.size());
    paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
    device->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandel.first);

    // influence用のResourceを確保
    skinCluster.influenceResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexInfluence) * modelData.vertices.size());
    VertexInfluence* mappedInfluence = nullptr;
    skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
    std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
    skinCluster.mappedInfluence = { mappedInfluence, modelData.vertices.size() };

    //  influence用のVBVを作成
    skinCluster.influenceResourceView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
    skinCluster.influenceResourceView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
    skinCluster.influenceResourceView.StrideInBytes = sizeof(VertexInfluence);

    // InverseBindPoseMatrixの保存領域を作成
    skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
    skinCluster.lastPaletteMatrices.resize(skeleton.joints.size());
    // 範囲ベースforループで1つずつ単位行列を代入
    for (size_t i = 0; i < skeleton.joints.size(); ++i) {
        skinCluster.inverseBindPoseMatrices[i] = MakeIdentity4x4();
        skinCluster.lastPaletteMatrices[i] = MakeIdentity4x4();
    }

    // ModelDataのSkinCluster情報を解析してinfluenceの中身を埋める
    for (const auto& JointWeightData : modelData.skinClusterData) {
        // modelのskinClusterの情報を解析
        auto it = skeleton.jointMap.find(JointWeightData.first); // skelrtonに対象となるjointが含まれているか判断
        if (it == skeleton.jointMap.end()) {
            continue; // 存在しない場合。次に回す
        }
        // 存在する,該当のindexのinverseBindPosMatrixを代入―(*it).secondにはjointのindexがはいいている。
        skinCluster.inverseBindPoseMatrices[(*it).second] = JointWeightData.second.inverseBindPoseMatrix;
        for (const auto& VertexWeight : JointWeightData.second.vertexWeights) {
            auto& currentInfluence = skinCluster.mappedInfluence[VertexWeight.vertexIndex]; // 該当のvertexindexのInfluence情報を参照
            for (uint32_t index = 0; index < kNumMaxInfluence; index++) {
                if (currentInfluence.weights[index] == 0.0f) { // weigth==0が空いている状態。weightとjointのindexを代入
                    currentInfluence.weights[index] = VertexWeight.weight;
                    currentInfluence.jointIndices[index] = (*it).second;
                    break;
                }
            }
        }
    }

    uint32_t numVertices = static_cast<uint32_t>(modelData.vertices.size());
    UINT sizeInBytes = sizeof(VertexData) * numVertices;

    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES uploadHeapProps { };
    uploadHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // リソース作成
    // uavの生成
    D3D12_RESOURCE_DESC resDesc { };
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeInBytes;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&skinCluster.outputVerticesResource));
    assert(SUCCEEDED(hr));

    skinCluster.outputVerticesBufferView.BufferLocation = skinCluster.outputVerticesResource->GetGPUVirtualAddress();
    skinCluster.outputVerticesBufferView.SizeInBytes = sizeInBytes;
    skinCluster.outputVerticesBufferView.StrideInBytes = sizeof(VertexData);

    return skinCluster;
}

#ifdef USE_IMGUI
void Animator::InitializeSkeletonBuffer()
{
    // 100ジョイント分(100本)の線を想定
    uint32_t maxLines = 100;
    uint32_t bufferSize = sizeof(LineVertex) * maxLines * 2;

    // ヒーププロパティの設定 (アップロードヒープ)
    D3D12_HEAP_PROPERTIES heapProps { };
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // リソース記述
    D3D12_RESOURCE_DESC resDesc { };
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = bufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // リソースの生成
    auto device = object3dCommon_->GetDxCommon()->GetDevice();
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&skeletonVertexBuffer_));

    // ビューの作成
    skeletonVertexBufferView_.BufferLocation = skeletonVertexBuffer_->GetGPUVirtualAddress();
    skeletonVertexBufferView_.SizeInBytes = (UINT)bufferSize;
    skeletonVertexBufferView_.StrideInBytes = sizeof(LineVertex);
}

void Animator::UpdateSkeletonLines()
{
    if (skeleton_.joints.empty())
        return;

    skeletonVertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&lineVertices));

    skeletonLineCount_ = 0;
    for (const Joint& joint : skeleton_.joints) {
        if (joint.parent) {
            // 親の位置 (ローカルの累積行列の4列目)
            const Matrix4x4& parentMat = skeleton_.joints[*joint.parent].skeletonSpaceMatrix;
            Vector3 parentPos = { parentMat.m[3][0], parentMat.m[3][1], parentMat.m[3][2] };

            // 自分の位置
            const Matrix4x4& childMat = joint.skeletonSpaceMatrix;
            Vector3 childPos = { childMat.m[3][0], childMat.m[3][1], childMat.m[3][2] };

            // 頂点にセット（ワールド行列の適用はシェーダー側で行うため、ここではそのまま）
            lineVertices[skeletonLineCount_ * 2 + 0].position = { parentPos.x, parentPos.y, parentPos.z, 1.0f };
            lineVertices[skeletonLineCount_ * 2 + 1].position = { childPos.x, childPos.y, childPos.z, 1.0f };
            skeletonLineCount_++;
        }
    }
    skeletonVertexBuffer_->Unmap(0, nullptr);
}

void Animator::DrawSkeleton(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS wvpAddress)
{
    if (skeletonLineCount_ == 0)
        return;
    cmdList->IASetVertexBuffers(0, 1, &skeletonVertexBufferView_);

    // b0レジスタに WVP行列（transformationMatrixResource）を渡す
    cmdList->SetGraphicsRootConstantBufferView(0, wvpAddress);

    // 1本の線につき2頂点なので、総頂点数は LineCount * 2
    cmdList->DrawInstanced(skeletonLineCount_ * 2, 1, 0, 0);
}
#endif // USE_IMGUI