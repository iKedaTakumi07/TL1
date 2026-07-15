#include "GPUParticleManager.h"
#include "../3d/Camera.h"
#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/Math.h"
#include "../base/SrvManager.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "../scene/SceneManager.h"

#include <memory>
#include <numbers>

std::unique_ptr<GPUParticleManager> GPUParticleManager::instance = nullptr;

GPUParticleManager* GPUParticleManager::getInstance()
{

    if (instance == nullptr) {
        instance = std::make_unique<GPUParticleManager>(ConstructorKey());
    }
    return instance.get();
}

void GPUParticleManager::Initialize(DirectXCommon* DirectXCollision, SrvManager* srvManager)
{
    dxCommon = DirectXCollision;
    this->srvManager = srvManager;
    this->winApp_ = WinApp::GetInstance();

    graphicsPipelineInitialize(dxCommon);
    CSPipelineInitialize(dxCommon);
    CSUpdatePipelineInitialize(dxCommon);

    ResourceInitialize();

    PrepareCSInitialize();
    CSInitialize();
}

void GPUParticleManager::Update()
{
    if (!Camera_) {
        return;
    }

    float deltaTime = SceneManager::GetInstance()->GetDeltaTime();
    eitterSphere->frequemcyTime += deltaTime;

    PreFrameData_->deltaTime = deltaTime;
    PreFrameData_->time += deltaTime;

    if (eitterSphere->frequency <= eitterSphere->frequemcyTime) {
        eitterSphere->frequemcyTime -= eitterSphere->frequency;
        eitterSphere->emit = 1;
    } else {
        eitterSphere->emit = 0;
    }

    const Matrix4x4& viewProjection = Camera_->GetViewProjectionMatrix();

    // ビルボード行列の計算
    Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
    Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, Camera_->GetWorldMatrix());
    // 平行移動成分をリセット
    billboardMatrix.m[3][0] = 0.0f;
    billboardMatrix.m[3][1] = 0.0f;
    billboardMatrix.m[3][2] = 0.0f;

    PerView* perViewData = nullptr;
    perViewResource->Map(0, nullptr, reinterpret_cast<void**>(&perViewData));

    perViewData->viewProjection = viewProjection;
    perViewData->billboardMatrix = billboardMatrix;

    perViewResource->Unmap(0, nullptr);

    PrepareCSUpdate();
    CSUpdate();
}

void GPUParticleManager::Draw()
{

    auto* commandList = dxCommon->GetCommandList();

    // ルートシグネチャ設定
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    // PSO設定
    commandList->SetPipelineState(graphicsPipelineState.Get());

    // プリミティブトポロジー
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    commandList->SetGraphicsRootConstantBufferView(1, perViewResource->GetGPUVirtualAddress());

    // テクスチャSRV
    srvManager->SetGraphicsRootDescriptorTable(2, instancingSrvIndex);

    // インスタンシングSRV
    srvManager->SetGraphicsRootDescriptorTable(3, textureSrvIndex);

    // お試し1024
    commandList->DrawInstanced(6, kMaxParticles, 0, 0);
}

void GPUParticleManager::PrepareCSInitialize()
{
    dxCommon->GetCommandList()->SetComputeRootSignature(csInitRootSignature.Get());
    dxCommon->GetCommandList()->SetPipelineState(csInitPipelineState.Get());
}

void GPUParticleManager::CSInitialize()
{
    auto* cmdList = dxCommon->GetCommandList();

    // u0
    cmdList->SetComputeRootUnorderedAccessView(0, particleResource_->GetGPUVirtualAddress());

    // u1
    cmdList->SetComputeRootUnorderedAccessView(1, freeCounterResource->GetGPUVirtualAddress());

    // とりあえずお試し1024
    cmdList->Dispatch(1, 1, 1);

    // バリア張る
    D3D12_RESOURCE_BARRIER barrier = { };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    // UAVとして書き込んだリソースを指定
    auto* commandList = dxCommon->GetCommandList();
    barrier.UAV.pResource = particleResource_.Get();
    commandList->ResourceBarrier(1, &barrier);
}

void GPUParticleManager::PrepareCSUpdate()
{
    dxCommon->GetCommandList()->SetComputeRootSignature(csUpdateRootSignature.Get());
    dxCommon->GetCommandList()->SetPipelineState(csUpdatePipelineState.Get());
}

