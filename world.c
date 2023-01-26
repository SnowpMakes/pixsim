//
// Created by Snowp on 26/01/2023.
//

#include <stdio.h>
#include <stdlib.h>
#include "world.h"


BlockEntry *getBlockEntry(World *w, int x, int y)
{
    if (x < -1 || x > w->width || y < -1 || y >= w->height)
    {
        printf("Invalid call to getBlockEntry!\n");
        exit(1);
    }
    return *(w->blockLocations + ((y + 1) * w->simWidth) + x + 1);
}

void setBlockEntry(World *w, int x, int y, BlockEntry *entry)
{
    if (x < -1 || x > w->width || y < -1 || y >= w->height)
    {
        printf("Invalid call to setBlockEntry!\n");
        exit(1);
    }
    *(w->blockLocations + ((y + 1) * w->simWidth) + x + 1) = entry;
}

void getBlock(World *w, int x, int y, Block **b)
{
    if (x < -1 || x > w->width || y < -1 || y >= w->height)
    {
        *b = NULL;
        return;
    }
    BlockEntry *be = getBlockEntry(w, x, y);
    if (be == NULL) *b = NULL;
    else *b = be->block;
}

void createBlock(World *w, Block **b, BlockType t, int x, int y)
{
    if (x < -1 || x > w->width || y < -1 || y >= w->height)
    {
        *b = NULL;
        return;
    }

    Block *nb;
    getBlock(w, x, y, &nb);
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

    BlockEntry *tailp = w->blockTrailer->prev;
    BlockEntry *newbe = malloc(sizeof(BlockEntry));
    tailp->next = newbe;
    w->blockTrailer->prev = newbe;
    newbe->prev = tailp;
    newbe->next = w->blockTrailer;

    newbe->block = nb;

    setBlockEntry(w, x, y, newbe);
    nb->entry = newbe;

    *b = nb;
}

void moveBlock(World *w, Block *b, int x, int y)
{
    if (getBlockEntry(w, b->location.x, b->location.y) != b->entry)
    {
        printf("World location mapping <-> block entry mismatch during moveBlock!\n");
        exit(1);
    }
    setBlockEntry(w, b->location.x, b->location.y, NULL);
    b->location = (PairInt) {x, y};
    setBlockEntry(w, b->location.x, b->location.y, b->entry);
}

void swapBlockLocations(World *w, Block *b1, Block *b2)
{
    if (getBlockEntry(w, b1->location.x, b1->location.y) != b1->entry)
    {
        printf("World location mapping <-> block entry mismatch during swapBlockLocations b1!\n");
        exit(1);
    }
    if (getBlockEntry(w, b2->location.x, b2->location.y) != b2->entry)
    {
        printf("World location mapping <-> block entry mismatch during swapBlockLocations b2!\n");
        exit(1);
    }

    PairInt l1 = b1->location;
    PairInt l2 = b2->location;

    b1->location = (PairInt) {l2.x, l2.y};
    b2->location = (PairInt) {l1.x, l1.y};
    setBlockEntry(w, b1->location.x, b1->location.y, b1->entry);
    setBlockEntry(w, b2->location.x, b2->location.y, b2->entry);
}

void deleteBlock(World *w, Block *b)
{
    BlockEntry *e = b->entry;
    if (e == NULL)
    {
        printf("Attempted to delete Block with NULL as entry!! What went wrong?!");
        exit(1);
    }

    setBlockEntry(w, b->location.x, b->location.y, NULL);

    free(b);

    BlockEntry *p = e->prev;
    BlockEntry *n = e->next;
    p->next = n;
    n->prev = p;
    free(e);
}

void createWorld(World **w, int width, int height)
{
    World *nw;
    nw = malloc(sizeof(World));

    nw->width = width;
    nw->height = height;
    nw->simWidth = width + 2;
    nw->simHeight = height + 1;

    nw->blockHeader = malloc(sizeof(BlockEntry));
    nw->blockTrailer = malloc(sizeof(BlockEntry));
    nw->blockHeader->next = nw->blockTrailer;
    nw->blockTrailer->prev = nw->blockHeader;
    nw->blockLocations = malloc(sizeof(BlockEntry *) * nw->simWidth * nw->simHeight);

    Block *b;
    for (int x = -1; x <= width; ++x)
    {
        createBlock(nw, &b, CONCRETE, x, -1);
        b->gravity = 0;
    }
    for (int y = 0; y < height; ++y)
    {
        createBlock(nw, &b, CONCRETE, -1, y);
        b->gravity = 0;
        createBlock(nw, &b, CONCRETE, width, y);
        b->gravity = 0;
    }

    *w = nw;
}

void resetWorld(World *w)
{
    BlockEntry *e = w->blockHeader->next;
    Block *b;
    while (e != w->blockTrailer)
    {
        b = e->block;
        e = e->next;
        if (b->location.x >= 0 && b->location.x < w->width && b->location.y >= 0 && b->location.y < w->height)
        {
            deleteBlock(w, b);
        }
    }
}
