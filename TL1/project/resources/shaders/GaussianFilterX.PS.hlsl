#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
cbuffer FilterParameter : register(b0)
{
    int32_t gKernelSize;
    float32_t gSigma; // ぼかしの強さ
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

static const float32_t PI = 3.14159265f;

float Gauss(float x, float sigma)
{
    float exponent = -(x * x) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 usStepSize = float32_t2(rcp(width), 0.0f);
    
    float32_t weight = 0.0f;
    
    PixelShaderOutput output;
    output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
    
    // カーネルサイズ
    int32_t halfKernel = gKernelSize / 2;
    
    for (int32_t x = -halfKernel; x <= halfKernel; ++x)
    {
        // 1次元の重みを計算 (Sigma = 2.0f)
        float32_t w = Gauss((float32_t) x, gSigma);
        weight += w;
        
        // 横方向のみオフセットを適用
        float32_t2 texcoord = input.texcoord + float32_t2(x * usStepSize.x, 0.0f);
        
        output.color.rgb += gTexture.Sample(gSampler, texcoord).rgb * w;
    }
    
    output.color.rgb *= rcp(weight);

    return output;
}