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

internal_func LoadedBitmap Debug_LoadBMP(char *filename, debug_platformReadFile *Debug_PlatformReadFile)
{
    LoadedBitmap result = {};
    
    Debug_ReadFileResult readResult = Debug_PlatformReadFile(filename);
    if (readResult.contentsSize != 0)
    {
        BitmapHeader *header = (BitmapHeader *)readResult.contents;
        u32 *pixels = (u32 *)((u8 *)readResult.contents + header->bitmapOffset);
        result.pixelData = pixels;
        result.width = header->width;
        result.height = header->height;
        
        ASSERT(header->compression == 3);
        
        // NOTE(bSalmon): Bytes order determined by the header
        u32 redMask = header->redMask;
        u32 greenMask = header->greenMask;
        u32 blueMask = header->blueMask;
        u32 alphaMask = ~(redMask | greenMask | blueMask);
        
        BitScanResult redShift = FindLeastSignificantSetBit(redMask);
        BitScanResult greenShift = FindLeastSignificantSetBit(greenMask);
        BitScanResult blueShift = FindLeastSignificantSetBit(blueMask);
        BitScanResult alphaShift = FindLeastSignificantSetBit(alphaMask);
        
        ASSERT(redShift.found);
        ASSERT(greenShift.found);
        ASSERT(blueShift.found);
        ASSERT(alphaShift.found);
        
        u32 *srcDest = pixels;
        for (s32 y = 0; y < header->height; ++y)
        {
            for (s32 x = 0; x < header->width; ++x)
            {
                u32 pixel = *srcDest;
                *srcDest++ = ((((pixel >> alphaShift.index) & 0xFF) << 24) |
                              (((pixel >> redShift.index) & 0xFF) << 16) |
                              (((pixel >> greenShift.index) & 0xFF) << 8) |
                              (((pixel >> blueShift.index) & 0xFF)));
            }
        }
    }
    
    return result;
}

internal_func void DrawBitmap(Game_BackBuffer *backBuffer, LoadedBitmap *bitmap, f32 fX, f32 fY, s32 alignX = 0, s32 alignY = 0)
{
    s32 minX = RoundF32ToS32(fX) + alignX;
    s32 minY = RoundF32ToS32(fY) + alignY;
    s32 maxX = RoundF32ToS32(fX + (f32)bitmap->width);
    s32 maxY = RoundF32ToS32(fY + (f32)bitmap->height);
    
    if (minX < 0)
    {
        minX = 0;
    }
    
    if (minY < 0)
    {
        minY = 0;
    }
    
    if (maxX > backBuffer->width)
    {
        maxX = backBuffer->width;
    }
    
    if (maxY > backBuffer->height)
    {
        maxY = backBuffer->height;
    }
    
    u32 *srcRow = bitmap->pixelData + bitmap->width * (bitmap->height - 1);
    u8 *destRow = (u8 *)backBuffer->memory + (minX * backBuffer->bytesPerPixel) + (minY * backBuffer->pitch);
    for (s32 y = minY; y < maxY; ++y)
    {
        u32 *dest = (u32 *)destRow;
        u32 *src = srcRow;
        for (s32 x = minX; x < maxX; ++x)
        {
            
            f32 a = (f32)((*src >> 24) & 0xFF) / 255.0f;
            f32 srcR = (f32)((*src >> 16) & 0xFF);
            f32 srcG = (f32)((*src >> 8) & 0xFF);
            f32 srcB = (f32)(*src & 0xFF);
            
            f32 destR = (f32)((*dest >> 16) & 0xFF);
            f32 destG = (f32)((*dest >> 8) & 0xFF);
            f32 destB = (f32)(*dest & 0xFF);
            
            f32 r = (1.0f - a) * destR + (a * srcR);
            f32 g = (1.0f - a) * destG + (a * srcG);
            f32 b = (1.0f - a) * destB + (a * srcB);
            
            *dest = (((u32)(r + 0.5f) << 16) |
                     ((u32)(g + 0.5f) << 8) |
                     (u32)(b + 0.5f));
            
            ++dest;
            ++src;
        }
        
        destRow += backBuffer->pitch;
        srcRow -= bitmap->width;
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
    
    tileMap->chunkCountX = 128;
    tileMap->chunkCountY = 128;
    tileMap->chunkCountZ = 2;
    tileMap->chunks = PushArray(&gameState->worldRegion, tileMap->chunkCountX * tileMap->chunkCountY * tileMap->chunkCountZ, TileChunk);
    
    tileMap->tileSidePixels = 70;
    tileMap->tileSideMeters = 2.0f;
    tileMap->metersToPixels = (f32)tileMap->tileSidePixels / tileMap->tileSideMeters;
    
#if URBAN_INTERNAL
    u32 *mapReadBlock = (u32 *)calloc((((tileMap->chunkCountX * tileMap->chunkCountY) *
                                        (tileMap->chunkDim * tileMap->chunkDim)) *
                                       tileMap->chunkCountZ), sizeof(u32));
    
    Debug_ReadFileResult fileResult = memory->Debug_PlatformReadFile("map1.usm");
    
    if (fileResult.contents)
    {
        mapReadBlock = (u32 *)fileResult.contents;
        
        for (u32 z = 0; z < tileMap->chunkCountZ; ++z)
        {
            for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCountY); ++y)
            {
                for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCountX); ++x)
                {
                    SetTileValue(&gameState->worldRegion, tileMap, x, y, z,  mapReadBlock[(z * ((tileMap->chunkDim * tileMap->chunkCountX) * (tileMap->chunkDim * tileMap->chunkCountY))) +
                                 (y * (tileMap->chunkDim * tileMap->chunkCountX)) + x]);
                }
            }
        }
        
        memory->Debug_PlatformFreeFileMem(fileResult.contents);
