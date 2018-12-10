/*
Project: Urban Sprawl
File: sdl_urban_dev.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

/*
This file is used to hold functions for dev features, such as Live Code Editing and Looped Memory Playback
*/

// NOTE(bSalmon): ALL CODE IN THIS FILE IS WINDOWS ONLY CODE, AS SUCH DEVELOPER TOOLS SUCH AS MEMORY PLAYBACK
// AND LIVE CODE EDITING IS NOT CURRENTLY AVAILABLE ON NON-WINDOWS MACHINES

#include "urban.h"

#if URBAN_WIN32
#include <Windows.h>
#include <stdio.h>

internal_func void SDLPL_GetExeFileName(SDLPL_State *state)
{
	DWORD filenameSize = GetModuleFileNameA(0, state->exeName, sizeof(state->exeName));
	state->exeNameBegin = state->exeName;
	for (char *scan = state->exeName; *scan; ++scan)
	{
		if(*scan == '\\')
		{
			state->exeNameBegin = scan + 1;
		}
	}
}

internal_func void SDLPL_BuildExePathFileName(SDLPL_State *state, char *filename, s32 destCount, char *dest)
{
	SDLPL_ConcatenateStrings(state->exeNameBegin - state->exeName, state->exeName,
							 SDLPL_StringLength(filename), filename,
							 destCount, dest);
	
}

inline FILETIME SDLPL_GetLastWriteTime(char *filename)
{
    FILETIME result = {};
	
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
	{
		result = data.ftLastWriteTime;
	}
	
	return result;
}

internal_func SDLPL_GameCode SDLPL_LoadGameCode(char *dllName, char *tempDLLName, char *lockFileName)
{
    SDLPL_GameCode result = {};
	
	WIN32_FILE_ATTRIBUTE_DATA ignored;
	if (!GetFileAttributesEx(lockFileName, GetFileExInfoStandard, &ignored))
    {
        result.dllLastWriteTime = SDLPL_GetLastWriteTime(dllName);
        
        CopyFileA(dllName, tempDLLName, FALSE);
        result.gameCodeDLL = LoadLibrary(tempDLLName);
        if (result.gameCodeDLL)
        {
            result.GetAudioSamples = (game_getAudioSamples *)GetProcAddress(result.gameCodeDLL, "Game_GetAudioSamples");
            result.UpdateRender = (game_updateRender *)GetProcAddress(result.gameCodeDLL, "Game_UpdateRender");
            
            result.isValid = (result.UpdateRender && result.GetAudioSamples);
        }
	}
    
	if (!result.isValid)
	{
		result.GetAudioSamples = 0;
		result.UpdateRender = 0;
	}
	
	return result;
}

internal_func void SDLPL_UnloadGameCode(SDLPL_GameCode *gameCode)
{
    if(gameCode->gameCodeDLL)
    {
        FreeLibrary((HMODULE)gameCode->gameCodeDLL);
    }
    
    gameCode->gameCodeDLL = 0;
    gameCode->isValid = false;
    gameCode->UpdateRender = 0;
    gameCode->GetAudioSamples = 0;
}

internal_func SDLPL_ReplayBuffer* SDLPL_GetReplayBuffer(SDLPL_State *state, u32 index)
{
    ASSERT(index < ARRAY_COUNT(state->replayBuffers));
    SDLPL_ReplayBuffer *result = &state->replayBuffers[index];
    return result;
}

internal_func void SDLPL_GetInputFileLocation(SDLPL_State *state, b32 inputStream, s32 slotIndex, s32 destCount, char *dest)
{
    char temp[64];
    sprintf_s(temp, sizeof(temp), "loop_edit_%d_%s.usi", slotIndex, inputStream ? "input" : "state");
    SDLPL_BuildExePathFileName(state, temp, destCount, dest);
}

internal_func void SDLPL_BeginRecordingInput(SDLPL_State *state, s32 inputRecordingIndex)
{
    SDLPL_ReplayBuffer *replayBuffer = SDLPL_GetReplayBuffer(state, inputRecordingIndex);
    if (replayBuffer->memBlock)
    {
        state->inputRecordingIndex = inputRecordingIndex;
        
        char filename[SDL_STATE_FILE_PATH_MAX];
        SDLPL_GetInputFileLocation(state, true, inputRecordingIndex, sizeof(filename), filename);
        state->recordingHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        
        CopyMemory(replayBuffer->memBlock, state->gameMemBlock, state->totalSize);
    }
}

internal_func void SDLPL_EndRecordingInput(SDLPL_State *state)
{
    CloseHandle(state->recordingHandle);
    state->inputRecordingIndex = 0;
}

internal_func void SDLPL_BeginInputPlayback(SDLPL_State *state, s32 inputPlaybackIndex)
{
    SDLPL_ReplayBuffer *replayBuffer = SDLPL_GetReplayBuffer(state, inputPlaybackIndex);
    if (replayBuffer->memBlock)
    {
        state->inputPlaybackIndex = inputPlaybackIndex;
        
        char filename[SDL_STATE_FILE_PATH_MAX];
        SDLPL_GetInputFileLocation(state, true, inputPlaybackIndex, sizeof(filename), filename);
        state->playbackHandle = CreateFileA(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
        
        CopyMemory(replayBuffer->memBlock, state->gameMemBlock, state->totalSize);
    }
}


internal_func void SDLPL_EndInputPlayback(SDLPL_State *state)
{
    CloseHandle(state->playbackHandle);
    state->inputPlaybackIndex = 0;
}

internal_func void SDLPL_RecordInput(SDLPL_State *state, Game_Input *input)
{
    DWORD bytesWritten = 0;
    WriteFile(state->recordingHandle, input, sizeof(*input), &bytesWritten, 0);
}

internal_func void SDLPL_PlaybackInput(SDLPL_State *state, Game_Input *input)
{
    DWORD bytesRead = 0;
    if (ReadFile(state->playbackHandle, input, sizeof(*input), &bytesRead, 0))
    {
        if (bytesRead == 0)
        {
            s32 playbackIndex = state->inputPlaybackIndex;
            SDLPL_EndInputPlayback(state);
            SDLPL_BeginInputPlayback(state, playbackIndex);
            ReadFile(state->playbackHandle, input, sizeof(*input), &bytesRead, 0);
        }
    }
}

#endif