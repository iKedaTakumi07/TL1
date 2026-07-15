#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")

#include <Windows.h>

#include <mfidl.h>

#include <mfapi.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mfreadwrite.h>

#include <cassert>
#include <wrl.h>

#include "Sound.h"
#include "../base/StringUtility.h"

void Sound::SoundLoadFile(const std::string& filename)
{

    // フルパスをワイド文字に変換
    std::wstring filePathW = StringUtility::ConvertString(filename);
    HRESULT result;

    // SoundReader作成
    Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
    result = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &pReader);
    assert(SUCCEEDED(result));

    // PCM形式にフォーマット指定する
    Microsoft::WRL::ComPtr<IMFMediaType> pPCMType;
    MFCreateMediaType(&pPCMType);
    pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    result = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());
    assert(SUCCEEDED(result));

    // 実際にセットされたメディアタイプを取得する
    Microsoft::WRL::ComPtr<IMFMediaType> pOutType;
    pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);

    // Waveフォーマットを取得する
    WAVEFORMATEX* waveFormat = nullptr;
    MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);

    // コンテナに格納する音声データ
    soundData.wfex = *waveFormat;

    CoTaskMemFree(waveFormat);

    // PCMデータのバッファを構築
    while (true) {
        Microsoft::WRL::ComPtr<IMFSample> pSample;
        DWORD streamIndex = 0, flags = 0;
        LONGLONG llTimeStamp = 0;
        // サンプルを読み込む
        result = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);
        // ストリームの末尾に達したら抜ける
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            break;
        if (pSample) {
            Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
            // サンプルに含まれるサウンドデータのバッファを一繋ぎにして取得
            pSample->ConvertToContiguousBuffer(&pBuffer);

            BYTE* pData = nullptr;
            DWORD maxLength = 0, currentLength = 0;
            // バッファ読み込み用にロック
            pBuffer->Lock(&pData, &maxLength, &currentLength);
            // バッファの末尾にデータを追加
            soundData.buffer.insert(soundData.buffer.end(), pData, pData + currentLength);
            pBuffer->Unlock();
        }
    }
}

void Sound::Unload()
{
    soundData.buffer.clear();
    soundData.wfex = {};
}
