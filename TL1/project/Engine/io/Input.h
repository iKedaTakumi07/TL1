#pragma once
#define DIRECTINPUT_VERSION 0x0800

#include "../base/WinApp.h"
#include <Windows.h>
#include <dinput.h>
#include <wrl.h>

class Input {
public:
    // namespace省略
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    // 初期化
    void Initialize();
    // 更新
    void Update();

    bool PushKey(BYTE keyNumber);

    bool TriggerKey(BYTE keyNumber);

private:
    // 全キーの入力状態を取得する
    BYTE key[256] = {};
    BYTE prevKey[256] = {};

    ComPtr<IDirectInputDevice8> keyboard;
    // DirectInputの初期化
    ComPtr<IDirectInput8> directInput;

    // windowsAPI
    WinApp* winApp_ = nullptr;
};
