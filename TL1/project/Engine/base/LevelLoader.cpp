#include "LevelLoader.h"
#include "../resources/nlohmann/json.hpp"
#include <cassert>
#include <fstream>



LevelData* LevelLoader::Load(const std::string& DefaultBaseDirectory, const std::string& fileName, const std::string Extension)
{
    // フルパスを得る
    const std::string fullpath = DefaultBaseDirectory + fileName + Extension;

    // ファイルストリーム
    std::ifstream file { };

    file.open(fullpath);
    if (file.fail()) {
        assert(0);
    }

    // Json文字列から解凍したデータ
    nlohmann::json deserialized;

    // 解凍
    file >> deserialized;

    // 正しいレベルデータファイルかチェック
    assert(deserialized.is_object());
    assert(deserialized.contains("name"));
    ;
    assert(deserialized["name"].is_string());

    // "name"を文字列として取得
    std::string name = deserialized["name"].get<std::string>();
    assert(name.compare("scene") == 0);

    // レベルデータ格納用インスタンスを生成
    LevelData* levelData = new LevelData();

    for (nlohmann::json& object : deserialized["objects"]) {
        assert(object.contains("type"));

        // 種類を取得
        std::string type = object["type"].get<std::string>();

        // MESH
        if (type.compare("MESH") == 0) {
            // 要素追加
            levelData->objects.emplace_back(LevelData::ObjectData { });
            // 今追加した要素の照明を得る
            LevelData::ObjectData& objectData = levelData->objects.back();

            if (object.contains("file_name")) {
                // ファイル名
                objectData.fileName = object["file_name"].get<std::string>();

                // トランスフォームのパラメータ読み込み
                nlohmann::json& transform = object["transform"];

                // 平行移動
                objectData.translation.x = (float)transform["translation"][0];
                objectData.translation.y = (float)transform["translation"][2];
                objectData.translation.z = (float)transform["translation"][1];

                // 回転角
                objectData.rotation.x = -(float)transform["rotation"][0];
                objectData.rotation.y = -(float)transform["rotation"][2];
                objectData.rotation.z = -(float)transform["rotation"][1];

                // スケーリング
                objectData.scaling.x = (float)transform["scaling"][0];
                objectData.scaling.y = (float)transform["scaling"][2];
                objectData.scaling.z = (float)transform["scaling"][1];
            }
        }
    }

    return levelData;
}
