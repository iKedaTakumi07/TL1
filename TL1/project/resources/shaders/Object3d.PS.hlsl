#include "object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableEnvironmentMap;
    float32_t shininess;
    float32_t evnironmentCoefficient;
    float32_t time;
    float32_t4x4 uvTransform;
};
struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
    int32_t active;
};
struct Camera
{
    float32_t3 worldPosition;
};
struct PointLight
{
    float32_t4 color; // 色
    float32_t3 position; // 位置
    float intensity; // 輝度
    float radius; // ライトの届く最大距離
    float decay; // 減衰率
    int32_t active;
};
struct SpotLigth
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    float32_t3 direction; // スポットライトの方向
    float32_t distance; // ライトの届く最大距離
    float32_t decay; // 減衰率
    float32_t cosAngle; // スポットライトの余弦
    float32_t cosFalloffStart;
    int32_t active;
};


Texture2D<float32_t4> gTexture : register(t0);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLigth> gSpotLigth : register(b4);
SamplerState gSampler : register(s0);


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
   
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // 早期リターン
    if (textureColor.a == 0.0)
    {
        discard;
    }
    
    // PixelShaderでCameraへの方向を算出
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
     /* DirectionalLight */
    float32_t3 totalDiffuse = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t3 totalSpecular = float32_t3(0.0f, 0.0f, 0.0f);
    
    if (gDirectionalLight.active != 0)
    {
         // half lambert
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
            // 拡散反射
        totalDiffuse += gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
           
            // blinn-phong
        float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
        float NDotH = dot(normalize(input.normal), halfVector);
        float specularPow = pow(saturate(NDotH), gMaterial.shininess);

            // 鏡面反射
        totalSpecular += gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
    }
    
       /* pointLight */
    if (gPointLight.active != 0)
    {
        // 入射光 
        float32_t3 pointLightDirection = normalize(gPointLight.position - input.worldPosition);
        // ポイントライトへの距離
        float32_t distance = length(gPointLight.position - input.worldPosition);
        // 逆二乗則による減衰係数
        float32_t factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);
        
        float NdotL_pt = saturate(dot(input.normal, pointLightDirection));
        totalDiffuse += gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * gPointLight.intensity * factor * NdotL_pt;

        // --- 鏡面反射 (Blinn-Phong) ---
        float32_t3 halfVector_pt = normalize(pointLightDirection + toEye);
        float NDotH_pt = saturate(dot(input.normal, halfVector_pt));
        float specularPow_pt = pow(NDotH_pt, gMaterial.shininess);
        totalSpecular += gPointLight.color.rgb * gPointLight.intensity * factor * specularPow_pt;
    }
    
       /* spotLigth */
    if (gSpotLigth.active != 0)
    {
        float32_t3 spDiffuse = float32_t3(0.0f, 0.0f, 0.0f);
        float32_t3 spSpecular = float32_t3(0.0f, 0.0f, 0.0f);
         // 光源ベクトル
        float32_t3 spotLightDir = normalize(gSpotLigth.position - input.worldPosition);
        float32_t spotLightDistance = length(gSpotLigth.position - input.worldPosition);
          
        // 距離減衰
        float32_t distanceAttenuation = pow(saturate(-spotLightDistance / gSpotLigth.distance + 1.0f), gSpotLigth.decay);
        
        // 角度減衰
        float32_t3 spotLigthDirectionOnSurface = normalize(input.worldPosition - gSpotLigth.position);
        float32_t currentCosAngle = dot(spotLigthDirectionOnSurface, gSpotLigth.direction);
           
        // 内側から外側に向って減衰するようにする
        float32_t falloffFactor = saturate((currentCosAngle - gSpotLigth.cosAngle) / (gSpotLigth.cosFalloffStart - gSpotLigth.cosAngle));
              
        // 全体の減衰率を求める
        float32_t spotLightAttenuation = distanceAttenuation * falloffFactor;
            
        // 拡散反射
        float32_t spotNDotL = saturate(dot(normalize(input.normal), spotLightDir));
        spDiffuse = gMaterial.color.rgb * textureColor.rgb * gSpotLigth.color.rgb * spotNDotL * gSpotLigth.intensity;
        
        // 鏡面反射
        
        // カメラ方向のベクトル/ハーフベクトル
        float32_t3 spotHalfVector = normalize(spotLightDir + toEye);
        float32_t spotNDotH = saturate(dot(normalize(input.normal), spotHalfVector));
        
        // 鏡面反射
        float32_t spotSpecularPow = pow(spotNDotH, gMaterial.shininess);
        spSpecular = gSpotLigth.color.rgb * gSpotLigth.intensity * spotSpecularPow;
            
        
        // 減衰の適応
        totalDiffuse += spDiffuse * spotLightAttenuation;
        totalSpecular += spSpecular * spotLightAttenuation;
    }
    
    /* 環境マップ */
    float32_t3 environmentColor = float32_t3(0.0f, 0.0f, 0.0f);
    if (gMaterial.enableEnvironmentMap != 0)
    {
        float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float32_t4 envSample = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        environmentColor = envSample.rgb * gMaterial.evnironmentCoefficient;
    }
    
    bool isAnyLightActive = (gDirectionalLight.active != 0) ||
                            (gPointLight.active != 0) ||
                            (gSpotLigth.active != 0) ||
                            (gMaterial.enableEnvironmentMap != 0);
    
   // 最終カラー合成
    if (isAnyLightActive)
    {
        // いずれかのライトがONなら、ライティング結果を出力
        output.color.rgb = totalDiffuse + totalSpecular + environmentColor;
    }
    else
    {
        // 全てのライトがOFFなら、そのままのテクスチャ色を出力（Unlit）
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
    }
    output.color.a = gMaterial.color.a * textureColor.a;
    
     // 最終的なαチェック
    if (output.color.a <= 0.0f)
    {
        discard;
    }

    return output;
}