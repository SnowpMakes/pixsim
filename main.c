#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <time.h>

#include "structs.h"

#define WIDTH 320
#define HEIGHT 200
#define ZOOM 4

#define SIMWIDTH (WIDTH + 2)
#define SIMHEIGHT (HEIGHT + 1)


BlockEntry *worldHead;
BlockEntry *worldTail;
BlockEntry **worldLoc;
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

BlockEntry *getBlockEntry(int x, int y)
{
    if (x < -1 || x > WIDTH || y < -1 || y >= HEIGHT)
    {
        printf("Invalid call to getBlockEntry!\n");
        exit(1);
    }
    return *(worldLoc + ((y + 1) * SIMWIDTH) + x + 1);
}

void setBlockEntry(int x, int y, BlockEntry *entry)
{
    if (x < -1 || x > WIDTH || y < -1 || y >= HEIGHT)
    {
        printf("Invalid call to setBlockEntry!\n");
        exit(1);
    }
    *(worldLoc + ((y + 1) * SIMWIDTH) + x + 1) = entry;
}

void getBlock(int x, int y, Block **b)
{
    if (x < -1 || x > WIDTH || y < -1 || y >= HEIGHT)
    {
        *b = NULL;
        return;
    }
    BlockEntry *be = getBlockEntry(x, y);
    if (be == NULL) *b = NULL;
    else *b = be->block;
}

void createBlock(Block **b, BlockType t, int x, int y)
{
    if (x < -1 || x > WIDTH || y < -1 || y >= HEIGHT)
    {
        *b = NULL;
        return;
    }

    Block *nb;
    getBlock(x, y, &nb);
    if (nb != NULL)
    {
        //printf("Attempt to create Block which already existed at %d %d! Returned existing Block.\n", x, y);
        nb->type = t;
        *b = nb;
        return;
    }

    nb = malloc(sizeof(Block));
    nb->type = t;
    nb->location = (PairInt) {x, y};
    nb->gravity = 1;

    BlockEntry *tailp = worldTail->prev;
    BlockEntry *newbe = malloc(sizeof(BlockEntry));
    tailp->next = newbe;
    worldTail->prev = newbe;
    newbe->prev = tailp;
    newbe->next = worldTail;

    newbe->block = nb;

    setBlockEntry(x, y, newbe);
    nb->entry = newbe;

    *b = nb;
}

void moveBlock(Block *b, int x, int y)
{
    if (getBlockEntry(b->location.x, b->location.y) != b->entry)
    {
        printf("World location mapping <-> block entry mismatch during moveBlock!\n");
        exit(1);
    }
    setBlockEntry(b->location.x, b->location.y, NULL);
    b->location = (PairInt) {x, y};
    setBlockEntry(b->location.x, b->location.y, b->entry);
}

void swapBlockLocations(Block *b1, Block *b2)
{
    if (getBlockEntry(b1->location.x, b1->location.y) != b1->entry)
    {
        printf("World location mapping <-> block entry mismatch during swapBlockLocations b1!\n");
        exit(1);
    }
    if (getBlockEntry(b2->location.x, b2->location.y) != b2->entry)
    {
        printf("World location mapping <-> block entry mismatch during swapBlockLocations b2!\n");
        exit(1);
    }

    PairInt l1 = b1->location;
    PairInt l2 = b2->location;

    b1->location = (PairInt) {l2.x, l2.y};
    b2->location = (PairInt) {l1.x, l1.y};
    setBlockEntry(b1->location.x, b1->location.y, b1->entry);
    setBlockEntry(b2->location.x, b2->location.y, b2->entry);
}

void deleteBlock(Block *b)
{
    BlockEntry *e = b->entry;
    if (e == NULL)
    {
        printf("Attempted to delete Block with NULL as entry!! What went wrong?!");
        exit(1);
    }

    setBlockEntry(b->location.x, b->location.y, NULL);

    free(b);

    BlockEntry *p = e->prev;
    BlockEntry *n = e->next;
    p->next = n;
    n->prev = p;
    free(e);
}

