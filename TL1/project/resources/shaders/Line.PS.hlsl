struct VSOutput
{
    float4 pos : SV_POSITION;
};

float4 main(VSOutput input) : SV_TARGET
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f); // 蛍光グリーンで描画
}