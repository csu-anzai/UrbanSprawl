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

internal_func u32 GetTileValue(TileMap *tileMap, TileChunk *chunk, v2<u32> tile)
{
    u32 result = 0;
    
    if (chunk && chunk->tiles)
    {
        result = GetTileValueUnchecked(tileMap, chunk, tile.x, tile.y);
    }
    
    return result;
}

internal_func u32 GetTileValue(TileMap *tileMap, v3<u32> tilePos)
{
    u32 result = 0;
    
    TileChunkPosition chunkPos = GetChunkPosition(tileMap, tilePos);
    TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
    result = GetTileValue(tileMap, chunk, chunkPos.relTile);
    
    return result;
}

internal_func u32 GetTileValue(TileMap *tileMap, TileMapPosition pos)
{
    u32 result = 0;
    
    result = GetTileValue(tileMap, pos.absTile);
    
    return result;
}

inline void SetTileValueUnchecked(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y, u32 tileValue)
{
    ASSERT(chunk);
    ASSERT(x < tileMap->chunkDim);
    ASSERT(y < tileMap->chunkDim);
    
    chunk->tiles[x + (tileMap->chunkDim * y)] = tileValue;
}

internal_func void SetTileValue(TileMap *tileMap, TileChunk *chunk, v2<u32> tile, u32 tileValue)
{
    if (chunk && chunk->tiles)
    {
        SetTileValueUnchecked(tileMap, chunk, tile.x, tile.y, tileValue);
    }
}

internal_func void SetTileValue(MemoryRegion *memRegion, TileMap *tileMap, v3<u32> tilePos, u32 tileValue)
{
    TileChunkPosition chunkPos = GetChunkPosition(tileMap, tilePos);
    TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
    
    ASSERT(chunk);
    if (!chunk->tiles)
    {
        u32 tileCount = tileMap->chunkDim * tileMap->chunkDim;
        chunk->tiles = PushArray(memRegion, tileCount, u32);
        for (u32 tileIndex = 0; tileIndex < tileCount; ++tileIndex)
        {
            chunk->tiles[tileIndex] = TILE_WALKABLE;
        }
    }
    
    SetTileValue(tileMap, chunk, chunkPos.relTile, tileValue);
}

inline void RecanonicaliseCoord(TileMap *tileMap, u32 *tile, f32 *tileRel)
{
    s32 offset = RoundF32ToS32(*tileRel / tileMap->tileSideMeters);
    *tile += offset;
    *tileRel -= offset * tileMap->tileSideMeters;
    
    ASSERT(*tileRel >= -0.5f * tileMap->tileSideMeters);
    ASSERT(*tileRel <= 0.5f * tileMap->tileSideMeters);
}

internal_func b32 IsTileMapPointValid(TileMap *tileMap, TileMapPosition tileMapPos)
{
    b32 result = false;
    
    u32 tileValue = GetTileValue(tileMap, tileMapPos.absTile);
    result = ((tileValue == TILE_WALKABLE) ||
              (tileValue == TILE_STAIRS_UP) ||
              (tileValue == TILE_STAIRS_DOWN));
    
    return result;
}

internal_func TileMapPosition RecanonicalisePos(TileMap *tileMap, TileMapPosition tileMapPos)
{
    TileMapPosition result = tileMapPos;
    
    RecanonicaliseCoord(tileMap, &result.absTile.x, &result.tileRel.x);
    RecanonicaliseCoord(tileMap, &result.absTile.y, &result.tileRel.y);
    
    return result;
}
