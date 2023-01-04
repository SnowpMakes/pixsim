#include <stdio.h>
#include <SDL.h>
#include <time.h>

#include "structs.h"

#define WIDTH 1280
#define HEIGHT 00
#define ZOOM 1


BlockEntry *worldHead;
BlockEntry *worldTail;
BlockEntry **worldLoc;
void *pixels;

void getBlock(int x, int y, Block **b)
{
    BlockEntry *be = *(worldLoc + (y * WIDTH) + x);
    if (be == NULL) *b = NULL;
    else *b = be->block;
}

void createBlock(Block **b, int x, int y)
{
    Block *nb = malloc(sizeof(Block));
    nb->location = (PairInt) {x, y};

    BlockEntry *tailp = worldTail->prev;
    BlockEntry *newbe = malloc(sizeof(BlockEntry));
    tailp->next = newbe;
    worldTail->prev = newbe;
    newbe->prev = tailp;
    newbe->next = worldTail;

    newbe->block = nb;

    *(worldLoc + (y * WIDTH) + x) = newbe;
    nb->entry = newbe;

    *b = nb;
}

void moveBlock(Block *b, int x, int y)
{
    if (*(worldLoc + (b->location.y * WIDTH) + b->location.x) != b->entry)
    {
        printf("World location mapping <-> block entry mismatch during moveBlock!\n");
        exit(1);
    }
    *(worldLoc + (b->location.y * WIDTH) + b->location.x) = NULL;
    b->location = (PairInt) {x, y};
    *(worldLoc + (b->location.y * WIDTH) + b->location.x) = b->entry;
}

void deleteBlock(Block *b)
{
    BlockEntry *e = b->entry;
    if (e == NULL)
    {
        printf("Attempted to delete Block with NULL as entry!! What went wrong?!");
        exit(1);
    }

    *(worldLoc + (b->location.y * WIDTH) + b->location.x) = NULL;

    free(b);

    BlockEntry *p = e->prev;
    BlockEntry *n = e->next;
    p->next = n;
    n->prev = p;
    free(e);
}

void simulate()
{
    printf("Simulation running..\n");
    BlockEntry **i = worldLoc + (WIDTH * HEIGHT) - 1;
    while (i >= worldLoc)
    {
        if (*i != NULL)
        {
            Block *b = (*i)->block;

            if (b->location.y < HEIGHT - 1)
            {
                if (b->color.r)
                {
                    Block *under;
                    getBlock(b->location.x, b->location.y + 1, &under);
                    if (under == NULL)
                    {
                        moveBlock(b, b->location.x, b->location.y + 1);
                    }
                }
                else if (b->color.g)
                {
                    Block *lu, *u, *ru;
                    getBlock(b->location.x - 1, b->location.y + 1, &lu);
                    getBlock(b->location.x, b->location.y + 1, &u);
                    getBlock(b->location.x + 1, b->location.y + 1, &ru);

                    if (u == NULL)
                    {
                        moveBlock(b, b->location.x, b->location.y + 1);
                    }
                    else if (lu == NULL)
                    {
                        moveBlock(b, b->location.x - 1, b->location.y + 1);
                    }
                    else if (ru == NULL)
                    {
                        moveBlock(b, b->location.x + 1, b->location.y + 1);
                    }
                }
                else if (b->color.b)
                {
                    Block *l, *u, *r;
                    getBlock(b->location.x - 1, b->location.y, &l);
                    getBlock(b->location.x, b->location.y + 1, &u);
                    getBlock(b->location.x + 1, b->location.y, &r);

                    if (u == NULL)
                    {
                        moveBlock(b, b->location.x, b->location.y + 1);
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
            }
        }
        i--;
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
        draw_pixel_c(b->location.x, b->location.y, b->color);
        e = e->next;
    }
}

int main()
{
    srand((unsigned) time(NULL));

    worldHead = malloc(sizeof(BlockEntry));
    worldTail = malloc(sizeof(BlockEntry));
    worldHead->next = worldTail;
    worldTail->prev = worldHead;
    worldLoc = malloc(sizeof(BlockEntry *) * WIDTH * HEIGHT);

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_CreateWindowAndRenderer(WIDTH * ZOOM, HEIGHT * ZOOM, 0, &window, &renderer);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                             WIDTH * ZOOM, HEIGHT * ZOOM);

    int pitch;
    SDL_Event event;
    int mouseLDown = 0;
    int mouseRDown = 0;
    while (1)
    {
        SDL_LockTexture(texture, NULL, &pixels, &pitch);

        simulate();
        if (mouseLDown || mouseRDown)
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            mx /= ZOOM;
            my /= ZOOM;
            if (mx >= 0 && my >= 0 && mx < WIDTH && my < HEIGHT)
            {
                Block *b;
                getBlock(mx, my, &b);
                printf("%d\n", b != NULL);
                if (b == NULL)
                {
                    createBlock(&b, mx, my);
                    if (mouseLDown && mouseRDown) b->color = (Color) {0, 0, 255};
                    else if (mouseLDown) b->color = (Color) {255, 0, 0};
                    else b->color = (Color) {0, 255, 0};
                }
            }
        }
        render();

        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

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
                default:
                    break;
            }
            if (quit) break;
        }
        if (quit) break;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
