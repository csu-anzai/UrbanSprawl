/*
Project: Urban Sprawl
File: urban_tile.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_TILE_H

struct TileChunkPosition
{
    u32 chunkX;
    u32 chunkY;
    u32 chunkZ;
    
    u32 relTileX;
    u32 relTileY;
};

struct TileChunk
{
    // TODO(bSalmon): Tile structure
    u32 *tiles;
};

struct TileMap
{
    u32 chunkCountX;
    u32 chunkCountY;
    u32 chunkCountZ;
    
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
    u32 absTileX;
    u32 absTileY;
    u32 absTileZ;
    
    // NOTE(bSalmon): Still in pixels
    f32 tileRelX;
    f32 tileRelY;
    f32 tileRelZ;
};

enum TileType
{
    TILE_EMPTY,
    TILE_WALKABLE,
    TILE_WALL,
    TILE_STAIRS_UP,
    TILE_STAIRS_DOWN
};

inline TileChunk *GetTileChunk(TileMap *tileMap, u32 chunkX, u32 chunkY, u32 chunkZ)
{
    TileChunk *result = 0;
    
    if ((chunkX < tileMap->chunkCountX) &&
        (chunkY < tileMap->chunkCountY) &&
        (chunkZ < tileMap->chunkCountZ))
    {
        result = &tileMap->chunks[chunkX + 
                (tileMap->chunkCountX * chunkY) + 
                (tileMap->chunkCountX * tileMap->chunkCountY * chunkZ)];
    }
    
    return result;
}

inline TileChunkPosition GetChunkPosition(TileMap *tileMap, u32 absTileX, u32 absTileY, u32 absTileZ)
{
    TileChunkPosition result = {};
    
    result.chunkX = absTileX >> tileMap->chunkShift;
    result.chunkY = absTileY >> tileMap->chunkShift;
    result.chunkZ = absTileZ;
    
    result.relTileX = absTileX & tileMap->chunkMask;
    result.relTileY = absTileY & tileMap->chunkMask;
    
    return result;
}

inline b32 OnSameTile(TileMapPosition *a, TileMapPosition *b)
{
    b32 result = false;
    
    result = ((a->absTileX == b->absTileX) &&
              (a->absTileY == b->absTileY) &&
              (a->absTileZ == b->absTileZ));
    
    return result;
}

#define URBAN_TILE_H
#endif