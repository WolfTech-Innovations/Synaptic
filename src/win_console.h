#pragma once
#ifndef WIN_CONSOLE_H
#define WIN_CONSOLE_H

#if defined(_WIN32) || defined(_WIN64) || defined(WINDOWS_BUILD)
    #include <windows.h>
    #include <conio.h>
    #include <string>
    
    // Windows Console state
    namespace WinConsole {
        static HANDLE hConsole = INVALID_HANDLE_VALUE;
        static HANDLE hInput = INVALID_HANDLE_VALUE;
        static int cur_x = 0;
        static int cur_y = 0;
        static int max_y = 24;
        static int max_x = 80;
        static WORD current_attrs = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        static bool initialized = false;
        
        // Color mapping
        inline WORD map_color_pair(int pair) {
            switch(pair) {
                case 1: return FOREGROUND_RED | FOREGROUND_INTENSITY;
                case 2: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                case 3: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                case 4: return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                case 5: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                case 6: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                case 7: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            }
        }
    }
    
    // Curses compatibility layer using Windows Console API
    typedef void* WINDOW;
    #define stdscr ((WINDOW*)1)
    
    // Constants
    #define ERR (-1)
    #define OK (0)
    #define TRUE 1
    #define FALSE 0
    
    inline WINDOW* initscr() {
        WinConsole::hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WinConsole::hInput = GetStdHandle(STD_INPUT_HANDLE);
        
        // Set console mode
        DWORD mode;
        GetConsoleMode(WinConsole::hInput, &mode);
        SetConsoleMode(WinConsole::hInput, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
        
        // Get console size
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(WinConsole::hConsole, &csbi)) {
            WinConsole::max_x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            WinConsole::max_y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
        
        WinConsole::initialized = true;
        return stdscr;
    }
    
    inline int endwin() {
        if (WinConsole::hConsole != INVALID_HANDLE_VALUE) {
            DWORD mode;
            GetConsoleMode(WinConsole::hInput, &mode);
            SetConsoleMode(WinConsole::hInput, mode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        }
        WinConsole::initialized = false;
        return OK;
    }
    
    inline int move(int y, int x) {
        if (!WinConsole::initialized) return ERR;
        COORD coord = {(SHORT)x, (SHORT)y};
        SetConsoleCursorPosition(WinConsole::hConsole, coord);
        WinConsole::cur_x = x;
        WinConsole::cur_y = y;
        return OK;
    }
    
    inline int printw(const char* fmt, ...) {
        if (!WinConsole::initialized) return ERR;
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        
        DWORD written;
        WriteConsoleA(WinConsole::hConsole, buffer, len, &written, NULL);
        WinConsole::cur_x += len;
        return len;
    }
    
    inline int mvprintw(int y, int x, const char* fmt, ...) {
        move(y, x);
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        
        DWORD written;
        WriteConsoleA(WinConsole::hConsole, buffer, len, &written, NULL);
        return len;
    }
    
    inline int addstr(const char* str) {
        if (!WinConsole::initialized) return ERR;
        DWORD written;
        DWORD len = strlen(str);
        WriteConsoleA(WinConsole::hConsole, str, len, &written, NULL);
        WinConsole::cur_x += len;
        return OK;
    }
    
    inline int addch(char ch) {
        if (!WinConsole::initialized) return ERR;
        DWORD written;
        WriteConsoleA(WinConsole::hConsole, &ch, 1, &written, NULL);
        WinConsole::cur_x++;
        return OK;
    }
    
    inline int mvaddstr(int y, int x, const char* str) {
        move(y, x);
        return addstr(str);
    }
    
    inline int mvaddch(int y, int x, char ch) {
        move(y, x);
        return addch(ch);
    }
    
    inline int clear() {
        if (!WinConsole::initialized) return ERR;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD written;
        DWORD size;
        COORD coord = {0, 0};
        
        GetConsoleScreenBufferInfo(WinConsole::hConsole, &csbi);
        size = csbi.dwSize.X * csbi.dwSize.Y;
        
        FillConsoleOutputCharacterA(WinConsole::hConsole, ' ', size, coord, &written);
        FillConsoleOutputAttribute(WinConsole::hConsole, csbi.wAttributes, size, coord, &written);
        SetConsoleCursorPosition(WinConsole::hConsole, coord);
        
        WinConsole::cur_x = 0;
        WinConsole::cur_y = 0;
        return OK;
    }
    
    inline int clrtoeol() {
        if (!WinConsole::initialized) return ERR;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(WinConsole::hConsole, &csbi);
        
        COORD coord = {(SHORT)WinConsole::cur_x, (SHORT)WinConsole::cur_y};
        DWORD written;
        DWORD len = csbi.dwSize.X - WinConsole::cur_x;
        
        FillConsoleOutputCharacterA(WinConsole::hConsole, ' ', len, coord, &written);
        return OK;
    }
    
    inline int clrtobot() {
        if (!WinConsole::initialized) return ERR;
        clrtoeol();
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(WinConsole::hConsole, &csbi);
        
        for (int y = WinConsole::cur_y + 1; y < csbi.dwSize.Y; y++) {
            COORD coord = {0, (SHORT)y};
            DWORD written;
            FillConsoleOutputCharacterA(WinConsole::hConsole, ' ', csbi.dwSize.X, coord, &written);
        }
        return OK;
    }
    
    inline int refresh() {
        // Windows console auto-refreshes
        return OK;
    }
    
    inline int getch() {
        if (!WinConsole::initialized) return ERR;
        if (_kbhit()) {
            return _getch();
        }
        return ERR;
    }
    
    inline int getnstr(char* buf, int n) {
        if (!WinConsole::initialized) return ERR;
        
        // Enable line input temporarily
        DWORD mode;
        GetConsoleMode(WinConsole::hInput, &mode);
        SetConsoleMode(WinConsole::hInput, mode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        
        DWORD read;
        ReadConsoleA(WinConsole::hInput, buf, n - 1, &read, NULL);
        buf[read] = '\0';
        
        // Remove newline
        if (read > 0 && (buf[read-1] == '\n' || buf[read-1] == '\r')) {
            buf[read-1] = '\0';
            if (read > 1 && buf[read-2] == '\r') buf[read-2] = '\0';
        }
        
        // Restore mode
        SetConsoleMode(WinConsole::hInput, mode);
        
        return OK;
    }
    
    inline int cbreak() { return OK; }
    inline int noecho() { return OK; }
    inline int echo() { 
        DWORD mode;
        GetConsoleMode(WinConsole::hInput, &mode);
        SetConsoleMode(WinConsole::hInput, mode | ENABLE_ECHO_INPUT);
        return OK; 
    }
    inline int nodelay(WINDOW*, bool) { return OK; }
    inline int keypad(WINDOW*, bool) { return OK; }
    
    inline int curs_set(int visibility) {
        if (!WinConsole::initialized) return ERR;
        CONSOLE_CURSOR_INFO cci;
        GetConsoleCursorInfo(WinConsole::hConsole, &cci);
        cci.bVisible = (visibility != 0);
        SetConsoleCursorInfo(WinConsole::hConsole, &cci);
        return OK;
    }
    
    inline int timeout(int delay) {
        // Not directly supported, getch handles non-blocking
        return OK;
    }
    
    inline int start_color() { return OK; }
    
    inline int init_pair(short pair, short fg, short bg) {
        return OK;
    }
    
    inline int attron(int attrs) {
        if (attrs & 0xFF00) { // Color pair
            int pair = (attrs >> 8) & 0xFF;
            WinConsole::current_attrs = WinConsole::map_color_pair(pair);
            SetConsoleTextAttribute(WinConsole::hConsole, WinConsole::current_attrs);
        }
        if (attrs & 0x01) { // A_BOLD
            WinConsole::current_attrs |= FOREGROUND_INTENSITY;
            SetConsoleTextAttribute(WinConsole::hConsole, WinConsole::current_attrs);
        }
        return OK;
    }
    
    inline int attroff(int attrs) {
        WinConsole::current_attrs = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        SetConsoleTextAttribute(WinConsole::hConsole, WinConsole::current_attrs);
        return OK;
    }
    
    inline void getmaxyx(WINDOW*, int& y, int& x) {
        y = WinConsole::max_y;
        x = WinConsole::max_x;
    }
    
    // Stub functions for compatibility
    inline int wrefresh(WINDOW*) { return OK; }
    inline int wgetch(WINDOW*) { return getch(); }
    inline int wclear(WINDOW*) { return clear(); }
    inline int box(WINDOW*, int, int) { return OK; }
    inline WINDOW* newwin(int, int, int, int) { return stdscr; }
    inline int delwin(WINDOW*) { return OK; }
    inline int wresize(WINDOW*, int, int) { return OK; }
    inline int mvwin(WINDOW*, int, int) { return OK; }
    inline int scrollok(WINDOW*, bool) { return OK; }
    inline int wscrl(WINDOW*, int) { return OK; }
    inline int getstr(char* buf) { return getnstr(buf, 256); }
    inline int wgetnstr(WINDOW*, char* buf, int n) { return getnstr(buf, n); }
    inline int mvgetnstr(int y, int x, char* buf, int n) { move(y, x); return getnstr(buf, n); }
    
    // Color constants
    #define COLOR_PAIR(n) ((n) << 8)
    #define A_BOLD 0x01
    #define A_REVERSE 0x02
    #define A_NORMAL 0x00
    #define A_STANDOUT 0x04
    #define A_UNDERLINE 0x08
    #define A_DIM 0x10
    #define COLOR_BLACK 0
    #define COLOR_RED 1
    #define COLOR_GREEN 2
    #define COLOR_YELLOW 3
    #define COLOR_BLUE 4
    #define COLOR_MAGENTA 5
    #define COLOR_CYAN 6
    #define COLOR_WHITE 7
    
    // Key constants
    #define KEY_DOWN 80
    #define KEY_UP 72
    #define KEY_LEFT 75
    #define KEY_RIGHT 77
    #define KEY_HOME 71
    #define KEY_BACKSPACE 8
    #define KEY_F0 58
    #define KEY_ENTER 13
    #define KEY_RESIZE 0

#endif // Windows

#endif // WIN_CONSOLE_H