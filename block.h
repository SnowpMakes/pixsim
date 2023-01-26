//
// Created by Snowp on 26/01/2023.
//

#ifndef PIXSIM_BLOCK_H

#include "color.h"
#include "vector.h"

struct BlockEntry_;

typedef enum BlockType_
{
    CONCRETE,
    SAND,
    WATER
} BlockType;

typedef struct Block_
{
    struct BlockEntry_ *entry;
    BlockType type;
    PairInt location;
    int gravity;
    Color color;
} Block;

#define PIXSIM_BLOCK_H

#endif //PIXSIM_BLOCK_H
