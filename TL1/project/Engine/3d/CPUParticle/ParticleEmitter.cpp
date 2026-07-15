#include "ParticleEmitter.h"
#include "../../scene/SceneManager.h"
#include "CPUParticleManager.h"

ParticleEmitter::ParticleEmitter(const std::string& groupName, const Transform& transform, float emitRate, uint32_t emitCount, bool isLoop)
    : groupName_(groupName)
    , transform_(transform)
    , emitRate_(emitRate)
    , emitCount_(emitCount)
    , elapsedTime_(0.0f)
    , isLoop_(isLoop)
    , hasEmitted_(false)
{
}

void ParticleEmitter::Update()
{
    if (isLoop_) {
        // ループ再生
        if (emitRate_ <= 0.0f) {
            return;
        }

        // 時刻を進める
        elapsedTime_ += SceneManager::GetInstance()->GetDeltaTime();

        // 1回発生するのに必要な時間
        const float emitInterval = 1.0f / emitRate_;

        // 発生可能な回数を計算（余剰時間を保持）
        while (elapsedTime_ >= emitInterval) {
            Emit();
            elapsedTime_ -= emitInterval;
        }
    } else {
        // notループ
        if (!hasEmitted_) {
            Emit();
            hasEmitted_ = true; // 二度と実行されないようにロック
        }
    }
}

void ParticleEmitter::Emit()
{
    CPUParticleManager::getInstance()->Emit(groupName_, transform_, emitCount_, param_);
}
