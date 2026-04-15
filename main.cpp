#include "loader.h"

int main()
{
    init();

    float px = 20.0f, py = 10.0f;
    float cx = 0.0f, cy = 0.0f;
    const int MARGIN = 8;

    while (true)
    {
        Size ws = getSize();
        clear();

        if (px - cx < MARGIN) cx = px - MARGIN;
        if (px - cx > ws.w - MARGIN) cx = px - (ws.w - MARGIN);
        if (py - cy < MARGIN) cy = py - MARGIN;
        if (py - cy > ws.h - MARGIN) cy = py - (ws.h - MARGIN);

        drawGrid((int)cx, (int)cy);
        draw("ore.txt", 1 - (int)cx, 1 - (int)cy);
        draw("hero.txt", (int)px - (int)cx, (int)py - (int)cy);

        render();

        InputState input = getInput();

        if (input.up)    py -= 0.6f;
        if (input.down)  py += 0.6f;
        if (input.left)  px -= 1.0f;
        if (input.right) px += 1.0f;

        if (input.lastChar == 'q') break;

        wait(30);
    }

    close();
    return 0;
}
