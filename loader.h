#ifndef LOADER_H
#define LOADER_H

#include <string>

struct Size
{
  int w, h;
};

void init();
void close();
Size getSize();
void clear();
void drawGrid(int camX, int camY);
void draw(const std::string& file, int x, int y);
void render();
char getKey();
void wait(int ms);

#endif
