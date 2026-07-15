#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer VignetteParameter : register(b0)
{
    float32_t gScale; // 16.0fの代わり
    float32_t gExponent; // 0.8fの代わり
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
   
    // 周囲を0に、中心になるほど明るくなるように計算で調整。
    float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    // correctだけで計算すると中心の最大値は0.0625で暗すぎるのでscaleで調整。
    float vignette = correct.x * correct.y * gScale;
    // とりあえず0.8乗でそれっぽく
    vignette = saturate(pow(vignette, gExponent));
    output.color.rgb *= vignette;

    return output;
}