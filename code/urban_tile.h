/*
Project: Urban Sprawl
File: urban_tile.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_TILE_H

#include "urban_math.h"

struct TileChunkPosition
{
    v3<u32> chunk;
    v2<u32> relTile;
};

struct TileChunk
{
    // TODO(bSalmon): Tile structure
    u32 *tiles;
};

struct TileMap
{
    v3<u32> chunkCount;
    
    u32 chunkShift;
    u32 chunkMask;
    u32 chunkDim;
    
    s32 tileSidePixels;
    f32 tileSideMeters;
    f32 metersToPixels;
    
    TileChunk *chunks;
};

struct TileMapPosition
{
    // NOTE(bSalmon): Fixed point tile locations, 
    // high bytes are the chunk index, low bytes are the tile index within the chunk
    v3<u32> absTile;
    
    // NOTE(bSalmon): Still in pixels
    v2<f32> tileRel;
};

enum TileType
{
    TILE_EMPTY,
    TILE_WALKABLE,
    TILE_WALL,
    TILE_STAIRS_UP,
    TILE_STAIRS_DOWN
};

inline TileChunk *GetTileChunk(TileMap *tileMap, v3<u32> chunk)
{
    TileChunk *result = 0;
    
    if (chunk < tileMap->chunkCount)
    {
        result = &tileMap->chunks[chunk.x + 
                (tileMap->chunkCount.x * chunk.y) + 
                (tileMap->chunkCount.x * tileMap->chunkCount.y * chunk.z)];
    }
    
    return result;
}

inline TileChunkPosition GetChunkPosition(TileMap *tileMap, v3<u32> absTile)
{
    TileChunkPosition result = {};
    
    result.chunk.x = absTile.x >> tileMap->chunkShift;
    result.chunk.y = absTile.y >> tileMap->chunkShift;
    result.chunk.z = absTile.z;
    
    result.relTile.x = absTile.x & tileMap->chunkMask;
    result.relTile.y = absTile.y & tileMap->chunkMask;
    
    return result;
}

inline b32 OnSameTile(TileMapPosition *a, TileMapPosition *b)
{
    b32 result = false;
    
    result = (a->absTile == b->absTile);
    
    return result;
}

#define URBAN_TILE_H
#endif