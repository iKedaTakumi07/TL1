#include "Fullscreen.hlsli"

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct OutlineData
{
    float32_t4x4 projectionInverse;
    float32_t weightMultiplier;
};

ConstantBuffer<OutlineData> gOutlineData : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

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
            
            float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            // NDC -> View。
            float32_t4 viewSpace = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f), gOutlineData.projectionInverse);
            float32_t viewZ = viewSpace.z * rcp(viewSpace.w);
            

            differnce.x += viewZ * kPrewittHorizontalKernel[x][y];
            differnce.y += viewZ * kPrewittVeticalKernel[x][y];
        }
    }
    
    float32_t weight = length(differnce);
    // cbufferで調整可能に
    weight = saturate(weight * gOutlineData.weightMultiplier);
    
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;

    return output;
}