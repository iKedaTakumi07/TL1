#pragma once
#include <fstream>
#include <wrl.h>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

class Sound;

class Audio {
public:
    // Singleton 取得
    static Audio* GetInstance();
  
    Audio() = default;
    ~Audio() { Finalize(); }

    /// <summary>
    /// 初期化
    /// </summary>
    /// <returns></returns>
    bool Initialize();
    void Finalize();

    void Play(const Sound& sound);

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

private:
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;
};
