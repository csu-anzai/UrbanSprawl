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
    
    u32 relTileX;
    u32 relTileY;
};

struct TileChunk
{
    u32 *tiles;
};

struct TileMap
{
    u32 chunkCountX;
    u32 chunkCountY;
    
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
    u32 absTileX;
    u32 absTileY;
    
    // NOTE(bSalmon): Still in pixels
    f32 tileRelX;
    f32 tileRelY;
};

inline TileChunk *GetTileChunk(TileMap *tileMap, u32 chunkX, u32 chunkY)
{
    TileChunk *result = 0;
    
    if ((chunkX < tileMap->chunkCountX) &&
        (chunkY < tileMap->chunkCountY))
    {
        result = &tileMap->chunks[chunkX + (tileMap->chunkCountX * chunkY)];
    }
    
    return result;
}

inline TileChunkPosition GetChunkPosition(TileMap *tileMap, u32 absTileX, u32 absTileY)
{
    TileChunkPosition result = {};
    
    result.chunkX = absTileX >> tileMap->chunkShift;
    result.chunkY = absTileY >> tileMap->chunkShift;
    result.relTileX = absTileX & tileMap->chunkMask;
    result.relTileY = absTileY & tileMap->chunkMask;
    
    return result;
}

#define URBAN_TILE_H
#endif