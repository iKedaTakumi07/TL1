#pragma once
#include <Windows.h>
#include <cstdint>
#include <memory>

class WinApp {
public:
    static LRESULT CALLBACK Windowproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
    // コンストラクタに渡すための鍵
    class ConstructorKey {
    private:
        ConstructorKey() = default;
        friend class WinApp;
    };

    // passkeyを受け取るコンストラクタ
    explicit WinApp(ConstructorKey) { }

    // Singleton 取得
    static WinApp* GetInstance();

    // 初期化
    void Initialize();

    // 更新
    void Update();

    // delete
    void Finalize();

    // メッセージ処理
    bool ProcessMessage();

    // getter
    HWND GetHwnd() const { return hwnd; }
    HINSTANCE GetHInstance() const { return wc.hInstance; }

    WinApp(const WinApp&) = delete;
    WinApp& operator=(const WinApp&) = delete;

private:
    ~WinApp() = default;

    friend struct std::default_delete<WinApp>;

public:
    // クライアント領域のサイズ
    static const int32_t KClientWidth = 1280;
    static const int32_t KClientHeight = 720;

private:
    // ウィンドウハンドル
    HWND hwnd = nullptr;
    // ウィンドウクラスの設定
    WNDCLASS wc {};
};
