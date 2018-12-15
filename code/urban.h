/*
Project: Urban Sprawl PoC
File: urban.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_H

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

#include <math.h>
#include <stdio.h>
#include <malloc.h>

#include "urban_memory.h"

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
#define INCOMING_PACKET_SIZE 128

#define PI 3.14159265359f

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

struct Game_State
{
    s32 playerX;
    s32 playerY;
    s32 toneHz;
};

struct Game_NetworkPacket
{
    u8 data[INCOMING_PACKET_SIZE];
};

struct InterpretedNetworkData
{
    s32 playerX;
    s32 playerY;
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

inline void ConcatenateStrings(mem_index sourceACount, char *sourceA,
                               mem_index sourceBCount, char *sourceB,
                               mem_index destCount, char *dest)
{
	for (s32 i = 0; i < sourceACount; ++i)
	{
		*dest++ = *sourceA++;
	}
	
	for (s32 i = 0; i < sourceBCount; ++i)
	{
		*dest++ = *sourceB++;
	}
	
	*dest++ = 0;
}

inline s32 StringLength(char *string)
{
	s32 result = 0;
	while (*string++)
	{
		++result;
	}
	
	return result;
}

inline s32 GetHostPieceRange(u8 hostSegment)
{
    s32 result = 0;
    
    if (hostSegment > 99)
    {
        result = 3;
    }
    else if (hostSegment < 100 && hostSegment > 9)
    {
        result = 2;
    }
    else
    {
        result = 1;
    }
    
    return result;
}

inline char *ConstructIPString(u32 host, u16 port)
{
    // NOTE(bSalmon): Starts at 5 to accomodate the 3 periods, the colon, and the terminator
    s32 charAmount = 5;
    
    u8 ip1 = (host >> 24) & 0xFF;
    charAmount+= GetHostPieceRange(ip1);
    
    u8 ip2 = (host >> 16) & 0xFF;
    charAmount += GetHostPieceRange(ip2);
    
    u8 ip3 = (host >> 8) & 0xFF;
    charAmount += GetHostPieceRange(ip3);
    
    u8 ip4 = host & 0xFF;
    charAmount += GetHostPieceRange(ip4);
    
    if (port > 9999)
    {
        charAmount += 5;
    }
    else if (port < 10000 && port > 999)
    {
        charAmount += 4;
    }
    else if (port < 1000 && port > 99)
    {
        charAmount += 3;
    }
    else if (port < 100 && port > 9)
    {
        charAmount += 2;
    }
    else
    {
        charAmount += 1;
    }
    
    mem_index stringSize = (sizeof(char) * charAmount);
    char *resultString = (char *)malloc(stringSize);
    
    sprintf_s(resultString, stringSize, "%d.%d.%d.%d:%d\0", ip4, ip3, ip2, ip1, port);
    
    return resultString;
}

// Use memcpy to copy variables into a data buffer and progress the index so the next slot can be filled
// (and vice versa)
template <class varType>
inline void U8DataBlockFill(u8 *dataBlockStart, varType *var, s32 *index)
{
    CopyMem(dataBlockStart, var, sizeof(*var));
    *index += sizeof(*var);
}

template <class varType>
inline void FillVariableFromDataBlock(varType *var, u8 *dataBlockStart, s32 *index)
{
    CopyMem(var, dataBlockStart, sizeof(*var));
    *index += sizeof(*var);
}

#define GAME_UPDATE_RENDER(funcName) void funcName(Game_BackBuffer *backBuffer, Game_Input *input, Game_Memory *memory, b32 multiplayer, Game_NetworkPacket *networkPacket)
typedef GAME_UPDATE_RENDER(game_updateRender);

#define GAME_GET_AUDIO_SAMPLES(funcName) void funcName(Game_Memory *memory, Game_AudioBuffer *audioBuffer)
typedef GAME_GET_AUDIO_SAMPLES(game_getAudioSamples);

#define URBAN_H
#endif // URBAN_H