#pragma once
#include "../base/Math.h"
#include "Model.h"
#include <d3d12.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

class Object3dCommon;

class Animator {
public:
    Animator() = default;
    ~Animator() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="object3dCommon">dxcommon</param>
    /// <param name="modelData">モデルデータ</param>
    void Initialize(Object3dCommon* object3dCommon, const ModelData& modelData);

    /// <summary>
    /// 更新
    /// </summary>
    /// <param name="deltaTime">デルタタイム</param>
    void Update(float deltaTime);

    /// <summary>
    /// アニメーション読み込み
    /// </summary>
    /// <param name="directoryPath"フォルダ名></param>
    /// <param name="filenamem">ファイルネーム</param>
    /// <param name="animName">アニメーション名(保存用)</param>
    void LoadAnimation(const std::string& directoryPath, const std::string& filename, const std::string& animName);

    /// <summary>
    /// アニメーションを再生
    /// </summary>
    /// <param name="animName">アニメーション名</param>
    /// <param name="loop">ループ再生</param>
    void PlayAnimation(const std::string& animName, bool loop = true);

    /// <summary>
    /// アニメーションを停止
    /// </summary>
    void StopAnimation();

#ifdef USE_IMGUI
    void InitializeSkeletonBuffer();
    void UpdateSkeletonLines();
    // 骨を表示
    void DrawSkeleton(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS wvpAddress);
#endif

    // Object3d や Model が描画時に必要とするデータのゲッター
    const SkinCluster& GetSkinCluster() const { return skinCluster_; }
    const Skeleton& GetSkeleton() const { return skeleton_; }
    bool IsAnimating() const { return isAnimating_; }

private:
    // アニメーションファイルの直接ロード（内部用）
    static Animation LoadAinmationFile(const std::string& directoryPath, const std::string& filename);

    // スケルトン・スキンクラスターの生成
    Skeleton CreateSkeleton(const Node& rootNode);
    int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);
    SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData);

    // 行列の更新
    void UpdateSkeleton(Skeleton& skeleton);
    void UpdateSkinCluster(SkinCluster& skinCluster, Skeleton& skeleton);
    void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);

private:
    // ポインター
    Object3dCommon* object3dCommon_ = nullptr;

    // アニメーションデータ・再生状態
    std::unordered_map<std::string, Animation> animation_;
    Animation* currentAnimation_ = nullptr;
    std::string currentAnimationName_ = "";
    float animationTime_ = 0.0f;
    bool isAnimating_ = false;
    bool isLoop_ = true;

    // スケルトン・スキンクラスターの実体
    Skeleton skeleton_;
    SkinCluster skinCluster_;
    bool isSkinClusterDirty_ = true;

#ifdef USE_IMGUI
    Microsoft::WRL::ComPtr<ID3D12Resource> skeletonVertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW skeletonVertexBufferView_ { };
    uint32_t skeletonLineCount_ = 0;
    LineVertex* lineVertices = nullptr;
#endif
};
