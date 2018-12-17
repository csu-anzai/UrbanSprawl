/*
Project: Urban Sprawl
File: urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "urban.h"
#include "urban_intrinsics.h"

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

internal_func InterpretedNetworkData InterpretNetworkData(Game_NetworkPacket *networkPacket)
{
    InterpretedNetworkData result = {};
    
    s32 index = 0;
    
    FillVariableFromDataBlock(&result.playerX, &networkPacket->data[index], &index);
    FillVariableFromDataBlock(&result.playerY, &networkPacket->data[index], &index);
    
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

inline u32 GetTileValueUnchecked(Tilemap *tilemap, s32 x, s32 y)
{
    u32 result = tilemap->tiles[x + (tilemap->dimX * y)];
    return result;
}

internal_func b32 IsTilemapPointValid(Tilemap *tilemap, f32 testX, f32 testY)
{
    b32 result = false;
    
    s32 playerTileX = TruncateF32ToS32(testX / tilemap->tileSide);
    s32 playerTileY = TruncateF32ToS32(testY / tilemap->tileSide);
    
    if ((playerTileX >= 0) && (playerTileX < tilemap->dimX) &&
        (playerTileY >= 0) && (playerTileY < tilemap->dimY))
    {
        u32 tileValue = GetTileValueUnchecked(tilemap, playerTileX, playerTileY);
        result = (tileValue == 0);
    }
    
    return result;
}

extern "C" GAME_UPDATE_RENDER(Game_UpdateRender)
{
    ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
    
#define TILEMAP_WIDTH 17
#define TILEMAP_HEIGHT 9
    
    u32 tilemap00[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    
    u32 tilemap01[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    
    u32 tilemap10[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    
    u32 tilemap11[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    
    Tilemap tilemaps[2][2];
    tilemaps[0][0].dimX = TILEMAP_WIDTH;
    tilemaps[0][0].dimY = TILEMAP_HEIGHT;
    tilemaps[0][0].tileSide = 70;
    tilemaps[0][0].tiles = (u32 *)tilemap00;
    
    tilemaps[0][1] = tilemaps[0][0];
    tilemaps[0][1].tiles = (u32 *)tilemap01;
    
    tilemaps[1][0] = tilemaps[0][0];
    tilemaps[1][0].tiles = (u32 *)tilemap10;
    
    tilemaps[1][1] = tilemaps[0][0];
    tilemaps[1][1].tiles = (u32 *)tilemap11;
    
    Tilemap *currTilemap = &tilemaps[0][0];
    
    World world = {};
    world.tilemapCountX = 2;
    world.tilemapCountY = 2;
    world.tilemaps = (Tilemap *)tilemaps;
    
    f32 playerWidth = currTilemap->tileSide * 0.5f;
    f32 playerHeight = (f32)currTilemap->tileSide;
    
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    if (!memory->memInitialised)
    {
        gameState->playerX = 100;
        gameState->playerY = 100;
        memory->memInitialised = true;
    }
    
    if (multiplayer)
    {
        InterpretedNetworkData networkData = {};
        networkData = InterpretNetworkData(networkPacket);
        gameState->playerX += networkData.playerX;
        gameState->playerY += networkData.playerY;
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
                    deltaPlayerX -= 1.0f;
                }
                if (controller->dPadRight.endedDown)
                {
                    deltaPlayerX += 1.0f;
                }
                if (controller->dPadUp.endedDown)
                {
                    deltaPlayerY -= 1.0f;
                }
                if (controller->dPadDown.endedDown)
                {
                    deltaPlayerY += 1.0f;
                }
            }
            
            deltaPlayerX *= 64.0f;
            deltaPlayerY *= 64.0f;
            
            f32 newPlayerX = gameState->playerX + (input->deltaTime * deltaPlayerX);
            f32 newPlayerY = gameState->playerY + (input->deltaTime * deltaPlayerY);
            
            // Check Left, Right, and Middle of the PLayer for collision 
            if (IsTilemapPointValid(currTilemap, (newPlayerX - (0.5f * playerWidth)), newPlayerY) &&
                IsTilemapPointValid(currTilemap, (newPlayerX + (0.5f * playerWidth)), newPlayerY) &&
                IsTilemapPointValid(currTilemap, newPlayerX, newPlayerY))
            {
                gameState->playerX = newPlayerX;
                gameState->playerY = newPlayerY;
            }
        }
    }
    
    DrawRect(backBuffer, 0.0f, 0.0f, (f32)backBuffer->width, (f32)backBuffer->height, 0xFF4C0296);
    
    for (s32 y = 0; y < TILEMAP_HEIGHT; ++y)
    {
        for (s32 x = 0; x < TILEMAP_WIDTH; ++x)
        {
            u32 tileID = GetTileValueUnchecked(currTilemap, x, y);
            u32 tileColour = 0xFF888888;
            if (tileID == 1)
            {
                tileColour = 0xFFFFFFFF;
            }
            
            f32 minX = (f32)(x * currTilemap->tileSide);
            f32 minY = (f32)(y * currTilemap->tileSide);
            
            DrawRect(backBuffer, minX, minY, minX + currTilemap->tileSide, minY + currTilemap->tileSide, tileColour);
        }
    }
    
    f32 playerLeft = gameState->playerX - (0.5f * playerWidth);
    f32 playerTop = gameState->playerY - playerHeight;
    
    DrawRect(backBuffer, playerLeft, playerTop, playerLeft + playerWidth, playerTop + playerHeight, 0xFF0000FF);
}

extern "C" GAME_GET_AUDIO_SAMPLES(Game_GetAudioSamples)
{
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    Game_OutputSound(audioBuffer, 256);
}