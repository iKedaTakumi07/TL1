#pragma once
#include "../base/Math.h"
#include "../externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>

class SpriteCommon;
class WinApp;
class TextureManager;

class Sprite {
public:
    // 初期化
    void Initialize(std::string texturefilePath);

    // 更新
    void Update();

    // 更新
    void Draw();

    // getter
    const Vector2& GetPosition() const { return position; }
    float GetRotation() const { return rotation; }
    const Vector4& GetColor() { return materialData->color; }
    const Vector2& GetSize() const { return size; }
    const uint32_t GettextureIndex() { return textureIndex; }
    const Vector2& GetAnchorPoint() const { return anchorPoint; }
    bool GetisFlipX_() const { return isFlipX_; }
    bool GetisFlipY_() const { return isFlipY_; }
    const Vector2& GettextureLeftTop() const { return textureLeftTop; }
    const Vector2& GetextureSize() const { return textureSize; }

    // setter
    void SetPosition(const Vector2& position) { this->position = position; }
    void SetRotation(float rotation) { this->rotation = rotation; }
    void SetColor(const Vector4& color) { materialData->color = color; }
    void SetSize(const Vector2& size) { this->size = size; }
    void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }
    void SetisFlipX_(float isFlipX_) { this->isFlipX_ = isFlipX_; }
    void SetisFlipY_(float isFlipY_) { this->isFlipY_ = isFlipY_; }
    void SettextureLeftTop(const Vector2& textureLeftTop) { this->textureLeftTop = textureLeftTop; }
    void SettextureSize(const Vector2& textureSize) { this->textureSize = textureSize; }

private:
    void VertexResourceInitialize();
    void MaterialResourceInitialize();
    void TransMatrixResourceInitialize();

    // Sprite用の頂点リソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

    VertexData* vertexData = nullptr;
    uint32_t* indexData = nullptr;

    // 頂点バッファビューを作る
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView {};

    // Sprite用のマテリアルリソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;

    // バッファリソース
    Material* materialData;

    // バッファリソース
    TransformationMatrix* transformationMatrixData;

    // Sprite用のtransformMatrix用のリソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;

    // 動かす用のtransform
    Transform transform { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    Vector2 position = { 0.0f, 0.0f };
    float rotation = 0.0f;
    Vector2 size { 640.0f, 360.0f };

    uint32_t textureIndex = 0;

    // アンカーポイント
    Vector2 anchorPoint = { 0.0f, 0.0f };
    // フリップ
    bool isFlipX_ = false;
    bool isFlipY_ = false;
    // テクスチャ左上座標
    Vector2 textureLeftTop = { 0.0f, 0.0f };
    // テクスチャ切り出しサイズ
    Vector2 textureSize = { 100.0f, 100.0f };

    void AdjustTextureSize(std::string texturefilePath);
    std::string texturefilePath_;
};