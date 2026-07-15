#include "ModelCommon.h"

ModelCommon* ModelCommon::GetInstance()
{
    static std::unique_ptr<ModelCommon> instance = std::make_unique<ModelCommon>(ConstructorKey());

    return instance.get();
}

void ModelCommon::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
}