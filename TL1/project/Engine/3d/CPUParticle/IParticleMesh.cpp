#include "IParticleMesh.h"
#include <cassert>
#include <cmath>
#include <numbers>

// バッファリソース生成用
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, size_t size)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_HEAP_PROPERTIES heapProps {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = size;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
    return resource;
}

// === PlaneMesh の実装 ===
PlaneMesh::PlaneMesh(ID3D12Device* device)
{
    size_t size = sizeof(VertexData) * 6;
    vertexResource_ = CreateBuffer(device, size);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(size);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    vertexData[0] = { { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    vertexData[1] = { { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    vertexData[2] = { { 1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };
    vertexData[3] = { { 1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };
    vertexData[4] = { { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    vertexData[5] = { { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };

    vertexResource_->Unmap(0, nullptr);
}

// === RingMesh の実装 ===
RingMesh::RingMesh(ID3D12Device* device, uint32_t divide, float outerRadius, float innerRadius)
{
    // 1分割につき2つの三角形（6頂点）が必要
    vertexCount_ = divide * 6;
    size_t size = sizeof(VertexData) * vertexCount_;
    vertexResource_ = CreateBuffer(device, size);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(size);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    float radianPreDivide = 2.0f * std::numbers::pi_v<float> / float(divide);

    for (uint32_t index = 0; index < divide; ++index) {
        float Rad = index * radianPreDivide;
        float nextRad = (index + 1) * radianPreDivide;

        // 元コードの cos の計算が sin になっていたバグと、
        // ルート内で同じ配列インデックス[0~3]を上書きしていたバグを修正
        float sin = std::sin(Rad);
        float cos = std::cos(Rad);
        float sinNext = std::sin(nextRad);
        float cosNext = std::cos(nextRad);

        float u = float(index) / float(divide);
        float uNext = float(index + 1) / float(divide);

        // 各位置の頂点座標を計算
        Vector4 pOuter = { -sin * outerRadius, cos * outerRadius, 0.0f, 1.0f };
        Vector4 pOuterNext = { -sinNext * outerRadius, cosNext * outerRadius, 0.0f, 1.0f };
        Vector4 pInner = { -sin * innerRadius, cos * innerRadius, 0.0f, 1.0f };
        Vector4 pInnerNext = { -sinNext * innerRadius, cosNext * innerRadius, 0.0f, 1.0f };

        uint32_t offset = index * 6;
        Vector3 normal = { 0.0f, 0.0f, 1.0f };

        // 三角形1
        vertexData[offset + 0] = { pOuter, { u, 0.0f }, normal };
        vertexData[offset + 1] = { pOuterNext, { uNext, 0.0f }, normal };
        vertexData[offset + 2] = { pInner, { u, 1.0f }, normal };
        // 三角形2
        vertexData[offset + 3] = { pInner, { u, 1.0f }, normal };
        vertexData[offset + 4] = { pOuterNext, { uNext, 0.0f }, normal };
        vertexData[offset + 5] = { pInnerNext, { uNext, 1.0f }, normal };
    }

    vertexResource_->Unmap(0, nullptr);
}

CylinderMesh::CylinderMesh(ID3D12Device* device, uint32_t divide, float kTopRadius, float kBottomRadius, float kHeight)
{
    // 1分割につき2つの三角形（6頂点）が必要
    vertexCount_ = divide * 6;
    size_t size = sizeof(VertexData) * vertexCount_;
    vertexResource_ = CreateBuffer(device, size);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(size);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    float radianPreDivide = 2.0f * std::numbers::pi_v<float> / float(divide);

    for (uint32_t index = 0; index < divide; ++index) {
        float sin = std::sin(index * radianPreDivide);
        float cos = std::cos(index * radianPreDivide);
        float sinNext = std::sin((index + 1) * radianPreDivide);
        float cosNext = std::cos((index + 1) * radianPreDivide);
        float u = float(index) / float(divide);
        float uNext = float(index + 1) / float(divide);

        // pos,tex,normal
        uint32_t offset = index * 6;
        // 三角形1
        vertexData[offset + 0] = { { -sin * kTopRadius, kHeight, cos * kTopRadius, 1.0f }, { u, vTop }, { -sin, 0.0f, cos } };
        vertexData[offset + 1] = { { -sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f }, { uNext, vTop }, { -sinNext, 0.0f, cosNext } };
        vertexData[offset + 2] = { { -sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f }, { u, vBottom }, { -sin, 0.0f, cos } };
        // 三角形2
        vertexData[offset + 3] = { { -sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f }, { u, vBottom }, { -sin, 0.0f, cos } };
        vertexData[offset + 4] = { { -sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f }, { uNext, vTop }, { -sinNext, 0.0f, cosNext } };
        vertexData[offset + 5] = { { -sinNext * kBottomRadius, 0.0f, cosNext * kBottomRadius, 1.0f }, { uNext, vBottom }, { -sinNext, 0.0f, cosNext } };
    }

    vertexResource_->Unmap(0, nullptr);
}
