#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
cbuffer weightMultiplier : register(b0)
{
    float32_t weightMultiplier;
};
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, },
};

static const float32_t kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVeticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 usStepSize = float32_t2(rcp(width), rcp(height));
    
    // 縦横それぞれの畳み込みの結果を格納する
    float32_t2 differnce = float32_t2(0.0f, 0.0f);
    
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            // texcoordを算出
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * usStepSize;
            // 1/9掛けて足す
            float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            float32_t luminance = Luminance(fetchColor);
            differnce.x += luminance * kPrewittHorizontalKernel[x][y];
            differnce.y += luminance * kPrewittVeticalKernel[x][y];
        }
    }
    
    float32_t weight = length(differnce);
    // cbufferで調整可能に
    weight = saturate(weight * weightMultiplier);
    
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;

    return output;
}