#include "Engine/base/Framework.h"
#include "Engine/base/Game.h"
#include <memory>
#include <wrl.h>

// windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    std::unique_ptr<Framework> game;

    game = std::make_unique<Game>();

    game->Run();

    return 0;
}
