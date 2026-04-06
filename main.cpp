#include "loader.h"

int main()
{
    init();

    int px = 20, py = 10;
    int cx = 0, cy = 0;
    const int MARGIN = 8;

    while(true)
{
        Size ws = getSize();
        clear();

        if(px - cx < MARGIN) cx = px - MARGIN;
        if(px - cx > ws.w - MARGIN) cx = px - (ws.w - MARGIN);
        if(py - cy < MARGIN) cy = py - MARGIN;
        if(py - cy > ws.h - MARGIN) cy = py - (ws.h - MARGIN);

        drawGrid(cx, cy);
        draw("ore.txt", 1 - cx, 1 - cy);
        draw("hero.txt", px - cx, py - cy);

        render();

        char key = getKey();
        if(key == 'U') py--;
        if(key == 'D') py++;
        if(key == 'L') px--;
        if(key == 'R') px++;
        if(key == 'q') break;

        wait(30);
    }

    close();
    return 0;
}
