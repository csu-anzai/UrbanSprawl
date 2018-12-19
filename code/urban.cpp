/*
Project: Urban Sprawl
File: urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "urban.h"

#include "urban_tile.cpp"

#include <stdio.h>

internal_func void Game_OutputSound(Game_AudioBuffer *audioBuffer, s32 toneHz)
{
    s16 toneVolume = 3000;
    s32 wavePeriod = audioBuffer->sampleRate/toneHz;
    
    s16 *sampleOut = audioBuffer->samples;
    for(int sampleIndex = 0; sampleIndex < audioBuffer->sampleCount; ++sampleIndex)
    {
        /*
        f32 sineValue = sinf(tSine);
        s16 sampleValue = (s16)(sineValue * toneVolume);
        */
        
        s16 sampleValue = 0;
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        /*
        tSine += 2.0f * PI * 1.0f / (f32)wavePeriod;
        
        if (tSine > 2.0f * PI)
        {
            tSine -= 2.0f * PI;
        }
        */
    }
}

internal_func void InitMemRegion(MemoryRegion *memRegion, mem_index size, u8 *base)
{
    memRegion->size = size;
    memRegion->base = base;
    memRegion->used = 0;
}

#define PushStruct(region, type) (type *)PushSize_(region, sizeof(type))
#define PushArray(region, count, type) (type *)PushSize_(region, (count) * sizeof(type))
internal_func void *PushSize_(MemoryRegion *memRegion, mem_index size)
{
    ASSERT((memRegion->used + size) <= memRegion->size);
    void *result = memRegion->base + memRegion->used;
    memRegion->used += size;
    
    return result;
}

internal_func InterpretedNetworkData InterpretNetworkData(Game_NetworkPacket *networkPacket)
{
    InterpretedNetworkData result = {};
    
    s32 index = 0;
    FillVariableFromDataBlock(&result.playerPos, &networkPacket->data[index], &index);
    
    return result;
}

internal_func void DrawRect(Game_BackBuffer *backBuffer, f32 minX, f32 minY, f32 maxX, f32 maxY, u32 colour)
{
    u8 *endOfBuffer = (u8 *)backBuffer->memory + (backBuffer->pitch * backBuffer->height);
    
    s32 roundedMinX = RoundF32ToS32(minX);
    s32 roundedMinY = RoundF32ToS32(minY);
    s32 roundedMaxX = RoundF32ToS32(maxX);
    s32 roundedMaxY = RoundF32ToS32(maxY);
    
    if (roundedMinX < 0)
    {
        roundedMinX = 0;
    }
    
    if (roundedMinY < 0)
    {
        roundedMinY = 0;
    }
    
    if (roundedMaxX > backBuffer->width)
    {
        roundedMaxX = backBuffer->width;
    }
    
    if (roundedMaxY > backBuffer->height)
    {
        roundedMaxY = backBuffer->height;
    }
    
    for (s32 x = roundedMinX; x < roundedMaxX; ++x)
    {
        u8 *pixel = (u8 *)backBuffer->memory + (x * backBuffer->bytesPerPixel) + (roundedMinY * backBuffer->pitch);
        
        for (s32 y = roundedMinY; y < roundedMaxY; ++y)
        {
            if ((pixel >= backBuffer->memory) && ((pixel + 4) <= endOfBuffer))
            {
                *(u32 *)pixel = colour;
            }
            
            pixel += backBuffer->pitch;
        }
    }
}

