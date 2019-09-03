/*
Project: Urban Sprawl
File: sdl_urban_dev.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

/*
This file is used to hold functions for dev features, such as Live Code Editing
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
    ConcatenateStrings(state->exeNameBegin - state->exeName, state->exeName,
                       StringLength(filename), filename,
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

#endif