#pragma once
#include <fstream>
#include <vector>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

struct ChunkHeader {
    char id[4]; // チャンク毎のID
    int32_t size; // チャンクサイズ
};
struct RiffHeader {
    ChunkHeader chunk; // "RIFF"
    char type[4]; // "WAVE"
};
struct FormatChunk {
    ChunkHeader chunk; // "fmt"
    WAVEFORMATEX fmt; // 波型フォーマット
};
struct SoundData {
    // 波型フォーマット
    WAVEFORMATEX wfex;
    // バッファ
    std::vector<BYTE> buffer;
};

class Sound {
public:
    Sound() = default;
    ~Sound() { Unload(); }

    void SoundLoadFile(const std::string& filename);
    void Unload();

    const SoundData& GetSoundData() const { return soundData; }

private:
    SoundData soundData;
};
