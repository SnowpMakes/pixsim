//
// Created by Snowp on 03/01/2023.
//

#ifndef PIXSIM_STRUCTS_H
#define PIXSIM_STRUCTS_H

typedef struct PairDouble
{
    double x, y;
} Vector2;

typedef struct PairInt
{
    int x, y;
} PairInt;

typedef struct Pixel
{
    uint8_t r, g, b, a;
} Pixel;

typedef struct Color
{
    uint8_t r, g, b;
} Color;

typedef enum BlockType
{
    CONCRETE,
    SAND,
    WATER
} BlockType;

struct BlockEntry;

typedef struct Block
{
    struct BlockEntry *entry;
    BlockType type;
    PairInt location;
    int gravity;
    Color color;
} Block;

typedef struct BlockEntry
{
    struct BlockEntry *prev;
    struct BlockEntry *next;
    Block *block;
} BlockEntry;

#endif //PIXSIM_STRUCTS_H
