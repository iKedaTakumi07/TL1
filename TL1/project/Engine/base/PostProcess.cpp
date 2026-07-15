#include "PostProcess.h"
#include "../../externals/imgui/imgui.h"
#include "../3d/Camera.h"
#include "../3d/Object3d.h"
#include "../base/Logger.h"
#include "Framework.h"
#include "OffscreenSurface.h"
#include "TextureManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <d3d12.h>
#include <wrl.h>

PostProcess* PostProcess::GetInstance()
{
    static std::unique_ptr<PostProcess> instance = std::make_unique<PostProcess>(ConstructorKey());

    return instance.get();
}

void PostProcess::Initialize(DirectXCommon* dxcommon)
{
    dxCommon_ = dxcommon;

    graphicsPipelineInitialize(dxCommon_);

    maskTexturePaths_.clear();

    AddMaskTexture("resources/noise0.png");
    AddMaskTexture("resources/noise1.png");

    // 最初は0番目のテクスチャを選択状態にする
    maskTextureFilePath_ = maskTexturePaths_[selectedMaskIndex_];

    D3D12_HEAP_PROPERTIES heapProps { };
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc { };
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = (sizeof(VignetteData));
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vignetteBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = (sizeof(gIntensity));
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&GrayscaleBuffer_));
    assert(SUCCEEDED(hr));
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&SepiascaleBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = (sizeof(FilterData));
    dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&boxFilterBuffer_));
    dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gaussianFilterBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = sizeof(LuminanceOutlineData);
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&LuminanceBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = sizeof(DepthOutlineData);
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outlineBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = sizeof(BlurData);
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&RadialBlurBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = sizeof(dissolveData);
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&DissolveBuffer_));
    assert(SUCCEEDED(hr));

    resDesc.Width = sizeof(Material);
    hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&RandomBuffer_));

    // マップしてC++から書き込める状態にしておく
    GrayscaleBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&GrayscaleData_));
    SepiascaleBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&SepiascaleData_));
    vignetteBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vignetteMappedData_));
    boxFilterBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&boxFilterMappedData_));
    gaussianFilterBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&gaussianFilterMappedData_));
    outlineBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&outlineMappedData_));
    LuminanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&LuminanceData_));
    RadialBlurBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&RadialBlurData_));
    DissolveBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&DissolveData_));
    RandomBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&RandomMaterialData_));

    // 初期化がだるいっぴ。
    RandomMaterialParam_.color = { 0.0f, 0.0f, 0.0f, 0.0f };
    RandomMaterialParam_.enableEnvironmentMap = 0;
    RandomMaterialParam_.evnironmentCoefficient = 0.0f;
    RandomMaterialParam_.shininess = 0.0f;
    RandomMaterialParam_.uvTransform = MakeIdentity4x4();
    RandomMaterialParam_.time = 0.0f;

    effectOrder_ = {
        EffectType::Grayscale,
        EffectType::Sepiascale,
        EffectType::DepthOutline,
        EffectType::LuminanceOutline,
        EffectType::BoxFilter,
        EffectType::GaussianFilter,
        EffectType::RadialBlur,
        EffectType::Vignette,
        EffectType::Dissolve,
        EffectType::Random,
    };
}

