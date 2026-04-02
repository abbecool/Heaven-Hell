#include <SDL.h>
#include <windows.h>
#include "core/Game.h"
#include "external/json.hpp"
using json = nlohmann::json;

int main(int argc, char* argv[]){   
    Game g("config_files/assets.json", "config_files/text.txt");
    g.run();
    return EXIT_SUCCESS;
}

// WinMain entry point for Windows GUI/console compatibility
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc = 0;
    char **argv  = nullptr;
    return main(argc, argv);
}