#endif
    }
    else
    {
        for (u32 z = 0; z < tileMap->chunkCountZ; ++z)
        {
            for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCountY); ++y)
            {
                for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCountX); ++x)
                {
                    TileChunkPosition chunkPos = GetChunkPosition(tileMap, x, y, z);
                    TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunkX, chunkPos.chunkY, chunkPos.chunkZ);
                    
                    SetTileValue(tileMap, chunk, chunkPos.relTileX, chunkPos.relTileY, 0);
                }
            }
        }
    }
}

internal_func b32 Input(Game_Input *input, Game_State *gameState, TileMap *tileMap, f32 playerWidth)
{
    // NOTE(bSalmon): Returns a b32 so the server knows when to send back a packet, not used for singleplayer
    
    for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
    {
        f32 deltaPlayerX = 0.0f;
        f32 deltaPlayerY = 0.0f;
        
        Game_Controller *controller = GetGameController(input, controllerIndex);
        if (controller->isAnalog)
        {
            deltaPlayerX += controller->lAverageX;
            deltaPlayerY += controller->lAverageY;
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
                deltaPlayerY = -1.0f;
            }
            if (controller->dPadDown.endedDown)
            {
                deltaPlayerY = 1.0f;
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
        
        if (deltaPlayerX < 0.0f)
        {
            gameState->playerDir = FACING_LEFT;
        }
        else if (deltaPlayerX > 0.0f)
        {
            gameState->playerDir = FACING_RIGHT;
        }
        else if (deltaPlayerY < 0.0f)
        {
            gameState->playerDir = FACING_BACK;
        }
        else if (deltaPlayerY > 0.0f)
        {
            gameState->playerDir = FACING_FRONT;
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
            if (!OnSameTile(&gameState->playerPos, &newPlayerPos))
            {
                u32 newTileValue = GetTileValue(tileMap, newPlayerPos);
                
                if (newTileValue == TILE_STAIRS_UP)
                {
                    ++newPlayerPos.absTileZ;
                }
                else if (newTileValue == TILE_STAIRS_DOWN)
                {
                    --newPlayerPos.absTileZ;
                }
            }
            
            gameState->playerPos = newPlayerPos;
            return true;
        }
    }
    
    return false;
}

internal_func void Render(Game_BackBuffer *backBuffer, Game_State *gameState, TileMap *tileMap, f32 playerWidth, f32 playerHeight)
{
    gameState->cameraPos = gameState->playerPos;
    
    DrawRect(backBuffer, 0.0f, 0.0f, (f32)backBuffer->width, (f32)backBuffer->height, 0xFF4C0296);
    DrawBitmap(backBuffer, &gameState->background, 0, 0);
    
    f32 screenCenterX = 0.5f * (f32)backBuffer->width;
    f32 screenCenterY = 0.5f * (f32)backBuffer->height;
    
    for (s32 relY = -10; relY < 10; ++relY)
    {
        for (s32 relX = -20; relX < 20; ++relX)
        {
            u32 x = gameState->cameraPos.absTileX + relX;
            u32 y = gameState->cameraPos.absTileY - relY;
            u32 tileID = GetTileValue(tileMap, x, y, gameState->cameraPos.absTileZ);
            
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
                
                if ((x == gameState->playerPos.absTileX) &&
                    (y == gameState->playerPos.absTileY))
                {
                    tileColour = 0xFF000000;
                }
                
                f32 centerX = screenCenterX - (tileMap->metersToPixels * gameState->cameraPos.tileRelX) + ((f32)relX * tileMap->tileSidePixels);
                f32 centerY = screenCenterY - (tileMap->metersToPixels * gameState->cameraPos.tileRelY) - ((f32)relY * tileMap->tileSidePixels);
                f32 minX = centerX - (0.5f * tileMap->tileSidePixels);
                f32 minY = centerY - (0.5f * tileMap->tileSidePixels);
                f32 maxX = centerX + (0.5f * tileMap->tileSidePixels);
                f32 maxY = centerY + (0.5f * tileMap->tileSidePixels);
                
                DrawRect(backBuffer, minX, minY, minX + tileMap->tileSidePixels, minY + tileMap->tileSidePixels, tileColour);
            }
        }
    }
    
    f32 playerLeft = screenCenterX  - (0.5f * tileMap->metersToPixels * playerWidth);
    f32 playerTop = screenCenterY - (tileMap->metersToPixels * playerHeight);
    
    DrawRect(backBuffer, playerLeft, playerTop, playerLeft + (tileMap->metersToPixels * playerWidth), playerTop + (tileMap->metersToPixels * playerHeight), 0xFF0000FF);
    
    CharacterBitmaps *charBitmaps = &gameState->playerBitmaps[gameState->playerDir];
    DrawBitmap(backBuffer, &charBitmaps->head, screenCenterX, screenCenterY, charBitmaps->alignX, charBitmaps->alignY);
    DrawBitmap(backBuffer, &charBitmaps->torso, screenCenterX, screenCenterY, charBitmaps->alignX, charBitmaps->alignY);
    DrawBitmap(backBuffer, &charBitmaps->legs, screenCenterX, screenCenterY, charBitmaps->alignX, charBitmaps->alignY);
    DrawBitmap(backBuffer, &charBitmaps->feet, screenCenterX, screenCenterY, charBitmaps->alignX, charBitmaps->alignY);
}

