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

struct SDLPL_AudioBuffer
{
	SDL_AudioDeviceID deviceID;
	s32 samplesPerSecond;
	s32 toneHz;
	s16 toneVolume;
	s32 wavePeriod;
	s32 bytesPerSample;
	f32 tSine;
	s32 latencySampleCount;
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
