/*
Project: Urban Sprawl
File: urban_tile.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

inline u32 GetTileValueUnchecked(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y)
{
    ASSERT(chunk);
    ASSERT(x < tileMap->chunkDim);
    ASSERT(y < tileMap->chunkDim);
    
    u32 result = chunk->tiles[x + (tileMap->chunkDim * y)];
    return result;
}

inline void SetTileValueUnchecked(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y, u32 tileValue)
{
    ASSERT(chunk);
    ASSERT(x < tileMap->chunkDim);
    ASSERT(y < tileMap->chunkDim);
    
    chunk->tiles[x + (tileMap->chunkDim * y)] = tileValue;
}

inline void RecanonicaliseCoord(TileMap *tileMap, u32 *tile, f32 *tileRel)
{
    s32 offset = RoundF32ToS32(*tileRel / tileMap->tileSideMeters);
    *tile += offset;
    *tileRel -= offset * tileMap->tileSideMeters;
    
    ASSERT(*tileRel >= -0.5f * tileMap->tileSideMeters);
    ASSERT(*tileRel <= 0.5f * tileMap->tileSideMeters);
}

internal_func u32 GetTileValue(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y)
{
    u32 result = 0;
    
    if (chunk && chunk->tiles)
    {
        result = GetTileValueUnchecked(tileMap, chunk, x, y);
    }
    
    return result;
}

internal_func void SetTileValue(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y, u32 tileValue)
{
    if (chunk && chunk->tiles)
    {
        SetTileValueUnchecked(tileMap, chunk, x, y, tileValue);
    }
}

internal_func u32 GetTileValue(TileMap *tileMap, u32 x, u32 y)
{
    u32 result = 0;
    
    TileChunkPosition chunkPos = GetChunkPosition(tileMap, x, y);
    TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunkX, chunkPos.chunkY);
    result = GetTileValue(tileMap, chunk, chunkPos.relTileX, chunkPos.relTileY);
    
    return result;
}

internal_func b32 IsTileMapPointValid(TileMap *tileMap, TileMapPosition tileMapPos)

{
    b32 result = false;
    
    u32 tileValue = GetTileValue(tileMap, tileMapPos.absTileX, tileMapPos.absTileY);
    result = (tileValue == 0);
    
    return result;
}

internal_func TileMapPosition RecanonicalisePos(TileMap *tileMap, TileMapPosition tileMapPos)
{
    TileMapPosition result = tileMapPos;
    
    RecanonicaliseCoord(tileMap, &result.absTileX, &result.tileRelX);
    RecanonicaliseCoord(tileMap, &result.absTileY, &result.tileRelY);
    
    return result;
}

internal_func void SetTileValue(MemoryRegion *memRegion, TileMap *tileMap, u32 x, u32 y, u32 tileValue)
{
    TileChunkPosition chunkPos = GetChunkPosition(tileMap, x, y);
    TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunkX, chunkPos.chunkY);
    
    ASSERT(chunk);
    SetTileValueUnchecked(tileMap, chunk, chunkPos.relTileX, chunkPos.relTileY, tileValue);
}
