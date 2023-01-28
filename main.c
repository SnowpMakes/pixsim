#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <time.h>

#include "color.h"
#include "vector.h"
#include "block.h"
#include "world.h"

#define WIDTH 320
#define HEIGHT 200
#define ZOOM 4

#define SIMWIDTH (WIDTH + 2)
#define SIMHEIGHT (HEIGHT + 1)

void *pixels;

double get_secs(void)
{
    struct timespec ts;
    /* C11. Use this so we can get completely rid of SDL to benchmark the CPU. */
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec + (1e-9 * ts.tv_nsec);
    /*return SDL_GetTicks() / 1000.0;*/
}

TTF_Font *font;

SDL_Texture *renderText(SDL_Renderer *renderer, char *message, SDL_Color color)
{
    //We need to first render to a surface as that's what TTF_RenderText
    //returns, then load that surface into a texture
    SDL_Surface *surf = TTF_RenderText_Blended(font, message, color);
    if (surf == NULL)
    {
        printf("Could not create surface for text!\n");
        return NULL;
    }

    SDL_Texture *text_ure = SDL_CreateTextureFromSurface(renderer, surf);
    if (text_ure == NULL)
    {
        printf("Could not create texture from surface!\n");
        return NULL;
    }

    SDL_FreeSurface(surf);
    return text_ure;
}

void drawText(SDL_Renderer *renderer, char *message, SDL_Color color, int x, int y)
{
    SDL_Texture *text_ure = renderText(renderer, message, color);
    int tw, th;
    if (SDL_QueryTexture(text_ure, NULL, NULL, &tw, &th) != 0)
    {
        printf("Could not query text texture!\n");
    }
    SDL_RenderCopy(renderer, text_ure, NULL, &(SDL_Rect) {x, y, tw, th});
    SDL_DestroyTexture(text_ure);
}

void simulate(World *w)
{
    //printf("Simulation running..\n");
    BlockEntry *e = w->blockHeader->next;
    Block *b;
    while (e != w->blockTrailer)
    {
        b = e->block;

        if (b->gravity)
        {
            switch (b->type)
            {
                case CONCRETE:
                {
                    Block *under;
                    getBlock(w, b->location.x, b->location.y - 1, &under);
                    if (under == NULL)
                    {
                        moveBlock(w, b, b->location.x, b->location.y - 1);
                    }
                }
                    break;
                case SAND:
                {
                    Block *lu, *l, *u, *r, *ru;
                    getBlock(w, b->location.x - 1, b->location.y - 1, &lu);
                    getBlock(w, b->location.x - 1, b->location.y, &l);
                    getBlock(w, b->location.x, b->location.y - 1, &u);
                    getBlock(w, b->location.x + 1, b->location.y, &r);
                    getBlock(w, b->location.x + 1, b->location.y - 1, &ru);

                    if (u == NULL)
                    {
                        moveBlock(w, b, b->location.x, b->location.y - 1);
                    }
                    else if (u->type == WATER)
                    {
                        // Sink underwater
                        swapBlockLocations(w, b, u);

                        if (l == NULL && r == NULL)
                        {
                            int move = u->location.x - 1;
                            if (rand() % 2) move = u->location.x + 1;
                            moveBlock(w, u, move, u->location.y);
                        }
                        else if (l == NULL) moveBlock(w, u, u->location.x - 1, u->location.y);
                        else if (r == NULL) moveBlock(w, u, u->location.x + 1, u->location.y);
                    }
                    else if (lu == NULL)
                    {
                        moveBlock(w, b, b->location.x - 1, b->location.y - 1);
                    }
                    else if (ru == NULL)
                    {
                        moveBlock(w, b, b->location.x + 1, b->location.y - 1);
                    }
                    else if (lu->type == WATER)
                    {
                        swapBlockLocations(w, b, lu);
                    }
                    else if (ru->type == WATER)
                    {
                        swapBlockLocations(w, b, ru);
                    }
                }
                    break;
                case WATER:
                {
                    Block *l, *u, *r;
                    getBlock(w, b->location.x - 1, b->location.y, &l);
                    getBlock(w, b->location.x, b->location.y - 1, &u);
                    getBlock(w, b->location.x + 1, b->location.y, &r);

                    if (u == NULL)
                    {
                        moveBlock(w, b, b->location.x, b->location.y - 1);
                    }
                    else if (l == NULL && r == NULL)
                    {
                        moveBlock(w, b, b->location.x + (((rand() % 2) * 2) - 1), b->location.y);
                    }
                    else if (l == NULL)
                    {
                        moveBlock(w, b, b->location.x - 1, b->location.y);
                    }
                    else if (r == NULL)
                    {
                        moveBlock(w, b, b->location.x + 1, b->location.y);
                    }
                }
                    break;
            }
        }

        e = e->next;
    }

}

