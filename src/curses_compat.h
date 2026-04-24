#pragma once
#ifndef CURSES_COMPAT_H
#define CURSES_COMPAT_H

#if defined(_WIN32) || defined(_WIN64) || defined(WINDOWS_BUILD)
    // Use Windows Console API wrapper
    #include "win_console.h"
#else
    // Linux/Unix - use real ncurses
    #include <ncurses.h>
#endif

#endif // CURSES_COMPAT_H
