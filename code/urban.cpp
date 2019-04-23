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

internal_func void InitMap_Singleplayer(Game_State *gameState, Game_Memory *memory)
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
    
    Debug_ReadFileResult fileResult = memory->Debug_PlatformReadFile("map1.usm");
    
    if (fileResult.contents)
    {
        u8 *mapReadBlock = (u8 *)fileResult.contents;
        
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

// Initialise the Map from the connection packet in multiplayer
internal_func void InitMap_Multiplayer(Game_State *gameState, Game_Memory *memory, Game_NetworkPacket *gamePacket)
{
    InitMemRegion(&gameState->worldRegion, memory->permanentStorageSize - sizeof(Game_State), (u8 *)memory->permanentStorage + sizeof(Game_State));
    
    gameState->world = PushStruct(&gameState->worldRegion, World);
    World *world = gameState->world;
    world->tileMap = PushStruct(&gameState->worldRegion, TileMap);
    
    TileMap *tileMap = world->tileMap;
    
    // Start unpacking the server packet
    s32 unpackIndex = 0;
    FillVariableFromDataBlock<u8, u8>(&gameState->playerCount, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<v3<u32>, u8>(&tileMap->chunkCount, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<u32, u8>(&tileMap->chunkShift, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<u32, u8>(&tileMap->chunkMask, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<u32, u8>(&tileMap->chunkDim, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<s32, u8>(&tileMap->tileSidePixels, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<f32, u8>(&tileMap->tileSideMeters, gamePacket->data, &unpackIndex);
    FillVariableFromDataBlock<f32, u8>(&tileMap->metersToPixels, gamePacket->data, &unpackIndex);
    
    s32 coordIndex = unpackIndex;
    unpackIndex += (sizeof(u32) * tileMap->chunkCount.z) * 2;
    
    tileMap->chunks = PushArray(&gameState->worldRegion, tileMap->chunkCount.x * tileMap->chunkCount.y * tileMap->chunkCount.z, TileChunk);
    
    b32 yCoordChanged = false;
    for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
    {
        yCoordChanged = false;
        for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCount.y); ++y)
        {
            for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCount.x); ++x)
            {
                if (x >= gamePacket->data[coordIndex] || y >= gamePacket->data[coordIndex + 4])
                {
                    SetTileValue(&gameState->worldRegion, tileMap, v3<u32>{x, y, z}, TILE_EMPTY);
                    
                    if (y >= gamePacket->data[coordIndex + 4] && !yCoordChanged)
                    {
                        coordIndex += (sizeof(u32) * 2);
                        yCoordChanged = true;
                    }
                }
                else
                {
                    u8 tileID = 0;
                    FillVariableFromDataBlock<u8, u8>(&tileID, gamePacket->data, &unpackIndex);
                    SetTileValue(&gameState->worldRegion, tileMap, v3<u32>{x, y, z}, tileID);
                }
            }
        }
    }
}

// TODO(bSalmon): Should this just return ddPlayer or should ddPlayer be packaged elsewhere?
internal_func v2<f32> Input(Game_Input *input, Game_State *gameState, TileMap *tileMap, u8 clientID)
{
    v2<f32> ddPlayer = {};
    
    for (s32 controllerIndex = 0; controllerIndex < 2; ++controllerIndex)
    {
        ddPlayer = {};
        
        Game_Controller *controller = GetGameController(input, controllerIndex);
        if (controller->isAnalog)
        {
            ddPlayer.x = controller->lAverageX;
            ddPlayer.y = -controller->lAverageY;
            
            // TODO(bSalmon): Needs to be velocity based
            if (controller->lAverageX < 0)
            {
                gameState->players[clientID - 1].facingDir = FACING_LEFT;
            }
            if (controller->lAverageX > 0)
            {
                gameState->players[clientID - 1].facingDir = FACING_RIGHT;
            }
            if (controller->lAverageY < 0)
            {
                gameState->players[clientID - 1].facingDir = FACING_BACK;
            }
            if (controller->lAverageY > 0)
            {
                gameState->players[clientID - 1].facingDir = FACING_FRONT;
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
        else
        {
            if (controller->dPadLeft.endedDown)
            {
                ddPlayer.x = -1.0f;
                gameState->players[clientID - 1].facingDir = FACING_LEFT;
            }
            if (controller->dPadRight.endedDown)
            {
                ddPlayer.x = 1.0f;
                gameState->players[clientID - 1].facingDir = FACING_RIGHT;
            }
            if (controller->dPadUp.endedDown)
            {
                ddPlayer.y = -1.0f;
                gameState->players[clientID - 1].facingDir = FACING_BACK;
            }
            if (controller->dPadDown.endedDown)
            {
                ddPlayer.y = 1.0f;
                gameState->players[clientID - 1].facingDir = FACING_FRONT;
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
        
        if ((ddPlayer.x != 0.0f) || (ddPlayer.y != 0.0f))
        {
            break;
        }
    }
    
    return ddPlayer;
}

internal_func void MovePlayer(Game_Input *input, Game_State *gameState, TileMap *tileMap, v2<f32> ddPlayer, u8 clientID)
{
    TileMapPosition oldPlayerPos = gameState->players[clientID - 1].pos;
    
    ddPlayer += (gameState->players[clientID - 1].dPos * -1.5f);
    
    TileMapPosition newPlayerPos = gameState->players[clientID - 1].pos;
    v2<f32> playerDelta = ((ddPlayer * 0.5f) * Sq(input->deltaTime)) +
        gameState->players[clientID - 1].dPos * input->deltaTime;
    newPlayerPos.tileRel += playerDelta;
    gameState->players[clientID - 1].dPos = (ddPlayer * input->deltaTime) + gameState->players[clientID - 1].dPos;
    
    // P = 0.5 * At^2 + Vt + oP
    // V = At + oV
    // A = ddPlayer
    
    newPlayerPos = RecanonicalisePos(tileMap, newPlayerPos);
    
    TileMapPosition playerLeft = newPlayerPos;
    playerLeft.tileRel.x -= 0.5f * gameState->players[clientID - 1].dims.x;
    playerLeft = RecanonicalisePos(tileMap, playerLeft);
    
    TileMapPosition playerRight = newPlayerPos;
    playerRight.tileRel.x += 0.5f * gameState->players[clientID - 1].dims.x;
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
        if (collisionPoint.absTile.x < gameState->players[clientID - 1].pos.absTile.x)
        {
            normal = v2<f32>{1, 0};
        }
        if (collisionPoint.absTile.x > gameState->players[clientID - 1].pos.absTile.x)
        {
            normal = v2<f32>{-1, 0};
        }
        if (collisionPoint.absTile.y > gameState->players[clientID - 1].pos.absTile.y)
        {
            normal = v2<f32>{0, 1};
        }
        if (collisionPoint.absTile.y < gameState->players[clientID - 1].pos.absTile.y)
        {
            normal = v2<f32>{0, -1};
        }
        
        gameState->players[clientID - 1].dPos = gameState->players[clientID - 1].dPos - normal * (1 * Inner(gameState->players[clientID - 1].dPos, normal));
    }
    else
    {
        gameState->players[clientID - 1].pos = newPlayerPos;
    }
    
    if (!OnSameTile(&gameState->players[clientID - 1].pos, &oldPlayerPos))
    {
        u32 newTileValue = GetTileValue(tileMap, gameState->players[clientID - 1].pos);
        
        if (newTileValue == TILE_STAIRS_UP)
        {
            ++gameState->players[clientID - 1].pos.absTile.z;
            gameState->players[clientID - 1].pos.tileRel = v2<f32>{0.0f, 0.0f};
        }
        else if (newTileValue == TILE_STAIRS_DOWN)
        {
            --gameState->players[clientID - 1].pos.absTile.z;
            gameState->players[clientID - 1].pos.tileRel = v2<f32>{0.0f, 0.0f};
        }
    }
}

internal_func void Render(Game_BackBuffer *backBuffer, Game_State *gameState, TileMap *tileMap, u8 clientID)
{
    gameState->cameraPos = gameState->players[clientID - 1].pos;
    
    DrawRect(backBuffer, 0.0f, 0.0f, (f32)backBuffer->width, (f32)backBuffer->height, 0xFF4C0296);
    
    v2<f32> screenCenter = v2<f32>{(f32)backBuffer->width, (f32)backBuffer->height};
    screenCenter *= 0.5f;
    gameState->players[clientID - 1].screenPos = screenCenter;
    
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
                
                if ((x == gameState->players[0].pos.absTile.x) &&
                    (y == gameState->players[0].pos.absTile.y))
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
            
            for (u8 playerIndex = 0; playerIndex < gameState->playerCount; ++playerIndex)
            {
                if (playerIndex == clientID - 1)
                {
                    continue;
                }
                
                s32 xDiff = gameState->players[clientID - 1].pos.absTile.x - gameState->players[playerIndex].pos.absTile.x;
                s32 yDiff = gameState->players[clientID - 1].pos.absTile.y - gameState->players[playerIndex].pos.absTile.y;
                if (xDiff == relX && yDiff == relY)
                {
                    gameState->players[playerIndex].screenPos.x = screenCenter.x - (tileMap->metersToPixels * gameState->cameraPos.tileRel.x) - ((f32)relX * tileMap->tileSidePixels) + gameState->players[playerIndex].pos.tileRel.x;
                    gameState->players[playerIndex].screenPos.y = screenCenter.y - (tileMap->metersToPixels * gameState->cameraPos.tileRel.y) - ((f32)relY * tileMap->tileSidePixels) - gameState->players[playerIndex].pos.tileRel.y;
                }
            }
        }
    }
    
    f32 playerLeft = screenCenter.x - (0.5f * tileMap->metersToPixels * gameState->players[0].dims.x);
    f32 playerTop = screenCenter.y - (tileMap->metersToPixels * gameState->players[0].dims.y);
    
    DrawRect(backBuffer, playerLeft, playerTop, playerLeft + (tileMap->metersToPixels * gameState->players[clientID - 1].dims.x), playerTop + (tileMap->metersToPixels * gameState->players[clientID - 1].dims.y), 0xFF0000FF);
    
    for (u32 playerIndex = gameState->playerCount - 1; playerIndex < gameState->playerCount; --playerIndex)
    {
        if (gameState->cameraPos.absTile.z == gameState->players[playerIndex].pos.absTile.z)
        {
            CharacterBitmaps *charBitmaps = &gameState->playerBitmaps[gameState->players[playerIndex].facingDir];
            DrawBitmap(backBuffer, &charBitmaps->feet, gameState->players[playerIndex].screenPos, charBitmaps->alignX, charBitmaps->alignY);
            DrawBitmap(backBuffer, &charBitmaps->legs, gameState->players[playerIndex].screenPos, charBitmaps->alignX, charBitmaps->alignY);
            DrawBitmap(backBuffer, &charBitmaps->torso, gameState->players[playerIndex].screenPos, charBitmaps->alignX, charBitmaps->alignY);
            DrawBitmap(backBuffer, &charBitmaps->head, gameState->players[playerIndex].screenPos, charBitmaps->alignX, charBitmaps->alignY);
        }
    }
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
        
        if (multiplayer && isConnectPacket)
        {
            InitMap_Multiplayer(gameState, memory, networkPacket);
            gameState->cameraPos = gameState->players[clientID - 1].pos;
        }
        else
        {
            Player player = {};
            player.exists = true;
            player.pos = {};
            player.dPos = {};
            player.facingDir = FACING_FRONT;
            
            gameState->cameraPos = player.pos;
            
            gameState->players[0] = player;
            gameState->playerCount++;
            
            // NOTE(bSalmon): Test dummy
            gameState->players[1] = player;
            gameState->players[1].pos.absTile = v3<u32>{5, 5, 0};
            gameState->playerCount++;
            
            InitMap_Singleplayer(gameState, memory);
            
            gameState->players[0].dims.y = gameState->world->tileMap->tileSideMeters * 0.9f;
            gameState->players[0].dims.x = 0.5f * gameState->players[0].dims.y;
            gameState->players[1].dims = gameState->players[0].dims;
            
        }
        
        memory->memInitialised = true;
    }
    
    World *world = gameState->world;
    TileMap *tileMap = world->tileMap;
    
    if (multiplayer && !isConnectPacket && networkPacket->data)
    {
        s32 index = 0;
        for (u8 playerIndex = 0; playerIndex < 8; ++playerIndex)
        {
            FillVariableFromDataBlock<Player, u8>(&gameState->players[playerIndex], networkPacket->data, &index);
        }
    }
    else
    {
        // clientID needs to be 1 for singleplayer or the world eats itself
        v2<f32> ddPlayer = Input(input, gameState, tileMap, 1);
        MovePlayer(input, gameState, tileMap, ddPlayer, 1);
    }
    
    if (!multiplayer)
    {
        Render(backBuffer, gameState, tileMap, 1);
    }
    else
    {
        Render(backBuffer, gameState, tileMap, clientID);
    }
}

extern "C" GAME_GET_AUDIO_SAMPLES(Game_GetAudioSamples)
{
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    Game_OutputSound(audioBuffer, 256);
}