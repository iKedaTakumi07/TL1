#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
cbuffer FilterParameter : register(b0)
{
    int32_t gKernelSize;
    float32_t gSigma; // Boxでは使わない
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    // テクスチャの幅
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 usStepSize = float32_t2(rcp(width), 0.0f);
    
    PixelShaderOutput output;
    output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
 
    // カーネルサイズの半分
    int32_t halfKernel = gKernelSize / 2;
    // 1ピクセルの重み
    float32_t weight = 1.0f / (float32_t) gKernelSize;
    
    // 横方向（X軸）のみループ
    for (int x = -halfKernel; x <= halfKernel; ++x)
    {
        float2 offset = float2(x * usStepSize.x, 0.0f);
        // 3ピクセルなので重みは 1.0 / 3.0 = 0.33…f
        output.color.rgb += gTexture.Sample(gSampler, input.texcoord + offset).rgb * weight;
    }

    return output;
}