void simulate()
{
    //printf("Simulation running..\n");
    BlockEntry *e = worldHead->next;
    Block *b;
    while (e != worldTail)
    {
        b = e->block;

        if (b->gravity)
        {
            switch (b->type)
            {
                case CONCRETE:
                {
                    Block *under;
                    getBlock(b->location.x, b->location.y - 1, &under);
                    if (under == NULL)
                    {
                        moveBlock(b, b->location.x, b->location.y - 1);
                    }
                }
                    break;
                case SAND:
                {
                    Block *lu, *l, *u, *r, *ru;
                    getBlock(b->location.x - 1, b->location.y - 1, &lu);
                    getBlock(b->location.x - 1, b->location.y, &l);
                    getBlock(b->location.x, b->location.y - 1, &u);
                    getBlock(b->location.x + 1, b->location.y, &r);
                    getBlock(b->location.x + 1, b->location.y - 1, &ru);

                    if (u == NULL)
                    {
                        moveBlock(b, b->location.x, b->location.y - 1);
                    }
                    else if (u->type == WATER)
                    {
                        // Sink underwater
                        swapBlockLocations(b, u);

                        if (l == NULL && r == NULL)
                        {
                            int move = u->location.x - 1;
                            if (rand() % 2) move = u->location.x + 1;
                            moveBlock(u, move, u->location.y);
                        }
                        else if (l == NULL) moveBlock(u, u->location.x - 1, u->location.y);
                        else if (r == NULL) moveBlock(u, u->location.x + 1, u->location.y);
                    }
                    else if (lu == NULL)
                    {
                        moveBlock(b, b->location.x - 1, b->location.y - 1);
                    }
                    else if (ru == NULL)
                    {
                        moveBlock(b, b->location.x + 1, b->location.y - 1);
                    }
                    else if (lu->type == WATER)
                    {
                        swapBlockLocations(b, lu);
                    }
                    else if (ru->type == WATER)
                    {
                        swapBlockLocations(b, ru);
                    }
                }
                    break;
                case WATER:
                {
                    Block *l, *u, *r;
                    getBlock(b->location.x - 1, b->location.y, &l);
                    getBlock(b->location.x, b->location.y - 1, &u);
                    getBlock(b->location.x + 1, b->location.y, &r);

                    if (u == NULL)
                    {
                        moveBlock(b, b->location.x, b->location.y - 1);
                    }
                    else if (l == NULL && r == NULL)
                    {
                        moveBlock(b, b->location.x + (((rand() % 2) * 2) - 1), b->location.y);
                    }
                    else if (l == NULL)
                    {
                        moveBlock(b, b->location.x - 1, b->location.y);
                    }
                    else if (r == NULL)
                    {
                        moveBlock(b, b->location.x + 1, b->location.y);
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

void render()
{
    BlockEntry *e = worldHead->next;
    Block *b;
    while (e != worldTail)
    {
        b = e->block;
        if (b->location.x >= 0 && b->location.x < WIDTH && b->location.y >= 0 && b->location.y < HEIGHT)
        {
            draw_pixel_c(b->location.x, HEIGHT - b->location.y - 1, b->color);
        }
        e = e->next;
    }
}

void resetWorld()
{
    BlockEntry *e = worldHead->next;
    Block *b;
    while (e != worldTail)
    {
        b = e->block;
        e = e->next;
        if (b->location.x >= 0 && b->location.x < WIDTH && b->location.y >= 0 && b->location.y < HEIGHT)
        {
            deleteBlock(b);
        }
    }
}

int main()
{
    srand((unsigned) time(NULL));

    worldHead = malloc(sizeof(BlockEntry));
    worldTail = malloc(sizeof(BlockEntry));
    worldHead->next = worldTail;
    worldTail->prev = worldHead;
    worldLoc = malloc(sizeof(BlockEntry *) * SIMWIDTH * SIMHEIGHT);

    Block *b;
    for (int x = -1; x <= WIDTH; ++x)
    {
        createBlock(&b, CONCRETE, x, -1);
        b->gravity = 0;
    }
    for (int y = 0; y < HEIGHT; ++y)
    {
        createBlock(&b, CONCRETE, -1, y);
        b->gravity = 0;
        createBlock(&b, CONCRETE, WIDTH, y);
        b->gravity = 0;
    }

    //createBlock(&b, SAND, 0, 180);
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

        if (!simulationPaused) simulate();
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
                getBlock(mx, my, &b);
                //printf("%d\n", b != NULL);

                if (b != NULL)
                {
                    deleteBlock(b);
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
                            getBlock(bx, by, &b);
                            if (b != NULL) deleteBlock(b);
                        }
                        else
                        {
                            createBlock(&b, nt, bx, by);
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

            createBlock(&bw1, WATER, x1, HEIGHT - 1);
            createBlock(&bw2, WATER, x2, HEIGHT - 1);
            bw1->color = (Color) {0, 0, 255};
            bw2->color = (Color) {0, 0, 255};
        }
        render();

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
                            resetWorld();
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
