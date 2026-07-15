#include "CPUParticleManager.h"
#include "../../base/DirectXCommon.h"
#include "../../base/Logger.h"
#include "../../base/SrvManager.h"
#include "../../base/TextureManager.h"
#include "../Camera.h"
#include "IParticleMesh.h"
#include <cassert>
#include <numbers>

CPUParticle MakeNewParticle(std::mt19937& randomEngine, const Transform& translate, const EmitterParam& param)
{
    // [後日]emitter側に細かい指定を保存させて、任意で入力できるようにする。。

    auto randomFloat = [&](float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(randomEngine);
    };

    CPUParticle particle;

    // サイズ
    particle.transform.scale = {
        randomFloat(param.minScale.x, param.maxScale.x),
        randomFloat(param.minScale.y, param.maxScale.y),
        randomFloat(param.minScale.z, param.maxScale.z)
    };

    // 回転
    particle.transform.rotate = {
        randomFloat(param.minRotate.x, param.maxRotate.x),
        randomFloat(param.minRotate.y, param.maxRotate.y),
        randomFloat(param.minRotate.z, param.maxRotate.z)
    };

    // 座標(必要になればランダム化)
    particle.transform.translate = translate.translate;

    // 速度
    particle.velocity = {
        randomFloat(param.minVelocity.x, param.maxVelocity.x),
        randomFloat(param.minVelocity.y, param.maxVelocity.y),
        randomFloat(param.minVelocity.z, param.maxVelocity.z)
    };

    // 色の設定(グラデーション)
    particle.startColor = {
        randomFloat(param.minStartColor.x, param.maxStartColor.x),
        randomFloat(param.minStartColor.y, param.maxStartColor.y),
        randomFloat(param.minStartColor.z, param.maxStartColor.z),
        randomFloat(param.minStartColor.w, param.maxStartColor.w)
    };
    particle.endColor = {
        randomFloat(param.minEndColor.x, param.maxEndColor.x),
        randomFloat(param.minEndColor.y, param.maxEndColor.y),
        randomFloat(param.minEndColor.z, param.maxEndColor.z),
        randomFloat(param.minEndColor.w, param.maxEndColor.w)
    };
    particle.color = particle.startColor;

    // 寿命
    particle.lifeTime = randomFloat(param.minLifeTime, param.maxLifeTime);
    particle.currentTime = 0.0f;
    particle.isInfinite = param.isInfinite;

    return particle;
}

std::unique_ptr<CPUParticleManager> CPUParticleManager::instance = nullptr;

CPUParticleManager* CPUParticleManager::getInstance()
{
    if (instance == nullptr) {
        instance = std::make_unique<CPUParticleManager>(ConstructorKey());
    }
    return instance.get();
}

void CPUParticleManager::Finalize()
{
}

void CPUParticleManager::Initialize(DirectXCommon* DirectXCollision, SrvManager* srvManager)
{
    dxCommon = DirectXCollision;
    this->srvManager = srvManager;
    this->winApp_ = WinApp::GetInstance();

    // ランダムエンジン初期化
    std::random_device seedGenerator;
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    randomEngine = std::mt19937(seedGenerator());

    // パイプライン生成
    graphicsPipelineInitialize(dxCommon);

    // 頂点データの初期化
}

