/*
Project: Urban Sprawl PoC
File: sdl_urban.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef SDL_URBAN_H

#include "urban.h"

#if URBAN_WIN32
#include <Windows.h>
#endif

#include <stdio.h>

#define SDL_STATE_FILE_PATH_MAX 4096

struct SDLPL_BackBuffer
{
	void *memory;
	u32 memorySizeInBytes;
	SDL_Texture *texture;
	s32 width;
	s32 height;
	u8 bytesPerPixel;
	s32 pitch;
};

struct SDLPL_AudioRingBuffer
{
    s32 size;
    s32 writeCursor;
    s32 playCursor;
    void *data;
};

struct SDLPL_AudioOutput
{
	SDL_AudioDeviceID deviceID;
	s32 sampleRate;
    u32 runningSampleIndex;
	s32 bytesPerSample;
    s32 secondaryBufferSize;
    s32 safetyBytes;
};

struct SDLPL_OutgoingNetworkPacket
{
    u8 *data;
    s32 dataSize;
};

struct SDLPL_DebugTimeMarker
{
    s32 playCursor;
    s32 writeCursor;
};

struct SDLPL_WindowDimensions
{
	s32 width;
	s32 height;
};

#if URBAN_WIN32
struct SDLPL_GameCode
{
    HMODULE gameCodeDLL;
    FILETIME dllLastWriteTime;
    
    game_updateRender *UpdateRender;
    game_getAudioSamples *GetAudioSamples;
    
    b32 isValid;
};

struct SDLPL_ReplayBuffer
{
    HANDLE fileHandle;
    HANDLE memoryMap;
    char replayFilename[SDL_STATE_FILE_PATH_MAX];
    void *memBlock;
};
#endif

struct SDLPL_State
{
    u64 totalSize;
    void *gameMemBlock;
    
#if URBAN_WIN32
    char exeName[SDL_STATE_FILE_PATH_MAX];
    char *exeNameBegin;
    
    SDLPL_ReplayBuffer replayBuffers[4];
    
    HANDLE recordingHandle;
    s32 inputRecordingIndex;
    
    HANDLE playbackHandle;
    s32 inputPlaybackIndex;
#endif
};

inline SDLPL_WindowDimensions SDLPL_GetWindowDimensions(SDL_Window *window)
{
	SDLPL_WindowDimensions result = {};
	SDL_GetWindowSize(window, &result.width, &result.height);
	return result;
}

#define SDL_URBAN_H
#endif // SDL_URBAN_H
