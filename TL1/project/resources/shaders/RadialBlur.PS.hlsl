#include "Fullscreen.hlsli"

struct BlurData
{
    float32_t2 kCenter;
    float32_t kBlurwidth;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<BlurData> gBlurData : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    const float32_t2 kCenter = gBlurData.kCenter; // 中心点
    const int32_t kNumSamples = 10; // サンプリング数。多いと重い。
    const float32_t kBlurWidth = gBlurData.kBlurwidth; //  ぼかしの幅。大きくするとよりボケる(なんでやねん！)
    
    float32_t2 direction = input.texcoord - kCenter;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);
 
    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        // 現在のuvから先ほど計算した方向にサンプリング点を進めながらサンプリングしていく
        float32_t2 texcoord = input.texcoord + direction * kBlurWidth * float32_t(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSampler, texcoord).rgb;
    }
    // 平均化
    outputColor.rgb *= rcp(kNumSamples);
    
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;

    return output;
}