void GPUParticleManager::CSUpdate()
{
    auto* cmdList = dxCommon->GetCommandList();

    D3D12_RESOURCE_BARRIER barrier = { };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = particleResource_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // u0
    cmdList->SetComputeRootUnorderedAccessView(0, particleResource_->GetGPUVirtualAddress());

    // u1
    cmdList->SetComputeRootUnorderedAccessView(1, freeCounterResource->GetGPUVirtualAddress());

    // b0
    cmdList->SetComputeRootConstantBufferView(2, SphereResource->GetGPUVirtualAddress());

    // b1
    cmdList->SetComputeRootConstantBufferView(3, randomResource->GetGPUVirtualAddress());

    // とりあえずお試し1024
    cmdList->Dispatch(1, 1, 1);

    // バリア張る
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = particleResource_.Get();
    cmdList->ResourceBarrier(1, &barrier);

    // ステート
    D3D12_RESOURCE_BARRIER transitionBarrier = { };
    transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    transitionBarrier.Transition.pResource = particleResource_.Get();
    transitionBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    transitionBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; // VSでの読み込み用
    transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &transitionBarrier);
}

void GPUParticleManager::RootSignatureInitialize(DirectXCommon* dxcommon)
{
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature { };
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = { };
    descriptorRangeForInstancing[0].BaseShaderRegister = 0;
    descriptorRangeForInstancing[0].NumDescriptors = 1;
    descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = { };
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[4] = { };
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = { };
    // [0] WRAP用 (主にPlane用) -> register(s0)
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0; // ⭐️ s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // [1] CLAMP用 (主にRing用) -> register(s1)
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister = 1; // ⭐️ s1
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    HRESULT hr;

    // シリアスライズしてバイナリにする
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    // バイナリを元に生成
    rootSignature = nullptr;
    hr = dxcommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));
}

void GPUParticleManager::graphicsPipelineInitialize(DirectXCommon* dxcommon)
{
    // ルートシグネチャ作成
    RootSignatureInitialize(dxcommon);

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = { };
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[2].SemanticName = "COLOR";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc { };
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc { };
    // 全ての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc { };
    // 裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // Shaderをコンパイルする
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxcommon->CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderBlob = dxcommon->CompileShader(L"resources/shaders/CPUParticle.PS.hlsl", L"ps_6_0");
    assert(pixeShaderBlob != nullptr);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc { };
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.PS = { pixeShaderBlob->GetBufferPointer(), pixeShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // 利用するとぽろじのタイプ
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に色を打ち込むかの設定
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // depthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc { };
    // depthを有効化
    depthStencilDesc.DepthEnable = true;
    // 書き込み
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    // 比較関数はLessEqual
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 実際に生成
    HRESULT hr;
    graphicsPipelineState = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));
}

void GPUParticleManager::CSRootSignatureInitialize(DirectXCommon* dxcommon)
{
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature { };
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // CS用はインプットアセンブラ不要

    D3D12_ROOT_PARAMETER rootParameters[2] = { };

    // u0: gOutputVertices (UAV)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // u1
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParameters[1].Descriptor.ShaderRegister = 1;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    csInitRootSignature = nullptr;
    hr = dxcommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&csInitRootSignature));
}

void GPUParticleManager::CSPipelineInitialize(DirectXCommon* dxcommon)
{
    CSRootSignatureInitialize(dxcommon);

    HRESULT hr;

    // CSをコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> computeShaderBlob = dxcommon->CompileShader(L"resources/shaders/InitializeParticle.CS.hlsl", L"cs_6_0");
    assert(computeShaderBlob != nullptr);
    // PSOの設定
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc { };
    computePipelineStateDesc.CS = {
        .pShaderBytecode = computeShaderBlob->GetBufferPointer(),
        .BytecodeLength = computeShaderBlob->GetBufferSize()
    };
    computePipelineStateDesc.pRootSignature = csInitRootSignature.Get();

    csInitPipelineState = nullptr;
    hr = dxcommon->GetDevice()->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&csInitPipelineState));
}

