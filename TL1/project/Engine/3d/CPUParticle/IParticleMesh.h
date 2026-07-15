#pragma once
#include "../../base/Math.h" // Vector2, Vector3, Vector4 などの定義を想定
#include <d3d12.h>
#include <wrl.h>

// 形状の基底インターフェース
class IParticleMesh {
public:
    virtual ~IParticleMesh() = default;
    virtual const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const = 0;
    virtual uint32_t GetVertexCount() const = 0;
};

// --- 板ポリ (Plane) クラス ---
class PlaneMesh : public IParticleMesh {
public:
    PlaneMesh(ID3D12Device* device);
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const override { return vertexBufferView_; }
    uint32_t GetVertexCount() const override { return 6; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
};

// --- リング (Ring) クラス ---
class RingMesh : public IParticleMesh {
public:
    RingMesh(ID3D12Device* device, uint32_t divide = 32, float outerRadius = 1.0f, float innerRadius = 0.2f);
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const override { return vertexBufferView_; }
    uint32_t GetVertexCount() const override { return vertexCount_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
    uint32_t vertexCount_ = 0;
};

// Cylinder
class CylinderMesh : public IParticleMesh {
public:
    CylinderMesh(ID3D12Device* device, uint32_t divide = 32, float kTopRadius = 1.0f, float kBottomRadius = 1.0f, float kHeight = 3.0f);
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const override { return vertexBufferView_; }
    uint32_t GetVertexCount() const override { return vertexCount_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
    uint32_t vertexCount_ = 0;

    float vTop = 1.0f;
    float vBottom = 0.0f;
};