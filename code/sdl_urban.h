/*
Project: Urban Sprawl PoC
File: sdl_urban.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef SDL_URBAN_H

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
	s32 samplesPerSecond;
    u32 runningSampleIndex;
	s32 bytesPerSample;
    s32 secondaryBufferSize;
    s32 safetyBytes;
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

inline SDLPL_WindowDimensions SDLPL_GetWindowDimensions(SDL_Window *window)
{
	SDLPL_WindowDimensions result = {};
	SDL_GetWindowSize(window, &result.width, &result.height);
	return result;
}

#define SDL_URBAN_H
#endif // SDL_URBAN_H
