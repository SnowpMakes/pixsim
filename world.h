//
// Created by Snowp on 26/01/2023.
//

#ifndef PIXSIM_WORLD_H

#include "block.h"

typedef struct BlockEntry_
{
    struct BlockEntry_ *prev;
    struct BlockEntry_ *next;
    Block *block;
} BlockEntry;

typedef struct World_
{
    int width;
    int height;
    int simWidth;
    int simHeight;
    BlockEntry *blockHeader;
    BlockEntry *blockTrailer;
    BlockEntry **blockLocations;
} World;

BlockEntry *getBlockEntry(World *w, int x, int y);

void setBlockEntry(World *w, int x, int y, BlockEntry *entry);

void getBlock(World *w, int x, int y, Block **b);

void createBlock(World *w, Block **b, BlockType t, int x, int y);

void moveBlock(World *w, Block *b, int x, int y);

void swapBlockLocations(World *w, Block *b1, Block *b2);

void deleteBlock(World *w, Block *b);

void createWorld(World **w, int width, int height);

void resetWorld(World *w);

#define PIXSIM_WORLD_H

#endif //PIXSIM_WORLD_H