void GPUParticleManager::CSUpdateRootSignatureInitialize(DirectXCommon* dxcommon)
{
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature { };
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // CS用はインプットアセンブラ不要

    D3D12_ROOT_PARAMETER rootParameters[4] = { };

    // u0: gOutputVertices (UAV)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // u1
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParameters[1].Descriptor.ShaderRegister = 1;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b0
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[2].Descriptor.ShaderRegister = 0;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b1
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].Descriptor.ShaderRegister = 1;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    csUpdateRootSignature = nullptr;
    hr = dxcommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&csUpdateRootSignature));
}

void GPUParticleManager::CSUpdatePipelineInitialize(DirectXCommon* dxcommon)
{
    CSUpdateRootSignatureInitialize(dxcommon);

    HRESULT hr;

    // CSをコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> computeShaderBlob = dxcommon->CompileShader(L"resources/shaders/EmitParticle.CS.hlsl", L"cs_6_0");
    assert(computeShaderBlob != nullptr);
    // PSOの設定
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc { };
    computePipelineStateDesc.CS = {
        .pShaderBytecode = computeShaderBlob->GetBufferPointer(),
        .BytecodeLength = computeShaderBlob->GetBufferSize()
    };
    computePipelineStateDesc.pRootSignature = csUpdateRootSignature.Get();

    csUpdatePipelineState = nullptr;
    hr = dxcommon->GetDevice()->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&csUpdatePipelineState));
}

void GPUParticleManager::ResourceInitialize()
{
    auto* device = dxCommon->GetDevice();

    // テクスチャ
    TextureManager::getInstance()->LoadTexture("resources/circle2.png");
    uint32_t textureIndex = TextureManager::getInstance()->GetSrvIndex("resources/circle2.png");

    textureSrvIndex = textureIndex;

    // 頂点バッフア
    vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * 6);
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* mappedVertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData));

    // 板ポリ
    mappedVertexData[0] = { { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    mappedVertexData[1] = { { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    mappedVertexData[2] = { { 1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };
    mappedVertexData[3] = { { 1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };
    mappedVertexData[4] = { { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    mappedVertexData[5] = { { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };
    vertexResource->Unmap(0, nullptr);

    // エミッタデータ
    SphereResource = dxCommon->CreateBufferResource(sizeof(EmitterSphere));
    SphereResource->Map(0, nullptr, reinterpret_cast<void**>(&eitterSphere));
    eitterSphere->count = 10;
    eitterSphere->frequency = 0.5f;
    eitterSphere->frequemcyTime = 0.0f;
    eitterSphere->translate = Vector3(0.0f, 0.0f, 0.0f);
    eitterSphere->radius = 1.0f;
    eitterSphere->emit = 0;

    // マテリある
    materialResource = dxCommon->CreateBufferResource(sizeof(ParticleMaterial));
    ParticleMaterial* materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData->enableLighting = false;
    materialData->uvTransform = MakeIdentity4x4();
    materialData->useClampSampler = 0;
    materialResource->Unmap(0, nullptr);

    // カメラ
    perViewResource = dxCommon->CreateBufferResource(sizeof(PerView));

    randomResource = dxCommon->CreateBufferResource(sizeof(PreFrame));
    randomResource->Map(0, nullptr, reinterpret_cast<void**>(&PreFrameData_));
    PreFrameData_->time = 0;
    PreFrameData_->deltaTime = 0;

    // パーティクル構造化バッファ生成
    size_t particleBufferSize = sizeof(Particle) * kMaxParticles;

    D3D12_HEAP_PROPERTIES heapProps { };
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc { };
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = particleBufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&particleResource_));

    // カウンター用バッフア
    size_t counterBufferSize = sizeof(int32_t);

    D3D12_HEAP_PROPERTIES counterHeapProps { };
    counterHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC counterResDesc { };
    counterResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    counterResDesc.Width = particleBufferSize;
    counterResDesc.Height = 1;
    counterResDesc.DepthOrArraySize = 1;
    counterResDesc.MipLevels = 1;
    counterResDesc.Format = DXGI_FORMAT_UNKNOWN;
    counterResDesc.SampleDesc.Count = 1;
    counterResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    counterResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &counterResDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&freeCounterResource));

    instancingSrvIndex = srvManager->Allocate();
    srvManager->CreateSRVforStructuredBuffer(instancingSrvIndex, particleResource_.Get(), kMaxParticles, sizeof(Particle));
}