void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint8_t *base;

    for (int oy = 0; oy < ZOOM; oy++)
        for (int ox = 0; ox < ZOOM; ox++)
        {
            base = ((uint8_t *) pixels) + (4 * ((y * ZOOM + oy) * WIDTH * ZOOM + (x * ZOOM) + ox));
            base[0] = r;
            base[1] = g;
            base[2] = b;
            base[3] = a;
        }
}

void draw_pixel_c(int x, int y, Color c)
{
    draw_pixel(x, y, c.r, c.g, c.b, 255);
}

void render(World *w)
{
    BlockEntry *e = w->blockHeader->next;
    Block *b;
    while (e != w->blockTrailer)
    {
        b = e->block;
        if (b->location.x >= 0 && b->location.x < WIDTH && b->location.y >= 0 && b->location.y < HEIGHT)
        {
            draw_pixel_c(b->location.x, HEIGHT - b->location.y - 1, b->color);
        }
        e = e->next;
    }
}



int main()
{
    srand((unsigned) time(NULL));

    World *w;
    createWorld(&w, WIDTH, HEIGHT);

    Block *b;

    //addBlock(&b, SAND, 0, 180);
    //b->color = (Color) {255, 255, 255};

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    font = TTF_OpenFont("../assets/Arial.ttf", 16);
    if (font == NULL)
    {
        printf("Could not open font!\n");
        return 1;
    }

    SDL_CreateWindowAndRenderer(WIDTH * ZOOM, HEIGHT * ZOOM, 0, &window, &renderer);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                             WIDTH * ZOOM, HEIGHT * ZOOM);

    double oldTime = 0;
    double timeNow = 0;
    struct timespec startTime, endTime;

    int pitch;
    SDL_Event event;

    int mouseLDown = 0;
    int mouseRDown = 0;
    int mDown = 0;
    int qDown = 0;
    int raining = 0;
    int simulationPaused = 0;
    int brushSize = 1;
    int brushGravity = 1;

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);

        SDL_LockTexture(texture, NULL, &pixels, &pitch);

        if (!simulationPaused) simulate(w);
        if (mouseLDown || mouseRDown)
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            mx /= ZOOM;
            my /= ZOOM;
            my = HEIGHT - 1 - my;
            if (mx >= 0 && my >= 0 && mx < WIDTH && my < HEIGHT)
            {
                //printf("Spawn %d %d\n", mx, my);
                getBlock(w, mx, my, &b);
                //printf("%d\n", b != NULL);

                if (b != NULL)
                {
                    deleteBlock(w, b);
                }

                Color nc;
                BlockType nt;
                if (mouseLDown && qDown)
                {
                    nc = (Color) {0, 0, 255};
                    nt = WATER;
                }
                else if (mouseLDown)
                {
                    nc = (Color) {255, 0, 0};
                    nt = CONCRETE;
                }
                else
                {
                    nc = (Color) {0, 255, 0};
                    nt = SAND;
                }

                PairInt brushBlocks[brushSize * brushSize * 4];
                int brushc = 0;
                brushBlocks[brushc++] = (PairInt) {0, 0};
                for (int i = 1; i < brushSize; i++)
                {
                    brushBlocks[brushc++] = (PairInt) {-i, 0};
                    brushBlocks[brushc++] = (PairInt) {i, 0};
                }
                for (int j = 1; j < brushSize; j++)
                {
                    brushBlocks[brushc++] = (PairInt) {0, j};
                    brushBlocks[brushc++] = (PairInt) {0, -j};
                    for (int i = 1; i < brushSize - j; ++i)
                    {
                        brushBlocks[brushc++] = (PairInt) {-i, j};
                        brushBlocks[brushc++] = (PairInt) {i, j};
                        brushBlocks[brushc++] = (PairInt) {-i, -j};
                        brushBlocks[brushc++] = (PairInt) {i, -j};
                    }
                }

                for (int i = 0; i < brushc; ++i)
                {
                    PairInt l = brushBlocks[i];
                    int lx = l.x, ly = l.y;
                    int bx = mx + lx, by = my + ly;
                    if (bx >= 0 && by >= 0 && bx < WIDTH && by < HEIGHT)
                    {
                        if (mDown)
                        {
                            getBlock(w, bx, by, &b);
                            if (b != NULL) deleteBlock(w, b);
                        }
                        else
                        {
                            addBlock(w, &b, nt, bx, by);
                            b->color = (Color) {nc.r, nc.g, nc.b};
                            b->gravity = brushGravity;
                        }
                    }
                }
            }
        }
        if (raining)
        {
            int x1, x2;
            Block *bw1, *bw2;

            x1 = rand() % WIDTH;
            do
            {
                x2 = rand() % WIDTH;
            } while (x2 == x1);

            addBlock(w, &bw1, WATER, x1, HEIGHT - 1);
            addBlock(w, &bw2, WATER, x2, HEIGHT - 1);
            bw1->color = (Color) {0, 0, 255};
            bw2->color = (Color) {0, 0, 255};
        }
        render(w);

        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        oldTime = timeNow;
        timeNow = get_secs();
        double frameTime = timeNow - oldTime; //frameTime is the timeNow this frame has taken, in seconds
        printf("FPS = %f\n", 1 / frameTime);

        char fpsText[10];
        sprintf(fpsText, "%d FPS", (int) round(1 / frameTime));
        drawText(renderer, fpsText, (SDL_Color) {255, 255, 255, 255}, 10, 10);

        char brushSizeText[15];
        sprintf(brushSizeText, "Brush size: %d", brushSize);
        drawText(renderer, brushSizeText, (SDL_Color) {255, 255, 255, 255}, 10, 30);

        char brushGravityText[25];
        sprintf(brushGravityText, "Brush gravity: %s", brushGravity ? "enabled" : "disabled");
        drawText(renderer, brushGravityText, (SDL_Color) {255, 255, 255, 255}, 10, 50);

        if (simulationPaused)
        {
            char simulationPausedText[] = "Simulation Paused";
            SDL_Texture *sp_texture = renderText(renderer, simulationPausedText, (SDL_Color) {255, 255, 255, 255});
            int tw, th;
            if (SDL_QueryTexture(sp_texture, NULL, NULL, &tw, &th) != 0)
            {
                printf("Could not query text texture!\n");
            }
            SDL_RenderCopy(renderer, sp_texture, NULL, &(SDL_Rect) {(WIDTH * ZOOM) - 1 - 10 - tw, 10, tw, th});
            SDL_DestroyTexture(sp_texture);
        }

        SDL_RenderPresent(renderer);

        int quit = 0;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) mouseLDown = 1;
                    if (event.button.button == SDL_BUTTON_RIGHT) mouseRDown = 1;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) mouseLDown = 0;
                    if (event.button.button == SDL_BUTTON_RIGHT) mouseRDown = 0;
                    break;
                case SDL_MOUSEWHEEL:
                    brushSize += (event.wheel.y / 3);
                    if (brushSize > 10) brushSize = 10;
                    if (brushSize < 1) brushSize = 1;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_m:
                            mDown = 1;
                            break;
                        case SDLK_q:
                            qDown = 1;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_a:
                            raining = !raining;
                            break;
                        case SDLK_r:
                            resetWorld(w);
                            break;
                        case SDLK_p:
                            simulationPaused = !simulationPaused;
                            break;
                        case SDLK_g:
                            brushGravity = !brushGravity;
                            break;
                        case SDLK_m:
                            mDown = 0;
                            break;
                        case SDLK_q:
                            qDown = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            if (quit) break;
        }
        if (quit) break;

        clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
        struct timespec req = {
                0,
                (1000000000 / 65) -
                ((endTime.tv_sec - startTime.tv_sec) * 1000000000 + (endTime.tv_nsec - startTime.tv_nsec))
        };
        //printf("Sleep %ld\n", req.tv_nsec);
        struct timespec rem;
        nanosleep(&req, &rem);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
