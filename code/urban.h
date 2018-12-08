/*
Project: Urban Sprawl PoC
File: urban.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_H

// Utilities
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define SWAP(a, b) {decltype(a) temp = a; a = b; b = temp;}

#if URBAN_SLOW
#define ASSERT(expr) if(!(expr)) {*(s32 *)0 = 0;}
#else
#define ASSERT(expr)
#endif

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value)*1024LL)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)
#define TERABYTES(value) (GIGABYTES(value)*1024LL)

#define PORT 1842
#define SOCKET_BUFFER_SIZE 1024

#define PI 3.14159265359f

#if URBAN_INTERNAL
struct Debug_ReadFileResult
{
    u32 contentsSize;
    void *contents;
};

internal_func Debug_ReadFileResult Debug_PlatformReadFile(char *filename);
internal_func void Debug_PlatformFreeFileMem(void *memory);
internal_func b32 Debug_PlatformWriteFile(char *filename, u32 memSize, void *memory);
#endif

struct Game_BackBuffer
{
    // NOTE[bSalmon]: 32-bit wide, Mem Order BB GG RR xx
    void *memory;
    s32 width;
    s32 height;
    s32 bytesPerPixel;
    s32 pitch;
};

struct Game_AudioBuffer
{
    s32 samplesPerSecond;
    s32 sampleCount;
    s16 *samples;
};

struct Game_ButtonState
{
    s32 halfTransitionCount;
    b32 endedDown;
};

struct Game_Controller
{
    b32 isConnected;
    b32 isAnalog;
    
    f32 lAverageX;
    f32 lAverageY;
    f32 rAverageX;
    f32 rAverageY;
    
    union
    {
        Game_ButtonState buttons[21];
        struct
        {
            Game_ButtonState lStickUp;
            Game_ButtonState lStickDown;
            Game_ButtonState lStickLeft;
            Game_ButtonState lStickRight;
            
            Game_ButtonState rStickUp;
            Game_ButtonState rStickDown;
            Game_ButtonState rStickLeft;
            Game_ButtonState rStickRight;
            
            Game_ButtonState dPadUp;
            Game_ButtonState dPadDown;
            Game_ButtonState dPadLeft;
            Game_ButtonState dPadRight;
            
            Game_ButtonState faceUp;
            Game_ButtonState faceDown;
            Game_ButtonState faceLeft;
            Game_ButtonState faceRight;
            
            Game_ButtonState lShoulder;
            Game_ButtonState rShoulder;
            Game_ButtonState lThumbClick;
            Game_ButtonState rThumbClick;
            
            Game_ButtonState back;
            Game_ButtonState start;
            Game_ButtonState console;
        };
    };
};

struct Game_Input
{
    Game_ButtonState mouseButtons[3];
    s32 mouseX;
    s32 mouseY;
    s32 mouseZ;
    
    f32 deltaTime;
    
    Game_Controller controllers[2];
};

struct Game_Memory
{
    b32 memInitialised;
    
    u64 permanentStorageSize;
    void *permanentStorage;
    
    u64 transientStorageSize;
    void *transientStorage;
};

struct Game_State
{
    s32 xOffset;
    s32 yOffset;
    s32 toneHz;
};

inline Game_Controller *GetGameController(Game_Input *input, s32 controllerIndex)
{
    ASSERT(controllerIndex < ARRAY_COUNT(input->controllers));
    return &input->controllers[controllerIndex];
}

inline u32 SafeTruncateU64(u64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return result;
}

internal_func void Game_GetAudioSamples(Game_Memory *memory, Game_AudioBuffer *audioBuffer);
internal_func void Game_UpdateRender(Game_BackBuffer *backBuffer, Game_AudioBuffer *audioBuffer, Game_Input *input, Game_Memory *memory);

#define URBAN_H
#endif // URBAN_H