void PostProcess::Update()
{
    DrawImGui();

    /* 反映 */

    if (GrayscaleData_) {
        *GrayscaleData_ = GrayScaleParam_;
    }

    if (SepiascaleData_) {
        *SepiascaleData_ = SepiascaleParam_;
    }

    if (vignetteMappedData_) {
        *vignetteMappedData_ = vignetteParam_;
    }

    if (boxFilterMappedData_) {
        if (boxFilterParam_.kernelSize != boxKernelSize_) {
            boxFilterParam_.kernelSize = boxKernelSize_;
            boxFilterParam_.sigma = 0.0f;
            *boxFilterMappedData_ = boxFilterParam_;
        }
    }

    if (gaussianFilterMappedData_) {
        if (gaussianFilterParam_.kernelSize != gaussianKernelSize_ || gaussianFilterParam_.sigma != gaussianSigma_) {
            gaussianFilterParam_.kernelSize = gaussianKernelSize_;
            gaussianFilterParam_.sigma = gaussianSigma_;
            *gaussianFilterMappedData_ = gaussianFilterParam_;
        }
    }

    if (LuminanceData_) {
        *LuminanceData_ = LuminanceParam;
    }
    if (outlineMappedData_) {
        // カメラから最新のプロジェクション逆行列を取得して転送
        outlineMappedData_->projectionInverse = defaultCamera_->GetProjectionInverse();
        outlineMappedData_->weightMultiplier = weightMultiplierParam;
    }

    if (RadialBlurData_) {
        *RadialBlurData_ = RadialBlurParam;
    }

    if (DissolveData_) {
        *DissolveData_ = dissolveParam_;
    }

    if (RandomMaterialData_) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastTime_;
        lastTime_ = now;
        RandomMaterialParam_.time += elapsed.count();
        // オーバーフロー対策
        RandomMaterialParam_.time = std::fmod(RandomMaterialParam_.time, 100.0f);
        *RandomMaterialData_ = RandomMaterialParam_;
    }
}

void PostProcess::Execute(OffscreenSurface* surfaceA, OffscreenSurface* surfaceB)
{
    OffscreenSurface* currentSource = surfaceA;
    OffscreenSurface* currentDest = surfaceB;

    // リスト順にループ処理
    for (EffectType effect : effectOrder_) {
        bool isEnabled = false;

        switch (effect) {
        case PostProcess::EffectType::Grayscale:
            isEnabled = enableGrayscale_;
            break;
        case PostProcess::EffectType::Sepiascale:
            isEnabled = enableSepiascale_;
            break;
        case PostProcess::EffectType::DepthOutline:
            isEnabled = enableDepthOutLine_;
            break;
        case PostProcess::EffectType::LuminanceOutline:
            isEnabled = enableLuminanceOutLine_;
            break;
        case PostProcess::EffectType::BoxFilter:
            isEnabled = enableBoxFilter_;
            break;
        case PostProcess::EffectType::GaussianFilter:
            isEnabled = enableGaussianFilter_;
            break;
        case PostProcess::EffectType::RadialBlur:
            isEnabled = enableRadialBlur_;
            break;
        case PostProcess::EffectType::Vignette:
            isEnabled = enableVignette_;
            break;
        case PostProcess::EffectType::Dissolve:
            isEnabled = enableDissolve_;
            break;
        case PostProcess::EffectType::Random:
            isEnabled = enableRandom_;
        }

        // 実行しないものをスキップ
        if (!isEnabled)
            continue;

        // バケツリレー
        currentDest->PreDraw();

        // 特定エフェクトの必要なもの
        if (effect == EffectType::DepthOutline) {
            SetDepthSrvHandle(currentSource->GetDepthSRVHandle());
        }

        SetsrvHandle(currentSource->GetSRVHandle());

        // 各エフの描画実行
        switch (effect) {
        case PostProcess::EffectType::Grayscale:
            DrawGrayscale();
            break;
        case PostProcess::EffectType::Sepiascale:
            DrawSepiascale();
            break;
        case PostProcess::EffectType::DepthOutline:
            DrawDepthOutLine();
            break;
        case PostProcess::EffectType::LuminanceOutline:
            DrawLuminanceOutLine();
            break;
        case PostProcess::EffectType::BoxFilter:
            DrawBoxFilterHorizontal();
            currentDest->PostDraw();
            std::swap(currentSource, currentDest);

            currentDest->PreDraw();
            SetsrvHandle(currentSource->GetSRVHandle());
            DrawBoxFilterVertical();
            break;
        case PostProcess::EffectType::GaussianFilter:
            DrawGaussianFilterHorizontal();
            currentDest->PostDraw();
            std::swap(currentSource, currentDest);

            currentDest->PreDraw();
            SetsrvHandle(currentSource->GetSRVHandle());
            DrawGaussianFilterVertical();
            break;
        case PostProcess::EffectType::RadialBlur:
            DrawRadialBlur();
            break;
        case PostProcess::EffectType::Vignette:
            DrawVignette();
            break;

        case PostProcess::EffectType::Dissolve:
            DrawDissolve();
            break;
        case PostProcess::EffectType::Random:
            DrawRandom();
            break;
        }

        // バケツリレー/スワップ
        currentDest->PostDraw();
        std::swap(currentSource, currentDest);
    }
    // 最終実行したSRVをセット
    SetsrvHandle(currentSource->GetSRVHandle());
}

