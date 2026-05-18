#include "Utils.h"
#include "Game.h"
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

int main() {
    const std::string dir = applicationDirectory();
    if (!dir.empty()) {
#if defined(_WIN32)
        SetCurrentDirectoryA(dir.c_str());
#else
        (void)chdir(dir.c_str());
#endif
    }
    Game game;
    game.run();
    return 0;
}