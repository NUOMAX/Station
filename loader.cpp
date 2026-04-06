#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include "loader.h"

struct Anim
{
    std::string name;
    std::vector<std::vector<std::string>> frames;
    size_t frame = 0;

    Anim(const std::string& n) : name(n), frame(0) {}
};

static std::vector<Anim> anims;
static char **front = nullptr, **back = nullptr;
static int W = 0, H = 0;

#ifdef __linux__
static struct termios old_tio;
#endif

void init()
{
    #ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD m; GetConsoleMode(h, &m);
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        system("cls");
    #else
        tcgetattr(STDIN_FILENO, &old_tio);
        struct termios new_tio = old_tio;
        new_tio.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
        std::cout << "\033[?25l\033[2J";
    #endif
}

void close()
{
    #ifdef _WIN32
        system("cls");
    #else
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
        std::cout << "\033[?25h\033[2J" << std::flush;
    #endif
    for(int i = 0; i < H; i++)
    {
        delete[] front[i]; delete[] back[i];
    }
    delete[] front; delete[] back;
}

Size getSize()
{
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO i;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
        Size s = { i.srWindow.Right - i.srWindow.Left + 1,
                   i.srWindow.Bottom - i.srWindow.Top + 1 };
        return s;
    #else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        Size s = { w.ws_col, w.ws_row };
        return s;
    #endif
}

void clear() {
    Size s = getSize();
    if(s.w != W || s.h != H)
    {
        for(int i = 0; i < H; i++)
        {
            delete[] front[i]; delete[] back[i];
        }
        delete[] front; delete[] back;
        W = s.w; H = s.h;
        front = new char*[H];
        back = new char*[H];
        for(int i = 0; i < H; i++)
        {
            front[i] = new char[W];

            back[i] = new char[W];
            for(int j = 0; j < W; j++)
            {
                front[i][j] = ' ';
                back[i][j] = ' ';
            }
        }
    }
    for(int y = 0; y < H; y++)
    {
        for(int x = 0; x < W; x++)
        {
            back[y][x] = ' ';
        }
    }
}

void drawGrid(int camX, int camY)
{
    for(int gx = (camX/8)*8 - 8; gx < camX + W + 8; gx += 8)
    {
        for(int gy = (camY/8)*8 - 8; gy < camY + H + 8; gy += 8)
        {
            int x = gx - camX;
            int y = gy - camY;

            if(x >= 0 && x < W && y >= 0 && y < H)
            {
                back[y][x] = '.';
            }
        }
    }
}

void draw(const std::string& file, int x, int y)
{
    Anim* a = nullptr;

    for(auto& an : anims)
    {
        if(an.name == file)
        {
            a = &an;
            break;
        }
    }

    if(!a)
    {
        anims.push_back(Anim(file));
        a = &anims.back();
        std::ifstream f("Frames/" + file);

        if(f.is_open())
        {
            std::string line;
            std::vector<std::string> fr;

            while(std::getline(f, line))
            {
                if(line == "+")
                {
                    if(!fr.empty())
                    {
                        a->frames.push_back(fr);
                    }
                    fr.clear();
                }
                else
                {
                    fr.push_back(line);
                }
            }
            if(!fr.empty())
            {
                a->frames.push_back(fr);
            }
            f.close();
        }
    }

    if(a->frames.empty()) return;

    auto& lines = a->frames[a->frame];
    for(size_t i = 0; i < lines.size(); i++)
    {
        for(size_t j = 0; j < lines[i].size(); j++)
        {
            int dx = x + (int)j;
            int dy = y + (int)i;
            if(dx >= 0 && dx < W && dy >= 0 && dy < H && lines[i][j] != ' ')
            {
                back[dy][dx] = lines[i][j];
            }
        }
    }
    a->frame = (a->frame + 1) % a->frames.size();
}

void render()
{
    bool changed = false;
    for(int y = 0; y < H; y++)
    {
        for(int x = 0; x < W; x++)
        {
            if(front[y][x] != back[y][x])
            {
                front[y][x] = back[y][x];
                changed = true;
            }
        }
    }

    if(!changed) return;

    std::string out = "\033[H";
    for(int y = 0; y < H; y++)
    {
        out.append(front[y], W);
        if(y < H-1) out += '\n';
    }
    std::cout << out << std::flush;
}

char getKey()
{
    #ifdef _WIN32
        if(!_kbhit()) return 0;
        int c = _getch();
        if(c == 224) {
            switch(_getch()) {
                case 72: return 'U';
                case 80: return 'D';
                case 75: return 'L';
                case 77: return 'R';
            }
        }
        return (char)c;
    #else
        char c = 0;
        if(read(STDIN_FILENO, &c, 1) > 0)
        {
            if(c == '\033')
            {
                char seq[2];
                if(read(STDIN_FILENO, seq, 2) > 0 && seq[0] == '[')
                {
                    switch(seq[1])
                    {
                        case 'A': return 'U';
                        case 'B': return 'D';
                        case 'C': return 'R';
                        case 'D': return 'L';
                    }
                }
            }
            return c;
        }
        return 0;
    #endif
}

void wait(int ms)
{
    #ifdef _WIN32
        Sleep(ms);
    #else
        usleep(ms * 1000);
    #endif
}
