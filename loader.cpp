#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include "loader.h"

struct Anim {
    std::string name;
    std::vector<std::vector<std::string>> frames;
    size_t frame = 0;
    Anim(const std::string& n) : name(n), frame(0) {}
};

static std::vector<Anim> anims;
static char *front = nullptr, *back = nullptr;
static int W = 0, H = 0;

#ifdef __linux__
static struct termios old_tio;
#endif

void init() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m; GetConsoleMode(h, &m);
    SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    std::cout << "\033[?25l\033[2J";
#else
    tcgetattr(STDIN_FILENO, &old_tio);
    struct termios new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    new_tio.c_cc[VMIN] = 0;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    std::cout << "\033[?25l\033[2J";
#endif
}

void close() {
    std::cout << "\033[?25h\033[2J" << std::flush;
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
#endif
    delete[] front; delete[] back;
}

Size getSize() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
    return { i.srWindow.Right - i.srWindow.Left + 1, i.srWindow.Bottom - i.srWindow.Top + 1 };
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return { w.ws_col, w.ws_row };
#endif
}

void clear() {
    Size s = getSize();
    if ((s.w ^ W) | (s.h ^ H)) {
        delete[] front; delete[] back;
        W = s.w; H = s.h;
        front = new char[W * H];
        back = new char[W * H];
        memset(front, ' ', W * H);
    }
    memset(back, ' ', W * H);
}

void drawGrid(int camX, int camY) {
    int startX = (camX & ~7) - 8, endX = camX + W + 8;
    int startY = (camY & ~7) - 8, endY = camY + H + 8;
    for (int gx = startX; gx < endX; gx += 8) {
        for (int gy = startY; gy < endY; gy += 8) {
            int x = gx - camX, y = gy - camY;
            if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) back[y * W + x] = '.';
        }
    }
}

void draw(const std::string& file, int x, int y) {
    Anim* a = nullptr;
    for (auto& an : anims) { if (an.name == file) { a = &an; break; } }
    if (!a) {
        anims.push_back(Anim(file)); a = &anims.back();
        std::ifstream f("Frames/" + file);
        if (f.is_open()) {
            std::string line; std::vector<std::string> fr;
            while (std::getline(f, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line == "+") { if (!fr.empty()) a->frames.push_back(fr); fr.clear(); }
                else fr.push_back(line);
            }
            if (!fr.empty()) a->frames.push_back(fr);
        }
    }
    if (a->frames.empty()) return;
    auto& lines = a->frames[a->frame];
    for (size_t i = 0; i < lines.size(); i++) {
        for (size_t j = 0; j < lines[i].size(); j++) {
            int dx = x + (int)j, dy = y + (int)i;
            if ((unsigned)dx < (unsigned)W && (unsigned)dy < (unsigned)H && lines[i][j] != ' ')
                back[dy * W + dx] = lines[i][j];
        }
    }
    a->frame = (a->frame + 1) % a->frames.size();
}

void render() {
    if (memcmp(front, back, W * H) == 0) return;
    memcpy(front, back, W * H);
    std::string out = "\033[H";
    for (int y = 0; y < H; y++) {
        out.append(front + (y * W), W);
        if (y < H - 1) out += '\n';
    }
    std::cout << out << std::flush;
}

InputState getInput() {
    InputState s;
#ifdef _WIN32
    s.up = GetAsyncKeyState(VK_UP) & 0x8000;
    s.down = GetAsyncKeyState(VK_DOWN) & 0x8000;
    s.left = GetAsyncKeyState(VK_LEFT) & 0x8000;
    s.right = GetAsyncKeyState(VK_RIGHT) & 0x8000;
    if (_kbhit()) s.lastChar = (char)_getch();
#else
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == '\033') {
            char seq[2];
            if (read(STDIN_FILENO, seq, 2) == 2 && seq[0] == '[') {
                if (seq[1] == 'A') s.up = true;
                else if (seq[1] == 'B') s.down = true;
                else if (seq[1] == 'C') s.right = true;
                else if (seq[1] == 'D') s.left = true;
            }
        } else s.lastChar = c;
    }
#endif
    return s;
}

void wait(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
