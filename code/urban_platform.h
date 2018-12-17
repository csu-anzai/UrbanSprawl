/*
Project: Urban Sprawl PoC
File: urban.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_PLATFORM_H

#define PORT 1842
#define INCOMING_PACKET_SIZE 128

// Static Definitions
#define internal_func static
#define local_persist static
#define global_var static

// Typedefs
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t mem_index;

typedef uintptr_t umm;
typedef intptr_t smm;

typedef int32_t b32;
typedef bool b8;

typedef float f32;
typedef double f64;

#if URBAN_INTERNAL
struct Debug_ReadFileResult
{
    u32 contentsSize;
    void *contents;
};

#define DEBUG_PLATFORM_READ_FILE(funcName) Debug_ReadFileResult funcName(char *filename)
typedef DEBUG_PLATFORM_READ_FILE(debug_platformReadFile);

#define DEBUG_PLATFORM_FREE_FILE_MEM(funcName) void funcName(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEM(debug_platformFreeFileMem);

#define DEBUG_PLATFORM_WRITE_FILE(funcName) b32 funcName(char *filename, u32 memSize, void *memory)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platformWriteFile);
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
    s32 sampleRate;
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
    
    debug_platformReadFile *Debug_PlatformReadFile;
    debug_platformFreeFileMem *Debug_PlatformFreeFileMem;
    debug_platformWriteFile *Debug_PlatformWriteFile;
};

struct Game_NetworkPacket
{
    u8 data[INCOMING_PACKET_SIZE];
};

#define GAME_UPDATE_RENDER(funcName) void funcName(Game_BackBuffer *backBuffer, Game_Input *input, Game_Memory *memory, b32 multiplayer, Game_NetworkPacket *networkPacket)
typedef GAME_UPDATE_RENDER(game_updateRender);

#define GAME_GET_AUDIO_SAMPLES(funcName) void funcName(Game_Memory *memory, Game_AudioBuffer *audioBuffer)
typedef GAME_GET_AUDIO_SAMPLES(game_getAudioSamples);

#define URBAN_PLATFORM_H
#endif