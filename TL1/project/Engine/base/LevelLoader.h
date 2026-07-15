#pragma once
#include "Math.h"
#include <string>
#include <vector>

struct LevelData {
    struct ObjectData {
        std::string fileName;
        Vector3 translation;
        Vector3 rotation;
        Vector3 scaling;
    };

    std::vector<ObjectData> objects;
};

class LevelLoader {
public:
    // ファイルパス
    static const std::string kDefaultBaseDirectory;
    static const std::string kExtension;

    /// <summary>
    /// レベルデータを読み込む
    /// </summary>
    /// <param name="DefaultBaseDirectory">ディレクトリ</param>
    /// <param name="fileName">ファイル名</param>
    /// <param name="Extension">拡張子</param>
    /// <returns>読み込まれたレベルデータ（要delete）</returns>
    static LevelData* Load(const std::string& DefaultBaseDirectory, const std::string& fileName, const std::string Extension);
};
