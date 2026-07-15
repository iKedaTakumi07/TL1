#pragma comment(lib, "mfplat.lib")

#include "Audio.h"
#include "Sound.h"
#include <cassert>
#include <mfapi.h>

Audio* Audio::GetInstance()
{
    static Audio instance;
    return &instance;
}

bool Audio::Initialize()
{
    HRESULT result;

    result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    assert(SUCCEEDED(result));

    HRESULT hr = XAudio2Create(&xAudio2_, 0);
    if (FAILED(hr))
        return false;

    hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
    return SUCCEEDED(hr);
}

void Audio::Finalize()
{
    if (masterVoice_) {
        masterVoice_->DestroyVoice();
        masterVoice_ = nullptr;
    }
    xAudio2_.Reset();

    HRESULT result;

    result = MFShutdown();
    assert(SUCCEEDED(result));
}

void Audio::Play(const Sound& sound)
{
    IXAudio2SourceVoice* sourceVoice = nullptr;

    HRESULT hr = xAudio2_->CreateSourceVoice(
        &sourceVoice,
        &sound.GetSoundData().wfex);
    assert(SUCCEEDED(hr));

    XAUDIO2_BUFFER buffer {};
    buffer.pAudioData = sound.GetSoundData().buffer.data();
    buffer.AudioBytes = static_cast<UINT32>(sound.GetSoundData().buffer.size());
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    hr = sourceVoice->SubmitSourceBuffer(&buffer);
    assert(SUCCEEDED(hr));

    hr = sourceVoice->Start();
    assert(SUCCEEDED(hr));
}