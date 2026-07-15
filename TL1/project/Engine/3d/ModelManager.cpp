#include "ModelManager.h"
#include "../base/DirectXCommon.h"
#include "Model.h"
#include "ModelCommon.h"
#include <cassert>

std::unique_ptr<ModelManager> ModelManager::instance = nullptr;
bool ModelManager::finalized = false;

void ModelManager::LoadModel(const std::string& filePath)
{
    if (models.contains(filePath)) {
        return;
    }

    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize("resources", filePath);

    models.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
    if (models.contains(filePath)) {
        return models.at(filePath).get();
    }

    // 一致なし
    return nullptr;
}

ModelManager* ModelManager::GetInstance()
{
    // Finalize後に呼ばれた場合はエラー
    assert(!finalized && "ModelManager was already finalized!");

    if (instance == nullptr) {
        instance = std::make_unique<ModelManager>(ConstructorKey());
    }
    return instance.get();
}

void ModelManager::Finalize()
{
    models.clear(); // 解放
}

void ModelManager::Initialize(DirectXCommon* dxCommon)
{
    ModelCommon::GetInstance()->Initialize(dxCommon);
}