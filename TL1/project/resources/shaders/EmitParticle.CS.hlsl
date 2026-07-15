struct EmitterSphere
{
    float32_t3 translate;
    float radius; // 射出半径
    uint32_t count; // 数
    float frequency; // 射出時間
    float frequemcyTime; // 射出間隔調整用時間
    uint32_t emit; // 射出許可
};
struct Particle
{
    float32_t3 translate;
    float32_t3 scale;
    float32_t lifeTime;
    float32_t3 velocity;
    float32_t currentTime;
    float32_t4 color;
};
struct PerFrame
{
    float32_t time;
    float32_t deltaTime;
};

float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719))
{
    //make value smaller to avoid artefacts
    float3 smallValue = sin(value);
    //get scalar value from 3d vector
    float random = dot(smallValue, dotDir);
    //make value more random by making it bigger and then taking the factional part
    random = frac(sin(random) * 143758.5453);
    return random;
}

float3 rand3dTo3d(float3 value)
{
    return float3(
        rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
        rand3dTo1d(value, float3(39.346, 11.135, 83.155)),
        rand3dTo1d(value, float3(73.156, 52.235, 09.151))
    );
}

class RandomGenerator
{
    float32_t3 seed;
    float32_t3 Generate3d()
    {
        seed = rand3dTo3d(seed);
        return seed;
    }
    float32_t Generate1d()
    {
        float32_t result = rand3dTo1d(seed);
        seed.x = result;
        return result;
    }
};

static const uint32_t kMaxParticles = 1024;
ConstantBuffer<EmitterSphere> gEimtter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeCounter : register(u1);

[numthreads(1, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    RandomGenerator generator;
    generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;
    if (gEimtter.emit != 0)
    {
        for (uint32_t countIndex = 0; countIndex < gEimtter.count; ++countIndex)
        {
            int ParticleIndex;
            InterlockedAdd(gFreeCounter[0], 1, ParticleIndex);
            // 最大数よりもParticleの数が少なければ射出可能
            if (ParticleIndex < kMaxParticles)
            {
                // カウント分Particleを射出する
                gParticles[ParticleIndex].scale = generator.Generate3d();
                gParticles[ParticleIndex].translate = generator.Generate3d();
                gParticles[ParticleIndex].color.rgb = generator.Generate3d();
                gParticles[ParticleIndex].color.a = 1.0f;
            }
           
        }

    }
}