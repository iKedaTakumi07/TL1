#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer SepiaParameter : register(b0)
{
    float32_t gIntensity; // 0.0(通常) ～ 1.0(セピア)
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 originalColor = gTexture.Sample(gSampler, input.texcoord);
    float Value = dot(originalColor.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
    float32_t3 sepiaColor = Value * float32_t3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);

    output.color.rgb = lerp(originalColor.rgb, sepiaColor, gIntensity);
    output.color.a = originalColor.a;
    
    return output;
}