void CPUParticleManager::Update()
{
    const Matrix4x4& viewProjection = Camera_->GetViewProjectionMatrix();
    Matrix4x4 viewMatrix = Inverse(Camera_->GetWorldMatrix());

    Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
    Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, Camera_->GetWorldMatrix());
    billboardMatrix.m[3][0] = 0.0f;
    billboardMatrix.m[3][1] = 0.0f;
    billboardMatrix.m[3][2] = 0.0f;

    // 全パーティクルグループ
    for (auto& [name, group] : particleGroups) {

        // スクロールの計算
        group.uvOffset.x += group.uvScrollSpeed.x * kDeltaTime;
        group.uvOffset.y += group.uvScrollSpeed.y * kDeltaTime;

        // UV行列を送り込む
        ParticleMaterial* materialData = nullptr;
        group.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

        materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        materialData->enableLighting = false;
        materialData->uvTransform = MakeTranslateMatrix({ group.uvOffset.x, group.uvOffset.y, 0.0f });

        // サンプラー再設定
        if (group.meshType == ParticleMeshType::kPlane) {
            materialData->useClampSampler = 0;
        } else {
            materialData->useClampSampler = 1;
        }

        group.materialResource->Unmap(0, nullptr);

        // インスタンス数リセット
        group.instanceCount = 0;

        // インスタンシングバッファ Map
        ParticleForGPU* instancingData = nullptr;
        group.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));

        // グループ内全パーティクル
        for (auto it = group.particles.begin();
            it != group.particles.end();) {

            CPUParticle& particle = *it;

            // 寿命判定
            if (!particle.isInfinite && particle.currentTime >= particle.lifeTime) {
                it = group.particles.erase(it);
                continue;
            }

            // 移動
            particle.transform.translate += particle.velocity * kDeltaTime;

            // グラデーション(有限寿命の場合)
            Vector4 finalColor = particle.startColor;

            if (!particle.isInfinite) {
                // 経過時間を算出
                float t = particle.currentTime / particle.lifeTime;
                if (t > 1.0f)
                    t = 1.0f;

                // スタート色からエンド色へ補間
                finalColor = Lerp(particle.startColor, particle.endColor, t);
            } else {
                // 後に経過時間で変化できるようにする
                finalColor = particle.startColor;
            }

            // 経過時間加算
            particle.currentTime += kDeltaTime;

            // ワールド行列
            Matrix4x4 worldMatrix = MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
            if (useBillboard) {
                worldMatrix = Multiply(worldMatrix, billboardMatrix);
            }
            Matrix4x4 projectionMatrix = MakePrespectiveFovMatrix(0.45f, float(winApp_->KClientWidth) / float(winApp_->KClientHeight), 0.1f, 100.0f);
            Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

            // インスタンシングデータ書き込み
            instancingData[group.instanceCount].world = worldMatrix;
            instancingData[group.instanceCount].WVP = worldViewProjectionMatrix;
            instancingData[group.instanceCount].color = finalColor;

            group.instanceCount++;
            ++it;
        }

        group.instancingResource->Unmap(0, nullptr);
    }
}

void CPUParticleManager::Draw()
{

    auto* commandList = dxCommon->GetCommandList();

    // ルートシグネチャ設定
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    // PSO設定
    commandList->SetPipelineState(graphicsPipelineState.Get());

    // プリミティブトポロジー
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // VBV設定（板ポリ）

    // 全パーティクルグループ
    for (auto& [name, group] : particleGroups) {

        // 生存パーティクルがなければ描画しない
        if (group.instanceCount == 0) {
            continue;
        }

        commandList->IASetVertexBuffers(0, 1, &group.mesh->GetVertexBufferView());

        commandList->SetGraphicsRootConstantBufferView(0, group.materialResource->GetGPUVirtualAddress());

        // テクスチャSRV
        srvManager->SetGraphicsRootDescriptorTable(1, group.instancingSrvIndex);

        // インスタンシングSRV
        srvManager->SetGraphicsRootDescriptorTable(2, group.textureSrvIndex);

        // DrawCall（1グループ = 1回）
        commandList->DrawInstanced(group.mesh->GetVertexCount(), group.instanceCount, 0, 0);
    }
}