void PostProcess::DrawNormal()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(graphicsPipelineState.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawGrayscale()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateGrayscale_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, GrayscaleBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawSepiascale()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateSepiascale_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, SepiascaleBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawVignette()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateVignette.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, vignetteBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawBoxFilterHorizontal()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateBoxFilterX.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, boxFilterBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawBoxFilterVertical()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateBoxFilterY.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, boxFilterBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawGaussianFilterHorizontal()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateGaussianFilterX.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, gaussianFilterBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawGaussianFilterVertical()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateGaussianFilterY.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, gaussianFilterBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawLuminanceOutLine()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateLuminanceOutLine.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, LuminanceBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawDepthOutLine()
{
    assert(defaultCamera_ != nullptr && "PostProcessにカメラがセットされていません！");

    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateDepthOutLine.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 各パラメータをスロットに合わせてバインド
    cmd->SetGraphicsRootDescriptorTable(0, srvHandle); // [0] カラー (t0)
    cmd->SetGraphicsRootConstantBufferView(1, outlineBuffer_->GetGPUVirtualAddress()); // [1] 逆行列 (b0)
    cmd->SetGraphicsRootDescriptorTable(2, depthSrvHandle); // [2] 深度 (t1)

    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawRadialBlur()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateRadialBlur.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, RadialBlurBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawDissolve()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateDissolve.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, DissolveBuffer_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(2, TextureManager::getInstance()->GetSrvHandelGPU(maskTextureFilePath_));

    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawRandom()
{
    auto cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pipelineStateRandom.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd->SetGraphicsRootDescriptorTable(0, srvHandle);
    cmd->SetGraphicsRootConstantBufferView(1, RandomBuffer_->GetGPUVirtualAddress());
    cmd->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Begin("PostProcess Settings");

    // ラジオボタンをチェックボックスに変更
    ImGui::Checkbox("Grayscale", &enableGrayscale_);
    if (enableGrayscale_) {
        ImGui::Indent(); // ちょっと右にずらす
        ImGui::SliderFloat("intensity##GrayScale", &GrayScaleParam_.intensity, 0.0f, 1.0f, "%.2f");
        ImGui::Unindent();
    }
    ImGui::Checkbox("Sepiascale", &enableSepiascale_);
    if (enableSepiascale_) {
        ImGui::Indent(); // ちょっと右にずらす
        ImGui::SliderFloat("intensity##Sepiascale", &SepiascaleParam_.intensity, 0.0f, 1.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Checkbox("Vignette", &enableVignette_);
    if (enableVignette_) {
        ImGui::Indent(); // ちょっと右にずらす
        ImGui::SliderFloat("Vignette Scale", &vignetteParam_.scale, 0.0f, 32.0f, "%.2f");
        ImGui::SliderFloat("Vignette Exponent", &vignetteParam_.exponent, 0.0f, 5.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Checkbox("BoxFillter", &enableBoxFilter_);
    if (enableBoxFilter_) {
        ImGui::Indent();
        ImGui::SliderInt("Kernel Size##Box", &boxKernelSize_, 1, 31);
        if (boxKernelSize_ % 2 == 0)
            boxKernelSize_++; // 強制的に奇数にする
        ImGui::Unindent();
    }

    ImGui::Checkbox("GaussianFilter", &enableGaussianFilter_);
    if (enableGaussianFilter_) {
        ImGui::Indent();
        ImGui::SliderInt("Kernel Size##Gauss", &gaussianKernelSize_, 1, 31);
        if (gaussianKernelSize_ % 2 == 0)
            gaussianKernelSize_++; // 強制的に奇数にする
        ImGui::SliderFloat("Sigma", &gaussianSigma_, 0.1f, 10.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Checkbox("LuminanceOutline", &enableLuminanceOutLine_);
    if (enableLuminanceOutLine_) {
        ImGui::Indent();
        ImGui::SliderFloat("weightMultiplier##LuminanceOutline", &LuminanceParam.weightMultiplier, 0.0f, 10.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Checkbox("DepthOutline", &enableDepthOutLine_);
    if (enableDepthOutLine_) {
        ImGui::Indent();
        ImGui::SliderFloat("weightMultiplier##DepthOutline", &weightMultiplierParam, 0.0f, 5.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Checkbox("RadialBlur", &enableRadialBlur_);
    if (enableRadialBlur_) {
        ImGui::Indent();
        ImGui::SliderFloat2("kCenter##RadialBlur", &RadialBlurParam.kCenter.x, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("kBlurwidth##RadialBlur", &RadialBlurParam.kBlurwidth, 0.0f, 1.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Checkbox("Dissolve", &enableDissolve_);
    if (enableDissolve_) {
        ImGui::Indent();

        // 1. 閾値（進行度）の調整
        ImGui::SliderFloat("Threshold##Dissolve", &dissolveParam_.gthreshold, 0.0f, 1.0f, "%.2f");

        // 2. エッジ幅の調整
        ImGui::SliderFloat("Edge Width##Dissolve", &dissolveParam_.edgeWidth, 0.001f, 0.2f, "%.3f");

        // 3. 溶けた先の色（背景色）の調整
        ImGui::ColorEdit4("Threshold Color##Dissolve", &dissolveParam_.thresholdcolor.x);

        // 4. エッジの発光色の調整
        ImGui::ColorEdit3("Edge Color##Dissolve", &dissolveParam_.Edegcolor.x);

        // 5. マスクテクスチャを一覧から選べるコンボボックス
        if (ImGui::BeginCombo("Mask Texture##Dissolve", maskTexturePaths_[selectedMaskIndex_].c_str())) {
            for (int i = 0; i < maskTexturePaths_.size(); i++) {
                bool isSelected = (selectedMaskIndex_ == i);
                if (ImGui::Selectable(maskTexturePaths_[i].c_str(), isSelected)) {
                    selectedMaskIndex_ = i;
                    // 選択されたら即座にパスを更新
                    SetDissolveMaskTexture(maskTexturePaths_[i]);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static char newTexturePath[256] = "";
        ImGui::InputText("New Mask Path##Dissolve", newTexturePath, sizeof(newTexturePath));

        ImGui::SameLine(); // ボタンを横並びに
        if (ImGui::Button("Add & Load##Dissolve")) {
            if (strlen(newTexturePath) > 0) {
                // 入力されたパスをリストとテクスチャに書き込む！
                AddMaskTexture(newTexturePath);

                // 入力欄をクリア
                memset(newTexturePath, 0, sizeof(newTexturePath));
            }
        }

        ImGui::Unindent();
    }

    ImGui::Checkbox("RandomFilter", &enableRandom_);

    ImGui::End();
#endif // USE_IMGUI
}

void PostProcess::AddMaskTexture(const std::string& filePath)
{
    auto it = std::find(maskTexturePaths_.begin(), maskTexturePaths_.end(), filePath);

    if (it == maskTexturePaths_.end()) {
        // 1. TextureManagerでテクスチャを読み込む（GPUへの書き込み）
        TextureManager::getInstance()->LoadTexture(filePath);

        // 2. ImGuiなどの選択リスト（std::vector）にパスを書き込む
        maskTexturePaths_.push_back(filePath);

        Logger::Log("Added and Loaded Mask Texture: " + filePath + "\n");
    } else {
        Logger::Log("Texture already exists: " + filePath + "\n");
    }
}

void PostProcess::RootSignatureInitialize(DirectXCommon* dxcommon)
{
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature { };
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = { };
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE descriptorRangeDepth[1] = { };
    descriptorRangeDepth[0].BaseShaderRegister = 1; // t1
    descriptorRangeDepth[0].NumDescriptors = 1;
    descriptorRangeDepth[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeDepth[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //// RootParameter作成
    D3D12_ROOT_PARAMETER rootParameters[3] = { };
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVとして設定
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーから見える
    rootParameters[1].Descriptor.ShaderRegister = 0;

    // 深度テクスチャ
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeDepth;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeDepth);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = { };
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ShaderRegister = 0; // s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // ポイントフィルタ
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].ShaderRegister = 1;
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

void PostProcess::graphicsPipelineInitialize(DirectXCommon* dxcommon)
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

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc { };
    inputLayoutDesc.pInputElementDescs = nullptr;
    inputLayoutDesc.NumElements = 0;

    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc { };
    // 全ての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = false;

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc { };
    // 裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // Shaderをコンパイルする
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxcommon->CompileShader(L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderBlob = dxcommon->CompileShader(L"resources/shaders/Fullscreen.PS.hlsl", L"ps_6_0");
    assert(pixeShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderGrayscale = dxcommon->CompileShader(L"resources/shaders/Grayscale.PS.hlsl", L"ps_6_0");
    assert(pixeShaderGrayscale != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderSepiascale = dxcommon->CompileShader(L"resources/shaders/Sepiascale.PS.hlsl", L"ps_6_0");
    assert(pixeShaderSepiascale != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderVignette = dxcommon->CompileShader(L"resources/shaders/Vignette.PS.hlsl", L"ps_6_0");
    assert(pixeShaderVignette != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderBoxFilterX = dxcommon->CompileShader(L"resources/shaders/BoxFilterX.PS.hlsl", L"ps_6_0");
    assert(pixeShaderBoxFilterX != nullptr);
    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderBoxFilterY = dxcommon->CompileShader(L"resources/shaders/BoxFilterY.PS.hlsl", L"ps_6_0");
    assert(pixeShaderBoxFilterY != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderGaussianFilterX = dxcommon->CompileShader(L"resources/shaders/GaussianFilterX.PS.hlsl", L"ps_6_0");
    assert(pixeShaderGaussianFilterX != nullptr);
    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderGaussianFilterY = dxcommon->CompileShader(L"resources/shaders/GaussianFilterY.PS.hlsl", L"ps_6_0");
    assert(pixeShaderGaussianFilterY != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderLuminanceBasedOutline = dxcommon->CompileShader(L"resources/shaders/LuminanceBasedOutline.PS.hlsl", L"ps_6_0");
    assert(pixeShaderLuminanceBasedOutline != nullptr);
    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderDepthBasedOutline = dxcommon->CompileShader(L"resources/shaders/DepthBasedOutline.PS.hlsl", L"ps_6_0");
    assert(pixeShaderDepthBasedOutline != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderRadialBlur = dxcommon->CompileShader(L"resources/shaders/RadialBlur.PS.hlsl", L"ps_6_0");
    assert(pixeShaderRadialBlur != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderDissolve = dxcommon->CompileShader(L"resources/shaders/Dissolve.PS.hlsl", L"ps_6_0");
    assert(pixeShaderDissolve != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixeShaderRandom = dxcommon->CompileShader(L"resources/shaders/Random.PS.hlsl", L"ps_6_0");
    assert(pixeShaderRandom != nullptr);

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
    depthStencilDesc.DepthEnable = false;
    // 書き込み
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 実際に生成
    HRESULT hr;

    // 通常版
    graphicsPipelineState = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

    // グレースケール
    graphicsPipelineStateDesc.PS = { pixeShaderGrayscale->GetBufferPointer(), pixeShaderGrayscale->GetBufferSize() };
    pipelineStateGrayscale_ = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateGrayscale_));
    assert(SUCCEEDED(hr));

    // セピア調
    graphicsPipelineStateDesc.PS = { pixeShaderSepiascale->GetBufferPointer(), pixeShaderSepiascale->GetBufferSize() };
    pipelineStateSepiascale_ = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateSepiascale_));
    assert(SUCCEEDED(hr));

    // ヴィネッティング
    graphicsPipelineStateDesc.PS = { pixeShaderVignette->GetBufferPointer(), pixeShaderVignette->GetBufferSize() };
    pipelineStateVignette = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateVignette));
    assert(SUCCEEDED(hr));

    // ボックスフィルター(分離可能)
    graphicsPipelineStateDesc.PS = { pixeShaderBoxFilterX->GetBufferPointer(), pixeShaderBoxFilterX->GetBufferSize() };
    pipelineStateBoxFilterX = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateBoxFilterX));
    assert(SUCCEEDED(hr));

    graphicsPipelineStateDesc.PS = { pixeShaderBoxFilterY->GetBufferPointer(), pixeShaderBoxFilterY->GetBufferSize() };
    pipelineStateBoxFilterY = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateBoxFilterY));
    assert(SUCCEEDED(hr));

    // ガウシアンフィルタ(分離可能フィルタ)
    graphicsPipelineStateDesc.PS = { pixeShaderGaussianFilterX->GetBufferPointer(), pixeShaderGaussianFilterX->GetBufferSize() };
    pipelineStateGaussianFilterX = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateGaussianFilterX));
    assert(SUCCEEDED(hr));

    graphicsPipelineStateDesc.PS = { pixeShaderGaussianFilterY->GetBufferPointer(), pixeShaderGaussianFilterY->GetBufferSize() };
    pipelineStateGaussianFilterY = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateGaussianFilterY));
    assert(SUCCEEDED(hr));

    // アウトライン(輝度)
    graphicsPipelineStateDesc.PS = { pixeShaderLuminanceBasedOutline->GetBufferPointer(), pixeShaderLuminanceBasedOutline->GetBufferSize() };
    pipelineStateLuminanceOutLine = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateLuminanceOutLine));
    assert(SUCCEEDED(hr));

    // アウトライン(Depth)
    graphicsPipelineStateDesc.PS = { pixeShaderDepthBasedOutline->GetBufferPointer(), pixeShaderDepthBasedOutline->GetBufferSize() };
    pipelineStateDepthOutLine = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateDepthOutLine));
    assert(SUCCEEDED(hr));

    // ラジアルブラー
    graphicsPipelineStateDesc.PS = { pixeShaderRadialBlur->GetBufferPointer(), pixeShaderRadialBlur->GetBufferSize() };
    pipelineStateRadialBlur = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateRadialBlur));

    // でぃそるぶ
    graphicsPipelineStateDesc.PS = { pixeShaderDissolve->GetBufferPointer(), pixeShaderDissolve->GetBufferSize() };
    pipelineStateDissolve = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateDissolve));

    // ランダム(ノイズ)
    graphicsPipelineStateDesc.PS = { pixeShaderRandom->GetBufferPointer(), pixeShaderRandom->GetBufferSize() };
    pipelineStateRandom = nullptr;
    hr = dxcommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateRandom));

    assert(SUCCEEDED(hr));
}
