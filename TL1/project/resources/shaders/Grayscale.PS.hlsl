#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer GrayscaleParameter : register(b0)
{
    float32_t gIntensity;
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
    float32_t3 grayColor = float32_t3(Value, Value, Value);

    output.color.rgb = lerp(originalColor.rgb, grayColor, gIntensity);
    output.color.a = originalColor.a;
    
    return output;
}