extern "C" GAME_UPDATE_RENDER(Game_UpdateRender)
{
    ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
    
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    if (!memory->memInitialised)
    {
        gameState->playerPos.absTileX = 3;
        gameState->playerPos.absTileY = 3;
        gameState->playerPos.tileRelX = 5.0f;
        gameState->playerPos.tileRelY = 5.0f;
        
        InitMemRegion(&gameState->worldRegion, memory->permanentStorageSize - sizeof(Game_State), (u8 *)memory->permanentStorage + sizeof(Game_State));
        
        gameState->world = PushStruct(&gameState->worldRegion, World);
        World *world = gameState->world;
        world->tileMap = PushStruct(&gameState->worldRegion, TileMap);
        
        TileMap *tileMap = world->tileMap;
        
        tileMap->chunkShift = 4;
        tileMap->chunkMask = (1 << tileMap->chunkShift) - 1;
        tileMap->chunkDim = (1 << tileMap->chunkShift);
        
        tileMap->chunkCountX = 128;
        tileMap->chunkCountY = 128;
        tileMap->chunks = PushArray(&gameState->worldRegion, tileMap->chunkCountX * tileMap->chunkCountY, TileChunk);
        
        for (u32 y = 0; y < tileMap->chunkCountY; ++y)
        {
            for (u32 x = 0; x < tileMap->chunkCountX; ++x)
            {
                tileMap->chunks[x + (tileMap->chunkCountX * y)].tiles = PushArray(&gameState->worldRegion, tileMap->chunkDim * tileMap->chunkDim, u32);
            }
        }
        
        tileMap->tileSidePixels = 70;
        tileMap->tileSideMeters = 2.0f;
        tileMap->metersToPixels = (f32)tileMap->tileSidePixels / tileMap->tileSideMeters;
        
        u32 tilesPerWidth = 17;
        u32 tilesPerHeight = 9;
        for (u32 screenY = 0; screenY < 32; ++screenY)
        {
            for (u32 screenX = 0; screenX < 32; ++screenX)
            {
                for (u32 tileY = 0; tileY < tilesPerHeight; ++tileY)
                {
                    for (u32 tileX = 0; tileX < tilesPerWidth; ++tileX)
                    {
                        u32 absTileX = (screenX * tilesPerWidth) + tileX;
                        u32 absTileY = (screenY * tilesPerHeight) + tileY;
                        
                        SetTileValue(&gameState->worldRegion, world->tileMap, absTileX, absTileY, ((tileX == tileY) && (tileY % 2) ? 1 : 0));
                    }
                }
            }
        }
        
        memory->memInitialised = true;
    }
    
    World *world = gameState->world;
    TileMap *tileMap = world->tileMap;
    
    f32 playerHeight = tileMap->tileSideMeters * 0.9f;
    f32 playerWidth = 0.5f * playerHeight;
    
    if (multiplayer)
    {
        InterpretedNetworkData networkData = {};
        networkData = InterpretNetworkData(networkPacket);
        gameState->playerPos = networkData.playerPos;
    }
    else
    {
        for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
        {
            f32 deltaPlayerX = 0.0f;
            f32 deltaPlayerY = 0.0f;
            
            Game_Controller *controller = GetGameController(input, controllerIndex);
            if (controller->isAnalog)
            {
                deltaPlayerX += controller->lAverageX;
                deltaPlayerY -= controller->lAverageY;
            }
            else
            {
                if (controller->dPadLeft.endedDown)
                {
                    deltaPlayerX = -1.0f;
                }
                if (controller->dPadRight.endedDown)
                {
                    deltaPlayerX = 1.0f;
                }
                if (controller->dPadUp.endedDown)
                {
                    deltaPlayerY = 1.0f;
                }
                if (controller->dPadDown.endedDown)
                {
                    deltaPlayerY = -1.0f;
                }
                
                if (controller->faceDown.endedDown)
                {
                    deltaPlayerX *= 10.0f;
                    deltaPlayerY *= 10.0f;
                }
                else
                {
                    deltaPlayerX *= 2.0f;
                    deltaPlayerY *= 2.0f;
                }
            }
            
            // TODO(bSalmon): Use vectors to fix movement speed
            TileMapPosition newPlayerPos = gameState->playerPos;
            newPlayerPos.tileRelX += input->deltaTime * deltaPlayerX;
            newPlayerPos.tileRelY += input->deltaTime * deltaPlayerY;
            newPlayerPos = RecanonicalisePos(tileMap, newPlayerPos);
            
            TileMapPosition playerLeft = newPlayerPos;
            playerLeft.tileRelX -= 0.5f * playerWidth;
            playerLeft = RecanonicalisePos(tileMap, playerLeft);
            
            TileMapPosition playerRight = newPlayerPos;
            playerRight.tileRelX += 0.5f * playerWidth;
            playerRight = RecanonicalisePos(tileMap, playerRight);
            
            // Check Left, Right, and Middle of the Player for collision 
            if (IsTileMapPointValid(tileMap, newPlayerPos) &&
                IsTileMapPointValid(tileMap, playerLeft) &&
                IsTileMapPointValid(tileMap, playerRight))
            {
                gameState->playerPos = newPlayerPos;
            }
        }
    }
    
    DrawRect(backBuffer, 0.0f, 0.0f, (f32)backBuffer->width, (f32)backBuffer->height, 0xFF4C0296);
    
    f32 screenCenterX = 0.5f * (f32)backBuffer->width;
    f32 screenCenterY = 0.5f * (f32)backBuffer->height;
    
    for (s32 relY = -10; relY < 10; ++relY)
    {
        for (s32 relX = -20; relX < 20; ++relX)
        {
            u32 x = gameState->playerPos.absTileX + relX;
            u32 y = gameState->playerPos.absTileY + relY;
            u32 tileID = GetTileValue(tileMap, x, y);
            u32 tileColour = 0xFF888888;
            if (tileID == 1)
            {
                tileColour = 0xFFFFFFFF;
            }
            
            if ((x == gameState->playerPos.absTileX) &&
                (y == gameState->playerPos.absTileY))
            {
                tileColour = 0xFF000000;
            }
            
            
            f32 centerX = screenCenterX - (tileMap->metersToPixels * gameState->playerPos.tileRelX) + ((f32)relX * tileMap->tileSidePixels);
            f32 centerY = screenCenterY + (tileMap->metersToPixels * gameState->playerPos.tileRelY) - ((f32)relY * tileMap->tileSidePixels);
            f32 minX = centerX - (0.5f * tileMap->tileSidePixels);
            f32 minY = centerY - (0.5f * tileMap->tileSidePixels);
            f32 maxX = centerX + (0.5f * tileMap->tileSidePixels);
            f32 maxY = centerY + (0.5f * tileMap->tileSidePixels);
            
            DrawRect(backBuffer, minX, minY, minX + tileMap->tileSidePixels, minY + tileMap->tileSidePixels, tileColour);
        }
    }
    
    f32 playerLeft = screenCenterX  - (0.5f * tileMap->metersToPixels * playerWidth);
    f32 playerTop = screenCenterY - (tileMap->metersToPixels * playerHeight);
    
    DrawRect(backBuffer, playerLeft, playerTop, playerLeft + (tileMap->metersToPixels * playerWidth), playerTop + (tileMap->metersToPixels * playerHeight), 0xFF0000FF);
}

extern "C" GAME_GET_AUDIO_SAMPLES(Game_GetAudioSamples)
{
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    Game_OutputSound(audioBuffer, 256);
}