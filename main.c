
#include "emulation/c64.h"

#include "infrastructure/commandline.h"

#include "ui/ncurses_c64.h"
#include "ui/sdl_c64.h"


int main(int argc, char **argv)
{
    bool exit = false;

    if (c64_init("..") != 0) {
        return -1;
    }

    if (commandline_init(&exit) != 0) {
        return -1;
    }

    while (!exit) {
        //ncurses_c64_loop();
        sdl_c64_loop();
        commandline_loop();
    }

    return 0;
}

