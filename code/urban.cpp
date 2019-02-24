/*
Project: Urban Sprawl
File: urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "urban.h"

#include "urban_tile.cpp"
#include "urban_bitmap.cpp"

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
    FillVariableFromDataBlock<TileMapPosition, u8>(&result.playerPos, &networkPacket->data[index], &index);
    
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
            if ((pixel > backBuffer->memory) && ((pixel + 4) <= endOfBuffer))
            {
                *(u32 *)pixel = colour;
            }
            
            pixel += backBuffer->pitch;
        }
    }
}

// NOTE(bSalmon): The Input, Update and Render functions were abstracted from Game_UpdateRender so that the server can use the same functions without the Rendering capability

internal_func void InitMap(Game_State *gameState, Game_Memory *memory)
{
    InitMemRegion(&gameState->worldRegion, memory->permanentStorageSize - sizeof(Game_State), (u8 *)memory->permanentStorage + sizeof(Game_State));
    
    gameState->world = PushStruct(&gameState->worldRegion, World);
    World *world = gameState->world;
    world->tileMap = PushStruct(&gameState->worldRegion, TileMap);
    
    TileMap *tileMap = world->tileMap;
    
    tileMap->chunkShift = 4;
    tileMap->chunkMask = (1 << tileMap->chunkShift) - 1;
    tileMap->chunkDim = (1 << tileMap->chunkShift);
    
    tileMap->chunkCount = v3<u32>{128, 128, 2};
    tileMap->chunks = PushArray(&gameState->worldRegion, tileMap->chunkCount.x * tileMap->chunkCount.y * tileMap->chunkCount.z, TileChunk);
    
    tileMap->tileSidePixels = 70;
    tileMap->tileSideMeters = 2.0f;
    tileMap->metersToPixels = (f32)tileMap->tileSidePixels / tileMap->tileSideMeters;
    
#if URBAN_INTERNAL
    u32 *mapReadBlock = (u32 *)calloc((((tileMap->chunkCount.x * tileMap->chunkCount.y) *
                                        (tileMap->chunkDim * tileMap->chunkDim)) *
                                       tileMap->chunkCount.z), sizeof(u32));
    
    Debug_ReadFileResult fileResult = memory->Debug_PlatformReadFile("map1.usm");
    
    if (fileResult.contents)
    {
        mapReadBlock = (u32 *)fileResult.contents;
        
        for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
        {
            for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCount.y); ++y)
            {
                for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCount.x); ++x)
                {
                    SetTileValue(&gameState->worldRegion, tileMap, v3<u32>{x, y, z},  mapReadBlock[(z * ((tileMap->chunkDim * tileMap->chunkCount.x) * (tileMap->chunkDim * tileMap->chunkCount.y))) +
                                 (y * (tileMap->chunkDim * tileMap->chunkCount.x)) + x]);
                }
            }
        }
        
        memory->Debug_PlatformFreeFileMem(fileResult.contents);
#endif
    }
    else
    {
        for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
        {
            for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCount.y); ++y)
            {
                for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCount.x); ++x)
                {
                    TileChunkPosition chunkPos = GetChunkPosition(tileMap, v3<u32>{x, y, z});
                    TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
                    
                    SetTileValue(tileMap, chunk, chunkPos.relTile, 0);
                }
            }
        }
    }
}

internal_func b32 Input(Game_Input *input, Game_State *gameState, TileMap *tileMap, f32 playerWidth)
{
    // NOTE(bSalmon): Returns a b32 so the server knows when to send back a packet, not used for singleplayer
    b32 result = false;
    
    TileMapPosition oldPlayerPos = gameState->playerPos;
    for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
    {
        v2<f32> ddPlayer = {};
        
        Game_Controller *controller = GetGameController(input, controllerIndex);
        if (controller->isAnalog)
        {
            ddPlayer.x += controller->lAverageX;
            ddPlayer.y += controller->lAverageY;
        }
        else
        {
            if (controller->dPadLeft.endedDown)
            {
                ddPlayer.x = -1.0f;
                gameState->playerDir = FACING_LEFT;
            }
            if (controller->dPadRight.endedDown)
            {
                ddPlayer.x = 1.0f;
                gameState->playerDir = FACING_RIGHT;
            }
            if (controller->dPadUp.endedDown)
            {
                ddPlayer.y = -1.0f;
                gameState->playerDir = FACING_BACK;
            }
            if (controller->dPadDown.endedDown)
            {
                ddPlayer.y = 1.0f;
                gameState->playerDir = FACING_FRONT;
            }
            
            if (controller->faceDown.endedDown)
            {
                ddPlayer *= 50.0f;
            }
            else
            {
                ddPlayer *= 10.0f;
            }
        }
        
        ddPlayer += (gameState->dPlayer * -1.5f);
        
        TileMapPosition newPlayerPos = gameState->playerPos;
        v2<f32> playerDelta = ((ddPlayer * 0.5f) * Sq(input->deltaTime)) +
            gameState->dPlayer * input->deltaTime;
        newPlayerPos.tileRel += playerDelta;
        gameState->dPlayer = (ddPlayer * input->deltaTime) + gameState->dPlayer;
        
        // P = 0.5 * At^2 + Vt + oP
        // V = At + oV
        // A = ddPlayer
        
        newPlayerPos = RecanonicalisePos(tileMap, newPlayerPos);
        
        TileMapPosition playerLeft = newPlayerPos;
        playerLeft.tileRel.x -= 0.5f * playerWidth;
        playerLeft = RecanonicalisePos(tileMap, playerLeft);
        
        TileMapPosition playerRight = newPlayerPos;
        playerRight.tileRel.x += 0.5f * playerWidth;
        playerRight = RecanonicalisePos(tileMap, playerRight);
        
        b32 collided = false;
        TileMapPosition collisionPoint = {};
        
        // Check Left, Right, and Middle of the Player for collision 
        if (!IsTileMapPointValid(tileMap, newPlayerPos))
        {
            collisionPoint = newPlayerPos;
            collided = true;
        }
        if (!IsTileMapPointValid(tileMap, playerLeft))
        {
            collisionPoint = playerLeft;
            collided = true;
        }
        if (!IsTileMapPointValid(tileMap, playerRight))
        {
            collisionPoint = playerRight;
            collided = true;
        }
        
        if (collided)
        {
            v2<f32> normal = {0, 0};
            if (collisionPoint.absTile.x < gameState->playerPos.absTile.x)
            {
                normal = v2<f32>{1, 0};
            }
            if (collisionPoint.absTile.x > gameState->playerPos.absTile.x)
            {
                normal = v2<f32>{-1, 0};
            }
            if (collisionPoint.absTile.y > gameState->playerPos.absTile.y)
            {
                normal = v2<f32>{0, 1};
            }
            if (collisionPoint.absTile.y < gameState->playerPos.absTile.y)
            {
                normal = v2<f32>{0, -1};
            }
            
            gameState->dPlayer = gameState->dPlayer - normal * (1 * Inner(gameState->dPlayer, normal));
        }
        else
        {
            gameState->playerPos = newPlayerPos;
            result = true;
        }
    }
    
    if (!OnSameTile(&gameState->playerPos, &oldPlayerPos))
    {
        u32 newTileValue = GetTileValue(tileMap, gameState->playerPos);
        
        if (newTileValue == TILE_STAIRS_UP)
        {
            ++gameState->playerPos.absTile.z;
        }
        else if (newTileValue == TILE_STAIRS_DOWN)
        {
            --gameState->playerPos.absTile.z;
        }
    }
    
    return result;
}

internal_func void Render(Game_BackBuffer *backBuffer, Game_State *gameState, TileMap *tileMap, f32 playerWidth, f32 playerHeight)
{
    gameState->cameraPos = gameState->playerPos;
    
    DrawRect(backBuffer, 0.0f, 0.0f, (f32)backBuffer->width, (f32)backBuffer->height, 0xFF4C0296);
    
    v2<f32> screenCenter = v2<f32>{(f32)backBuffer->width, (f32)backBuffer->height};
    screenCenter *= 0.5f;
    
    for (s32 relY = -10; relY < 10; ++relY)
    {
        for (s32 relX = -20; relX < 20; ++relX)
        {
            u32 x = gameState->cameraPos.absTile.x + relX;
            u32 y = gameState->cameraPos.absTile.y - relY;
            u32 tileID = GetTileValue(tileMap, v3<u32>{x, y, gameState->cameraPos.absTile.z});
            
            if (tileID != TILE_EMPTY)
            {
                u32 tileColour = 0xFF888888;
                if (tileID == TILE_WALL)
                {
                    tileColour = 0xFFFFFFFF;
                }
                else if (tileID == TILE_STAIRS_UP)
                {
                    tileColour = 0xFF00FFFF;
                }
                else if (tileID == TILE_STAIRS_DOWN)
                {
                    tileColour = 0xFF008080;
                }
                
                if ((x == gameState->playerPos.absTile.x) &&
                    (y == gameState->playerPos.absTile.y))
                {
                    tileColour = 0xFF000000;
                }
                
                v2<f32> center = {};
                center.x = screenCenter.x - (tileMap->metersToPixels * gameState->cameraPos.tileRel.x) + ((f32)relX * tileMap->tileSidePixels);
                center.y = screenCenter.y - (tileMap->metersToPixels * gameState->cameraPos.tileRel.y) - ((f32)relY * tileMap->tileSidePixels);
                
                v2<f32> min = center - (0.5f * tileMap->tileSidePixels);
                v2<f32> max = center + (0.5f * tileMap->tileSidePixels);
                
                DrawRect(backBuffer, min.x, min.y, min.x + tileMap->tileSidePixels, min.y + tileMap->tileSidePixels, tileColour);
            }
        }
    }
    
    f32 playerLeft = screenCenter.x - (0.5f * tileMap->metersToPixels * playerWidth);
    f32 playerTop = screenCenter.y - (tileMap->metersToPixels * playerHeight);
    
    DrawRect(backBuffer, playerLeft, playerTop, playerLeft + (tileMap->metersToPixels * playerWidth), playerTop + (tileMap->metersToPixels * playerHeight), 0xFF0000FF);
    
    CharacterBitmaps *charBitmaps = &gameState->playerBitmaps[gameState->playerDir];
    DrawBitmap(backBuffer, &charBitmaps->feet, screenCenter, charBitmaps->alignX, charBitmaps->alignY);
    DrawBitmap(backBuffer, &charBitmaps->legs, screenCenter, charBitmaps->alignX, charBitmaps->alignY);
    DrawBitmap(backBuffer, &charBitmaps->torso, screenCenter, charBitmaps->alignX, charBitmaps->alignY);
    DrawBitmap(backBuffer, &charBitmaps->head, screenCenter, charBitmaps->alignX, charBitmaps->alignY);
}

extern "C" GAME_UPDATE_RENDER(Game_UpdateRender)
{
    ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
    
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    if (!memory->memInitialised)
    {
        gameState->playerBitmaps[FACING_FRONT].head = Debug_LoadBMP("test/test_headF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].torso = Debug_LoadBMP("test/test_torsoF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].legs = Debug_LoadBMP("test/test_legsF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].feet = Debug_LoadBMP("test/test_feetF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].alignX = 37;
        gameState->playerBitmaps[FACING_FRONT].alignY = 102;
        
        gameState->playerBitmaps[FACING_LEFT] = gameState->playerBitmaps[0];
        gameState->playerBitmaps[FACING_LEFT].head = Debug_LoadBMP("test/test_headL.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_LEFT].torso = Debug_LoadBMP("test/test_torsoL.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_LEFT].legs = Debug_LoadBMP("test/test_legsL.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_LEFT].feet = Debug_LoadBMP("test/test_feetL.bmp", memory->Debug_PlatformReadFile);
        
        gameState->playerBitmaps[FACING_BACK] = gameState->playerBitmaps[0];
        gameState->playerBitmaps[FACING_BACK].head = Debug_LoadBMP("test/test_headB.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_BACK].torso = Debug_LoadBMP("test/test_torsoB.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_BACK].legs = Debug_LoadBMP("test/test_legsB.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_BACK].feet = Debug_LoadBMP("test/test_feetB.bmp", memory->Debug_PlatformReadFile);
        
        gameState->playerBitmaps[FACING_RIGHT] = gameState->playerBitmaps[0];
        gameState->playerBitmaps[FACING_RIGHT].head = Debug_LoadBMP("test/test_headR.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_RIGHT].torso = Debug_LoadBMP("test/test_torsoR.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_RIGHT].legs = Debug_LoadBMP("test/test_legsR.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_RIGHT].feet = Debug_LoadBMP("test/test_feetR.bmp", memory->Debug_PlatformReadFile);
        
        gameState->playerPos = {};
        gameState->cameraPos = gameState->playerPos;
        
        InitMap(gameState, memory);
        
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
        Input(input, gameState, tileMap, playerWidth);
    }
    
    Render(backBuffer, gameState, tileMap, playerWidth, playerHeight);
    
}

extern "C" GAME_GET_AUDIO_SAMPLES(Game_GetAudioSamples)
{
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    Game_OutputSound(audioBuffer, 256);
}