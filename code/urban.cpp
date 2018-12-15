/*
Project: Urban Sprawl
File: urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "urban.h"

#include <stdio.h>

internal_func void RenderPlayer(Game_BackBuffer *backBuffer, s32 playerX, s32 playerY)
{
    u8 *endOfBuffer = (u8 *)backBuffer->memory + (backBuffer->pitch * backBuffer->height);
    
    u32 colour = 0xFFFFFFFF;
    s32 top = playerY;
    s32 bottom = playerY + 10;
    for (s32 x = playerX; x < (playerX + 10); ++x)
    {
        u8 *pixel = (u8 *)backBuffer->memory + (x * backBuffer->bytesPerPixel) + (top * backBuffer->pitch);
        
        for (s32 y = top; y < bottom; ++y)
        {
            if ((pixel >= backBuffer->memory) && ((pixel + 4) <= endOfBuffer))
            {
                *(u32 *)pixel = colour;
            }
            
            pixel += backBuffer->pitch;
        }
    }
}

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
    FillVariableFromDataBlock(&result.toneHz, &networkPacket->data[index], &index);
    
    printf("\n\nplayerX: %d\n", result.playerX);
    printf("playerY: %d\n", result.playerY);
    printf("toneHz: %d\n", result.toneHz);
    
    return result;
}

extern "C" GAME_UPDATE_RENDER(Game_UpdateRender)
{
    ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
    
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    if (!memory->memInitialised)
    {
        gameState->playerX = 100;
        gameState->playerY = 100;
        gameState->toneHz = 256;
        memory->memInitialised = true;
    }
    
    if (multiplayer)
    {
        InterpretedNetworkData networkData = {};
        networkData = InterpretNetworkData(networkPacket);
        gameState->playerX += networkData.playerX;
        gameState->playerY += networkData.playerY;
        gameState->toneHz = networkData.toneHz;
        
        if (gameState->toneHz == 0)
        {
            gameState->toneHz = 256;
        }
    }
    else
    {
        for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
        {
            Game_Controller *controller = GetGameController(input, controllerIndex);
            if (controller->isAnalog)
            {
                gameState->playerX += (s32)(4.0f * controller->lAverageX);
                gameState->playerY -= (s32)(4.0f * controller->lAverageY);
            }
            else
            {
                if (controller->dPadLeft.endedDown)
                {
                    gameState->playerX -= 4;
                }
                if (controller->dPadRight.endedDown)
                {
                    gameState->playerX += 4;
                }
                if (controller->dPadUp.endedDown)
                {
                    gameState->playerY -= 4;
                }
                if (controller->dPadDown.endedDown)
                {
                    gameState->playerY += 4;
                }
            }
        }
    }
    
    backBuffer->memory = SetMem(backBuffer->memory, 0, (backBuffer->width * backBuffer->height) * backBuffer->bytesPerPixel);
    RenderPlayer(backBuffer, gameState->playerX, gameState->playerY);
}

extern "C" GAME_GET_AUDIO_SAMPLES(Game_GetAudioSamples)
{
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    Game_OutputSound(audioBuffer, gameState->toneHz);
}