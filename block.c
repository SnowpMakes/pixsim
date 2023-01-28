//
// Created by Snowp on 26/01/2023.
//

#include <stdlib.h>

#include "block.h"


void createBlock(Block **b, BlockType t)
{
    Block *nb;

    nb = malloc(sizeof(Block));
    nb->type = t;
    nb->gravity = 1;

    *b = nb;
}

void destroyBlock(Block *b)
{
    free(b);
}
