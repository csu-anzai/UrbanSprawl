/*
Project: Urban Sprawl PoC
File: urban.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_H

#include "urban_platform.h"
#include "urban_memory.h"
#include "urban_tile.h"
#include "urban_intrinsics.h"

#include <math.h>
#include <stdio.h>
#include <malloc.h>

// Utilities
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define SWAP(a, b) {decltype(a) temp = a; a = b; b = temp;}

#if URBAN_SLOW
#define ASSERT(expr) if(!(expr)) {*(s32 *)0 = 0;}
#else
#define ASSERT(expr)
#endif

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value)*1024LL)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)
#define TERABYTES(value) (GIGABYTES(value)*1024LL)

#define PI 3.14159265359f
#define MAX_CLIENTS 8

enum FacingDir
{
    FACING_FRONT,
    FACING_LEFT,
    FACING_BACK,
    FACING_RIGHT
};

struct MemoryRegion
{
    mem_index size;
    u8 *base;
    mem_index used;
};

inline void InitMemRegion(MemoryRegion *memRegion, mem_index size, u8 *base)
{
    memRegion->size = size;
    memRegion->base = base;
    memRegion->used = 0;
}

#define PushStruct(region, type) (type *)PushSize_(region, sizeof(type))
#define PushArray(region, count, type) (type *)PushSize_(region, (count) * sizeof(type))
inline void *PushSize_(MemoryRegion *memRegion, mem_index size)
{
    ASSERT((memRegion->used + size) <= memRegion->size);
    void *result = memRegion->base + memRegion->used;
    memRegion->used += size;
    
    return result;
}

#pragma pack(push, 1)
struct BitmapHeader
{
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmapOffset;
    
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bitsPerPixel;
    u32 compression;
    u32 sizeOfBitmap;
    s32 horzResolution;
    s32 vertResolution;
    u32 coloursUsed;
    u32 coloursImportant;
    
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
};
#pragma pack(pop)

struct World
{
    TileMap *tileMap;
};

struct LoadedBitmap
{
    s32 width;
    s32 height;
    u32 *pixelData;
};

struct CharacterBitmaps
{
    s32 alignX;
    s32 alignY;
    LoadedBitmap head;
    LoadedBitmap torso;
    LoadedBitmap legs;
    LoadedBitmap feet;
};

struct Game_State
{
    MemoryRegion worldRegion;
    World *world;
    
    TileMapPosition cameraPos;
    TileMapPosition playerPos;
    
    LoadedBitmap background;
    u32 playerDir;
    CharacterBitmaps playerBitmaps[4];
};

struct InterpretedNetworkData
{
    TileMapPosition playerPos;
};

inline Game_Controller *GetGameController(Game_Input *input, s32 controllerIndex)
{
    ASSERT(controllerIndex < ARRAY_COUNT(input->controllers));
    return &input->controllers[controllerIndex];
}

inline u32 SafeTruncateU64(u64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return result;
}

inline void ConcatenateStrings(mem_index sourceACount, char *sourceA,
                               mem_index sourceBCount, char *sourceB,
                               mem_index destCount, char *dest)
{
	for (s32 i = 0; i < sourceACount; ++i)
	{
		*dest++ = *sourceA++;
	}
	
	for (s32 i = 0; i < sourceBCount; ++i)
	{
		*dest++ = *sourceB++;
	}
	
	*dest++ = 0;
}

inline s32 StringLength(char *string)
{
	s32 result = 0;
	while (*string++)
	{
		++result;
	}
	
	return result;
}

inline s32 GetHostPieceRange(u8 hostSegment)
{
    s32 result = 0;
    
    if (hostSegment > 99)
    {
        result = 3;
    }
    else if (hostSegment < 100 && hostSegment > 9)
    {
        result = 2;
    }
    else
    {
        result = 1;
    }
    
    return result;
}

inline char *ConstructIPString(u32 host, u16 port)
{
    // NOTE(bSalmon): Starts at 5 to accomodate the 3 periods, the colon, and the terminator
    s32 charAmount = 5;
    
    u8 ip1 = (host >> 24) & 0xFF;
    charAmount+= GetHostPieceRange(ip1);
    
    u8 ip2 = (host >> 16) & 0xFF;
    charAmount += GetHostPieceRange(ip2);
    
    u8 ip3 = (host >> 8) & 0xFF;
    charAmount += GetHostPieceRange(ip3);
    
    u8 ip4 = host & 0xFF;
    charAmount += GetHostPieceRange(ip4);
    
    if (port > 9999)
    {
        charAmount += 5;
    }
    else if (port < 10000 && port > 999)
    {
        charAmount += 4;
    }
    else if (port < 1000 && port > 99)
    {
        charAmount += 3;
    }
    else if (port < 100 && port > 9)
    {
        charAmount += 2;
    }
    else
    {
        charAmount += 1;
    }
    
    mem_index stringSize = (sizeof(char) * charAmount);
    char *resultString = (char *)malloc(stringSize);
    
    sprintf_s(resultString, stringSize, "%d.%d.%d.%d:%d\0", ip4, ip3, ip2, ip1, port);
    
    return resultString;
}

#define URBAN_H
#endif // URBAN_H