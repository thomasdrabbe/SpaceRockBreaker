#if defined(_WIN32)
#include "Utils.h"
#include <windows.h>
#endif

#include "Game.h"

int main() {
#if defined(_WIN32)
    const std::string dir = applicationDirectory();
    if (!dir.empty())
        SetCurrentDirectoryA(dir.c_str());
#endif
    Game game;
    game.run();
    return 0;
}