#include "Fullscreen.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableEnvironmentMap;
    float32_t shininess;
    float32_t evnironmentCoefficient;
    float32_t time;
    float32_t4x4 uvTransform;
};

SamplerState gSampler : register(s0);
Texture2D<float32_t4> gTexture : register(t0);
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 乱数生成
    float32_t random = rand2dTo1d(input.texcoord * gMaterial.time);
    // 色にする
    output.color.rgb = texColor.rgb * random;
    output.color.a = texColor.a;
    
    
    return output;
}