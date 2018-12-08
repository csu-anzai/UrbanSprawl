/*
Project: Urban Sprawl
File: urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "urban.h"

internal_func void Game_RenderTestPixels(Game_BackBuffer *backBuffer, s32 xOffset, s32 yOffset)
{
    u8 *row = (u8 *)backBuffer->memory;
    for(s32 y = 0; y < backBuffer->height; ++y)
    {
        u32 *pixel = (u32 *)row;
        
        for(s32 x = 0; x < backBuffer->width; ++x)
        {
            u8 blue = (u8)(x + xOffset);
            u8 green = (u8)(y + yOffset);
            
            *pixel++ = ((green << 8) | blue);
        }
        
        row += backBuffer->pitch;
    }
}

internal_func void Game_OutputSound(Game_AudioBuffer *audioBuffer, s32 toneHz)
{
    local_persist f32 tSine;
    s16 toneVolume = 3000;
    s32 wavePeriod = audioBuffer->samplesPerSecond/toneHz;
    
    s16 *sampleOut = audioBuffer->samples;
    for(int sampleIndex = 0; sampleIndex < audioBuffer->sampleCount; ++sampleIndex)
    {
        f32 sineValue = sinf(tSine);
        s16 sampleValue = (s16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        tSine += 2.0f * PI * 1.0f / (f32)wavePeriod;
        
        if (tSine > 2.0f * PI)
        {
            tSine -= 2.0f * PI;
        }
    }
}

internal_func void Game_UpdateRender(Game_BackBuffer *backBuffer, Game_Input *input, Game_Memory *memory)
{
    ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
    
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    if (!memory->memInitialised)
    {
        gameState->toneHz = 256;
        memory->memInitialised = true;
    }
    
    for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
    {
        Game_Controller *controller = GetGameController(input, controllerIndex);
        if (controller->isAnalog)
        {
            gameState->xOffset += (s32)(4.0f * controller->lAverageX);
            gameState->yOffset -= (s32)(4.0f * controller->lAverageY);
            gameState->toneHz = 256 + (s32)(128.0f * (controller->rAverageY));
        }
        else
        {
            if (controller->dPadLeft.endedDown)
            {
                gameState->xOffset -= 4;
            }
            if (controller->dPadRight.endedDown)
            {
                gameState->xOffset += 4;
            }
            if (controller->dPadUp.endedDown)
            {
                gameState->yOffset -= 4;
            }
            if (controller->dPadDown.endedDown)
            {
                gameState->yOffset += 4;
            }
        }
        
        if (controller->faceDown.endedDown)
        {
            gameState->toneHz = 5;
        }
    }
    
    Game_RenderTestPixels(backBuffer, gameState->xOffset, gameState->yOffset);
}

internal_func void Game_GetAudioSamples(Game_Memory *memory, Game_AudioBuffer *audioBuffer)
{
    Game_State *gameState = (Game_State *)memory->permanentStorage;
    Game_OutputSound(audioBuffer, gameState->toneHz);
}