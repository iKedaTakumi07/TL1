#define DIRECTINPUT_VERSION 0x0800

#include "Framework.h"
#include "../2d/SpriteCommon.h"
#include "../3d/Camera.h"
#include "../3d/ModelCommon.h"
#include "../3d/Object3dCommon.h"
#include "../3d/Skybox/SkyBoxCommon.h"
#include "../io/Input.h"
#include "../scene/SceneFactory.h"
#include "../scene/SceneManager.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"

#include "ImGuiManager.h"
#ifdef USE_IMGUI
#include "../externals/imgui/imgui.h"
#endif // USE_IMGUI

#include <wrl.h>

#include "../3d/CPUParticle/CPUParticleManager.h"
#include "../3d/GPUParticleManager.h"
#include "../3d/ModelManager.h"
#include "../audio/Audio.h"
#include "TextureManager.h"
#include <DbgHelp.h>
#include <strsafe.h>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxcompiler.lib")

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* excption)
{
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);
    StringCchPrintf(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
    HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    // processId とクラッシュの発生したthreadedを取得
    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();
    // 設定情報を入力
    MINIDUMP_EXCEPTION_INFORMATION minidumpInformation { 0 };
    minidumpInformation.ThreadId = threadId;
    minidumpInformation.ExceptionPointers = excption;
    minidumpInformation.ClientPointers = TRUE;

    // Dumpを出力
    MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

    return EXCEPTION_EXECUTE_HANDLER;
}

void Framework::Initialize()
{
    // 誰も捕捉しなかった場合に、捕捉する関数を登録
    SetUnhandledExceptionFilter(ExportDump);

    WinApp::GetInstance()->Initialize();

    dxCommon = std::make_unique<DirectXCommon>();
    dxCommon->Initialize();

    input = std::make_unique<Input>();
    input->Initialize();

    srvManager = std::make_unique<SrvManager>();
    srvManager->Initialize(dxCommon.get());

    uint32_t index = dxCommon->AllocateRTVIndex();
    offscreenSurface = std::make_unique<OffscreenSurface>();
    offscreenSurface->Initialize(dxCommon.get(), srvManager.get(), index);

    uint32_t indexB = dxCommon->AllocateRTVIndex(); // 新しいRTVインデックスを取得
    offscreenSurfaceB = std::make_unique<OffscreenSurface>();
    offscreenSurfaceB->Initialize(dxCommon.get(), srvManager.get(), indexB);

    imguiManager = std::make_unique<ImGuiManager>();
    imguiManager->Initialize(dxCommon.get(), srvManager.get());

    SpriteCommon::GetInstance()->Initialize(dxCommon.get());

    Object3dCommon::GetInstance()->Initialize(dxCommon.get(), srvManager.get());

    LightManager::GetInstance()->Initialize();

    ModelCommon::GetInstance()->Initialize(dxCommon.get());

    SkyBoxCommon::GetInstance()->Initialize(dxCommon.get());

    camera = std::make_unique<Camera>();
    camera->SetTranslate({ 0.0f, 4.0f, -10.0f });
    camera->SetRotate({ 0.3f, 0.0f, 0.0f });

    TextureManager::getInstance()->Initialize(dxCommon.get(), srvManager.get());
    ModelManager::GetInstance()->Initialize(dxCommon.get());

    CPUParticleManager::getInstance()->Initialize(dxCommon.get(), srvManager.get());
    CPUParticleManager::getInstance()->SetDefaultCamera(camera.get());

    GPUParticleManager::getInstance()->Initialize(dxCommon.get(), srvManager.get());
    GPUParticleManager::getInstance()->SetDefaultCamera(camera.get());

    PostProcess::GetInstance()->Initialize(dxCommon.get());
    PostProcess::GetInstance()->SetsrvHandle(offscreenSurface->GetSRVHandle());
    PostProcess::GetInstance()->SetDepthSrvHandle(offscreenSurface->GetDepthSRVHandle());
    PostProcess::GetInstance()->SetDefaultCamera(camera.get());

    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

    sceneFactory_ = std::make_unique<SceneFactory>(input.get(), camera.get());
    SceneManager::GetInstance()->SetSceneFactory(std::move(sceneFactory_));

    Audio::GetInstance()->Initialize();
}

void Framework::Run()
{
    Initialize();

    while (true) {

        Update();

        if (IsEndRequst()) {
            break;
        }

        Draw();
    }

    Finalize();
}

void Framework::Update()
{
    if (WinApp::GetInstance()->ProcessMessage()) {
        endRequst_ = true;
    }

    input->Update();

    Vector3 cameraPos = camera->GetTranslate();
    Vector3 cameraRot = camera->GetRotate();

#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();

    ImGui::Begin("Camera Debug");
    ImGui::DragFloat3("Position", &cameraPos.x, 0.1f);
    ImGui::DragFloat3("Rotation", &cameraRot.x, 0.01f);
    ImGui::End();

    LightManager::GetInstance()->DrawImGui();
#endif // USE_IMGUI

    camera->SetTranslate(cameraPos);
    camera->SetRotate(cameraRot);

    camera->Update();

    LightManager::GetInstance()->Update();

    CPUParticleManager::getInstance()->Update();

    GPUParticleManager::getInstance()->Update();
}

void Framework::Finalize()
{

    CloseHandle(dxCommon->GetfenceEvent());

    TextureManager::getInstance()->Finalize();
    ModelManager::GetInstance()->Finalize();
    CPUParticleManager::getInstance()->Finalize();

    imguiManager->Finalize();
}