void CPUParticleManager::RootSignatureInitialize(DirectXCommon* dxcommon)
{
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
    descriptorRangeForInstancing[0].BaseShaderRegister = 0;
    descriptorRangeForInstancing[0].NumDescriptors = 1;
    descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    // [後日]U,V,M,個別で設定できるようにしたい。(可能なら)

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
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

void CPUParticleManager::graphicsPipelineInitialize(DirectXCommon* dxcommon)
{
    // ルートシグネチャ作成
    RootSignatureInitialize(dxcommon);

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
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

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc {};
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
    D3D12_RASTERIZER_DESC rasterizerDesc {};
    // 裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // Shaderをコンパイルする
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxcommon->CompileShader(L"resources/shaders/CPUParticle.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderBlob = dxcommon->CompileShader(L"resources/shaders/CPUParticle.PS.hlsl", L"ps_6_0");
    assert(pixeShaderBlob != nullptr);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc {};
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
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc {};
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

void CPUParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath, ParticleMeshType meshType)
{
    // 登録済みチェック
    auto it = particleGroups.find(name);
    if (particleGroups.contains(name)) {
        return;
    }

    //  空のグループを作成＆登録
    ParticleGroup group {};

    group.meshType = meshType;
    // 形状タイプに応じて個別クラスを生成
    if (meshType == ParticleMeshType::kPlane) {
        group.mesh = std::make_unique<PlaneMesh>(dxCommon->GetDevice());
    } else if (meshType == ParticleMeshType::kRing) {
        group.mesh = std::make_unique<RingMesh>(dxCommon->GetDevice());
    } else if (meshType == ParticleMeshType::kCylinder) {
        group.mesh = std::make_unique<CylinderMesh>(dxCommon->GetDevice());
    }

    // マテリアルにファイルパス設定
    group.material.textureFilePath = textureFilePath;

    // テクスチャ読み込み
    TextureManager::getInstance()->LoadTexture(textureFilePath);

    // SRVインデクス取得
    uint32_t textureSrvIndex = TextureManager::getInstance()->GetSrvIndex(textureFilePath);

    group.textureSrvIndex = textureSrvIndex;
    group.material.textureIndex = textureSrvIndex;

    // インスタンシング用リソース生成
    group.instancingResource = dxCommon->CreateBufferResource(sizeof(ParticleForGPU) * kMaxInstanceCount);

    // インスタンシング用SRV確保
    group.instancingSrvIndex = srvManager->Allocate();

    // StructuredBuffer用SRV作成
    srvManager->CreateSRVforStructuredBuffer(group.instancingSrvIndex, group.instancingResource.Get(), kMaxInstanceCount, sizeof(ParticleForGPU));

    // マテリアル用のリソースを作る
    group.materialResource = dxCommon->CreateBufferResource(sizeof(ParticleMaterial));
    // マテリアルにデータを書き込む
    ParticleMaterial* materialData = nullptr;
    // 書き込むためのアドレスを取得
    group.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    // 今回は赤を書き込んでみる
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData->enableLighting = false;
    materialData->uvTransform = MakeIdentity4x4();
    // 形状タイプに応じてフラグ変更
    if (meshType == ParticleMeshType::kPlane) {
        materialData->useClampSampler = 0;
    } else if (meshType == ParticleMeshType::kRing) {
        materialData->useClampSampler = 1;
    } else if (meshType == ParticleMeshType::kCylinder) {
        materialData->useClampSampler = 1;
    }

    group.materialResource->Unmap(0, nullptr);

    // 登録
    particleGroups.emplace(name, std::move(group));
}

void CPUParticleManager::Emit(const std::string name, const Transform& transform, uint32_t count, const EmitterParam& param)
{
    auto it = particleGroups.find(name);
    assert(it != particleGroups.end());

    ParticleGroup& group = it->second;

    for (uint32_t i = 0; i < count; ++i) {
        CPUParticle particle = MakeNewParticle(randomEngine, transform, param);
        group.particles.push_back(particle);
    }
}

void CPUParticleManager::SetGroupScrollSpeed(const std::string& name, const Vector2& speed)
{
    auto it = particleGroups.find(name);
    if (it != particleGroups.end()) {
        it->second.uvScrollSpeed = speed;
    }
}
