struct VSInput
{
    float4 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};

cbuffer WVP : register(b0)
{
    matrix wvp;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(input.pos, wvp);
    return output;
}