extern "C" GAME_UPDATE_RENDER(Game_UpdateRender)
{
    ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
    
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    if (!memory->memInitialised)
    {
        gameState->background = Debug_LoadBMP("test/test_screen.bmp", memory->Debug_PlatformReadFile);
        
        gameState->playerBitmaps[FACING_FRONT].head = Debug_LoadBMP("test/test_headF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].torso = Debug_LoadBMP("test/test_torsoF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].legs = Debug_LoadBMP("test/test_legsF.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].feet = Debug_LoadBMP("test/test_feet.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_FRONT].alignX = 72;
        gameState->playerBitmaps[FACING_FRONT].alignY = 182;
        
        gameState->playerBitmaps[FACING_LEFT] = gameState->playerBitmaps[0];
        gameState->playerBitmaps[FACING_LEFT].head = Debug_LoadBMP("test/test_headL.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_LEFT].torso = Debug_LoadBMP("test/test_torsoL.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_LEFT].legs = Debug_LoadBMP("test/test_legsL.bmp", memory->Debug_PlatformReadFile);
        
        gameState->playerBitmaps[FACING_BACK] = gameState->playerBitmaps[0];
        gameState->playerBitmaps[FACING_BACK].head = Debug_LoadBMP("test/test_headB.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_BACK].torso = Debug_LoadBMP("test/test_torsoB.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_BACK].legs = Debug_LoadBMP("test/test_legsB.bmp", memory->Debug_PlatformReadFile);
        
        gameState->playerBitmaps[FACING_RIGHT] = gameState->playerBitmaps[0];
        gameState->playerBitmaps[FACING_RIGHT].head = Debug_LoadBMP("test/test_headR.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_RIGHT].torso = Debug_LoadBMP("test/test_torsoR.bmp", memory->Debug_PlatformReadFile);
        gameState->playerBitmaps[FACING_RIGHT].legs = Debug_LoadBMP("test/test_legsR.bmp", memory->Debug_PlatformReadFile);
        
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