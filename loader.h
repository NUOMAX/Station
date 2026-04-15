#ifndef LOADER_H
#define LOADER_H

#include <string>

struct Size
{
    int w, h;
};

struct InputState
{
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    char lastChar = 0;
};

void init();
void close();
void clear();
void render();
void wait(int ms);
void drawGrid(int camX, int camY);
void draw(const std::string& file, int x, int y);
Size getSize();
InputState getInput();

#endif
