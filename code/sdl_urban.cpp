/*
Project: Urban Sprawl
File: sdl_urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

/*
This file's purpose is to handle creating the window and setting up different systems, so that
the Game's Update and Render loop is able to be built on top of it
*/

// TODO(bSalmon): Replace VirtualAllocs/Frees with custom mmap, malloc, free
// TODO(bSalmon): Hardware Acceleration
// TODO(bSalmon): Fullscreen
// TODO(bSalmon): Remove Debug Platform File Functions when viable

// NOTE(bSalmon): SIGNIFICANT PORTIONS OF THE PLATFORM LAYER ARE CURRENTLY ONLY AVAILABLE TO WINDOWS MACHINES
// DUE TO THIS CURRENTLY BEING A PROTOTYPE PLATFORM LAYER TO SUPPORT THE IN DEVELOPMENT GAME,
// RESTRUCTURE IS NEEDED LATER TO MOVE AWAY FROM THIS WIN32 DEPENDENCE

#include "SDL/SDL.h"
#include "sdl_urban.h"
#include "urban.h"
#include "sdl_urban_dev.cpp"

#include <stdio.h>

// TODO(bSalmon): Create separate Memory Allocation library so that this doesn't need to be included
// TODO(bSalmon): Debug File Management Functions also need to be removed to remove Windows.h
#if URBAN_WIN32
#include <Windows.h>
#endif

#include <sys/stat.h>

#define MAX_CONTROLLERS 2
global_var SDLPL_BackBuffer globalBackBuffer = {};

#if URBAN_INTERNAL && URBAN_WIN32
//////////////////////// DEBUG PLATFORM CODE //////////////////////
// NOTE(bSalmon): THIS IS ALL WINDOWS CODE, ANY CODE USING THIS OFF WINDOWS MACHINES WILL FAIL

DEBUG_PLATFORM_FREE_FILE_MEM(Debug_PlatformFreeFileMem)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_FILE(Debug_PlatformReadFile)
{
    Debug_ReadFileResult result = {};
    
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            u32 fileSize32 = SafeTruncateU64(fileSize.QuadPart);
            result.contents = VirtualAlloc(0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (result.contents)
            {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead))
                {
                    result.contentsSize = fileSize32;
                }
                else
                {
                    Debug_PlatformFreeFileMem(result.contents);
                    result.contents = 0;
                }
            }
        }
        
        CloseHandle(fileHandle);
    }
    
    return result;
}

DEBUG_PLATFORM_WRITE_FILE(Debug_PlatformWriteFile)
{
    b32 result = {};
    
    HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, memSize, &bytesWritten, 0))
        {
            result = (bytesWritten == memSize);
        }
        
        CloseHandle(fileHandle);
    }
    
    return result;
}
///////////////////////////////////////////////////////////////////
#endif

internal_func void SDLPL_ResizePixelBuffer(SDL_Window *window, SDL_Renderer *renderer, SDLPL_BackBuffer *backBuffer)
{
    if (backBuffer->memory)
    {
        VirtualFree(backBuffer->memory, 0, MEM_RELEASE);
    }
    if (backBuffer->texture)
    {
        SDL_DestroyTexture(backBuffer->texture);
    }
    
    SDLPL_WindowDimensions windowDims = SDLPL_GetWindowDimensions(window);
    backBuffer->width = windowDims.width;
    backBuffer->height = windowDims.height;
    backBuffer->bytesPerPixel = 4;
    backBuffer->memorySizeInBytes = (backBuffer->width * backBuffer->height * backBuffer->bytesPerPixel);
    backBuffer->pitch = backBuffer->width * backBuffer->bytesPerPixel;
    
    backBuffer->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING, backBuffer->width, backBuffer->height);
    
    backBuffer->memory = VirtualAlloc(0, backBuffer->memorySizeInBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal_func void SDLPL_PresentBuffer(SDL_Window *window, SDL_Renderer *renderer, SDLPL_BackBuffer *backBuffer)
{
    SDL_LockTexture(backBuffer->texture, 0, &backBuffer->memory, &backBuffer->pitch);
    SDL_RenderCopy(renderer, backBuffer->texture, 0, 0);
    SDL_RenderPresent(renderer);
    SDL_UnlockTexture(backBuffer->texture);
}

internal_func void SDLPL_OpenControllers(SDL_GameController **controllers)
{
    s32 maxJoysticks = SDL_NumJoysticks();
    s32 controllerIndex = 0;
    
    // Open Controller if it is valid 
    for (s32 joystickIndex = 0; joystickIndex < maxJoysticks; ++joystickIndex)
    {
        if (!SDL_IsGameController(joystickIndex))
        {
            // TODO(bSalmon): Should this return an error in case a player tries to use a weird controller?
            continue;
        }
        if (controllerIndex >= MAX_CONTROLLERS)
        {
            break;
        }
        
        controllers[controllerIndex] = SDL_GameControllerOpen(joystickIndex);
        const char *controllerName = SDL_GameControllerNameForIndex(joystickIndex);
        printf("Joystick %i with Name '%s' has been registered\n", joystickIndex, controllerName);
        ++controllerIndex;
    }
}

internal_func void SDLPL_AudioCallback(void *userData, u8 *audioData, s32 length)
{
    SDLPL_AudioRingBuffer *ringBuffer = (SDLPL_AudioRingBuffer *)userData;
    
    s32 region1Size = length;
    s32 region2Size = 0;
    if ((ringBuffer->playCursor + length) > ringBuffer->size)
    {
        region1Size = ringBuffer->size - ringBuffer->playCursor;
        region2Size = length  - region1Size;
    }
    
    memcpy(audioData, (u8 *)ringBuffer->data + ringBuffer->playCursor, region1Size);
    memcpy(&audioData[region1Size], ringBuffer->data, region2Size);
    ringBuffer->playCursor = (ringBuffer->playCursor + length) % ringBuffer->size;
    ringBuffer->writeCursor = (ringBuffer->playCursor + length) % ringBuffer->size;
}

internal_func void SDLPL_OpenAudio(SDLPL_AudioOutput *audioOutput, SDLPL_AudioRingBuffer *audioRingBuffer)
{
    SDL_AudioSpec desiredSpec = {};
    SDL_AudioSpec audioSpec = {};
    
    desiredSpec.freq = audioOutput->sampleRate;
    desiredSpec.format = AUDIO_S16LSB;
    desiredSpec.channels = 2;
    desiredSpec.samples = 512;
    desiredSpec.callback = &SDLPL_AudioCallback;
    desiredSpec.userdata = audioRingBuffer;
    
    audioRingBuffer->size = audioOutput->secondaryBufferSize;
    audioRingBuffer->data = calloc(audioOutput->secondaryBufferSize, 1);
    
    audioOutput->deviceID = SDL_OpenAudioDevice(0, 0, &desiredSpec, &audioSpec, 0);
    
    if (audioOutput->deviceID)
    {
        if (audioSpec.format != AUDIO_S16LSB)
        {
            printf("Failed to retrieve AUDIO_S16LSB as the audio format");
            SDL_CloseAudioDevice(audioOutput->deviceID);
        }
        else
        {
            printf("Audio Device Opened at %iHz & %i channels\n", audioSpec.freq, audioSpec.channels);
        }
    }
    else
    {
        printf("Failed to Open Audio Device: %s\n", SDL_GetError());
    }
}

internal_func void SDLPL_FillAudioBuffer(SDLPL_AudioOutput *audioOutput, SDLPL_AudioRingBuffer *audioRingBuffer, Game_AudioBuffer *gameAudioBuffer, s32 lockByte, s32 writeBytes)
{
    s16 *samples = gameAudioBuffer->samples;
    
    void *region1 = (u8 *)audioRingBuffer->data + lockByte;
    s32 region1Size = writeBytes;
    if ((region1Size + lockByte) > audioOutput->secondaryBufferSize)
    {
        region1Size = audioOutput->secondaryBufferSize - lockByte;
    }
    
    void *region2 = audioRingBuffer->data;
    s32 region2Size = writeBytes - region1Size;
    s32 region1SampleCount = region1Size / audioOutput->bytesPerSample;
    s16 *sampleOut = (s16 *)region1;
    for (s32 sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
    {
        *sampleOut++ = *samples++;
        *sampleOut++ = *samples++;
        ++audioOutput->runningSampleIndex;
    }
    
    s32 region2SampleCount = region2Size / audioOutput->bytesPerSample;
    sampleOut = (s16 *)region2;
    for (s32 sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
    {
        *sampleOut++ = *samples++;
        *sampleOut++ = *samples++;
        ++audioOutput->runningSampleIndex;
    }
}

#if URBAN_INTERNAL
internal_func void SDLPL_DebugDrawVerticalLine(s32 x, s32 top, s32 bottom, u32 colour)
{
    u8 *pixel = ((u8 *)globalBackBuffer.memory + x * globalBackBuffer.bytesPerPixel + top *globalBackBuffer.pitch);
    
    for (s32 y = top; y < bottom; ++y)
    {
        *(u32 *)pixel = colour;
        pixel += globalBackBuffer.pitch;
    }
}

inline void SDLPL_DebugDrawAudioBufferMarker(SDLPL_AudioOutput *audioOutput, f32 c, s32 padX, s32 top, s32 bottom, s32 value, u32 colour)
{
    ASSERT(value < audioOutput->secondaryBufferSize);
    f32 xFloat = (c * (f32)value);
    s32 x = padX + (s32)xFloat;
    SDLPL_DebugDrawVerticalLine(x, top, bottom, colour);
}

internal_func void SDLPL_DebugDrawAudioSyncDisplay(SDLPL_DebugTimeMarker *markers, SDLPL_AudioOutput *audioOutput, s32 markerCount, f32 targetSecondsPerFrame)
{
    s32 padX = 16;
    s32 padY = 16;
    
    s32 top = padY;
    s32 bottom = globalBackBuffer.height - padY;
    
    f32 c = (f32)(globalBackBuffer.width - (2 * padX)) / (f32)audioOutput->secondaryBufferSize;
    for (s32 markerIndex = 0; markerIndex < markerCount; ++markerIndex)
    {
        SDLPL_DebugTimeMarker *currMarker = &markers[markerIndex];
        SDLPL_DebugDrawAudioBufferMarker(audioOutput, c, padX, top, bottom, currMarker->playCursor, 0xFFFF0000);
        SDLPL_DebugDrawAudioBufferMarker(audioOutput, c, padX, top, bottom, currMarker->writeCursor, 0xFFFFFFFF);
    }
}
#endif

internal_func void SDLPL_ProcessControllerButtons(Game_ButtonState *oldState, Game_ButtonState *newState, b32 buttonValue)
{
    newState->endedDown = buttonValue;
    newState->halfTransitionCount += ((newState->endedDown == oldState->endedDown) ? 0 : 1);
}

internal_func f32 SDLPL_ProcessControllerAxis(s16 axisValue, s16 deadzoneThreshold)
{
    f32 result = 0;
    
    if (axisValue < -deadzoneThreshold)
    {
        result = (f32)((axisValue + deadzoneThreshold) / (32768.0f - deadzoneThreshold));
    }
    else if (axisValue > deadzoneThreshold)
    {
        result = (f32)((axisValue + deadzoneThreshold) / (32767.0f - deadzoneThreshold));
    }
    
    return result;
}

internal_func void SDLPL_ProcessKeyPress(Game_ButtonState *newState, b32 isDown)
{
    if (newState->endedDown != isDown)
    {
        newState->endedDown = isDown;
        ++newState->halfTransitionCount;
    }
}

internal_func f32 SDLPL_GetSecondsElapsed(u64 old, u64 curr)
{
    f32 result = 0.0f;
    result = ((f32)(curr - old) / (f32)SDL_GetPerformanceFrequency());
    return result;
}

internal_func void SDLPL_HandleWindowEvent(SDL_Event *event)
{
    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
    SDL_Renderer *renderer = SDL_GetRenderer(window);
    
    switch (event->window.event)
    {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            SDLPL_ResizePixelBuffer(window, renderer, &globalBackBuffer);
            break;
        }
        
        case SDL_WINDOWEVENT_EXPOSED:
        {
            SDLPL_PresentBuffer(window, renderer, &globalBackBuffer);
            break;
        }
    }
}

internal_func void SDLPL_HandleEvent(SDL_Event *event, SDL_GameController **controllers, Game_Controller *keyboardController, SDLPL_State *state, b32 *running, b32 *pause)
{
    switch (event->type)
    {
        case SDL_QUIT:
        {
            *running = false;
            break;
        }
        
        case SDL_WINDOWEVENT:
        {
            SDLPL_HandleWindowEvent(event);
            break;
        }
        
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEREMAPPED:
        {
            SDLPL_OpenControllers(controllers);
            break;
        }
        
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_Keycode keycode = event->key.keysym.sym;
            b32 isDown = (event->key.state == SDL_PRESSED);
            b32 wasDown = false;
            if (event->key.state == SDL_RELEASED)
            {
                wasDown = true;
            }
            else if (event->key.repeat != 0)
            {
                wasDown = true;
            }
            
            if (event->key.repeat == 0)
            {
                if (wasDown != isDown)
                {
                    if (keycode == SDLK_w)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->lStickUp, isDown);
                    }
                    if (keycode == SDLK_s)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->lStickDown, isDown);
                    }
                    if (keycode == SDLK_a)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->lStickLeft, isDown);
                    }
                    if (keycode == SDLK_d)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->lStickRight, isDown);
                    }
                    if (keycode == SDLK_UP)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->dPadUp, isDown);
                    }
                    if (keycode == SDLK_DOWN)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->dPadDown, isDown);
                    }
                    if (keycode == SDLK_LEFT)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->dPadLeft, isDown);
                    }
                    if (keycode == SDLK_RIGHT)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->dPadRight, isDown);
                    }
                    if (keycode == SDLK_SPACE)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->faceDown, isDown);
                    }
                    if (keycode == SDLK_LSHIFT)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->faceRight, isDown);
                    }
                    if (keycode == SDLK_f)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->faceLeft, isDown);
                    }
                    if (keycode == SDLK_TAB)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->faceUp, isDown);
                    }
                    if (keycode == SDLK_q)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->lShoulder, isDown);
                    }
                    if (keycode == SDLK_e)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->rShoulder, isDown);
                    }
                    if (keycode == SDLK_BACKSPACE)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->back, isDown);
                    }
                    if (keycode == SDLK_ESCAPE)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->start, isDown);
                    }
                    if (keycode == SDLK_RETURN)
                    {
                        SDLPL_ProcessKeyPress(&keyboardController->console, isDown);
                    }
#if URBAN_INTERNAL
                    if (keycode == SDLK_p)
                    {
                        if (isDown)
                        {
                            *pause = !*pause;
                        }
                    }
#if URBAN_WIN32
                    if (keycode == SDLK_l)
                    {
                        if (isDown)
                        {
                            if (state->inputPlaybackIndex == 0)
                            {
                                if (state->inputRecordingIndex == 0)
                                {
                                    SDLPL_BeginRecordingInput(state, 1);
                                }
                                else
                                {
                                    SDLPL_EndRecordingInput(state);
                                    SDLPL_BeginInputPlayback(state, 1);
                                }
                            }
                            else
                            {
                                SDLPL_EndInputPlayback(state);
                            }
                        }
                    }
#endif
#endif
                }
            }
            
            break;
        }
        
        default:
        {
            break;
        }
    }
}

s32 main(s32 argc, char *argv[])
{
    u32 sdlInitFlags = (SDL_INIT_VIDEO |
                        SDL_INIT_GAMECONTROLLER |
                        SDL_INIT_AUDIO |
                        SDL_INIT_TIMER);
    if (SDL_Init(sdlInitFlags) != 0)
    {
        printf("SDL_Init Failed: %s\n", SDL_GetError());
    }
    
    // NOTE(bSalmon): This is necessary due to some incorrect behaviour with some Controller Axis'
    SDL_GameControllerAddMappingsFromFile("../gamecontrollerdb.txt");
    
    u64 perfCountFreq = SDL_GetPerformanceFrequency();
    
    SDLPL_State state = {};
    SDLPL_GetExeFileName(&state);
    
    char gameCodeDLLPath[SDL_STATE_FILE_PATH_MAX];
    SDLPL_BuildExePathFileName(&state, "urban.dll", sizeof(gameCodeDLLPath), gameCodeDLLPath);
    
    char tempDLLPath[SDL_STATE_FILE_PATH_MAX];
    SDLPL_BuildExePathFileName(&state, "urban_temp.dll", sizeof(tempDLLPath), tempDLLPath);
    
    char lockFilePath[SDL_STATE_FILE_PATH_MAX];
    SDLPL_BuildExePathFileName(&state, "lock.tmp", sizeof(lockFilePath), lockFilePath);
    
    SDL_Window *window = SDL_CreateWindow("Urban Sprawl PoC",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1280, 720, SDL_WINDOW_RESIZABLE);
    
    if (window)
    {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
        
        if (renderer)
        {
            SDL_DisplayMode displayMode;
            s32 displayIndex = SDL_GetWindowDisplayIndex(window);
            
            s32 monitorRefreshHz = 0;
            if ((SDL_GetDesktopDisplayMode(displayIndex, &displayMode) != 0) || (displayMode.refresh_rate == 0))
            {
                // NOTE(bSalmon): 60Hz is the default Hz
                monitorRefreshHz = 60;
            }
            else
            {
                monitorRefreshHz = displayMode.refresh_rate;
            }
            
            f32 targetSecondsPerFrame = 1.0f / (f32)monitorRefreshHz;
            
            b32 running = true;
            b32 pause = false;
            
            SDLPL_ResizePixelBuffer(window, renderer, &globalBackBuffer);
            
            // NOTE(bSalmon): SDL_CONTROLLERDEVICEADDED is called at the start of the program so it is not required to call SDLPL_OpenControllers before the main loop
            SDL_GameController *sdlControllers[MAX_CONTROLLERS] = {0, 0};
            
            SDLPL_AudioRingBuffer audioRingBuffer = {};
            SDLPL_AudioOutput audioOutput = {};
            audioOutput.sampleRate = 48000;
            audioOutput.runningSampleIndex = 0;
            audioOutput.bytesPerSample = sizeof(s16) * 2;
            audioOutput.secondaryBufferSize = audioOutput.sampleRate * audioOutput.bytesPerSample;
            audioOutput.safetyBytes = (s32)(((f32)audioOutput.sampleRate * (f32)audioOutput.bytesPerSample / monitorRefreshHz) / 2.0f);
            
            // Open the audio device and retrieve the Device ID for later use
            SDLPL_OpenAudio(&audioOutput, &audioRingBuffer);
            
            SDL_PauseAudioDevice(audioOutput.deviceID, 0);
            
            s16 *samples = (s16 *)calloc(audioOutput.sampleRate, audioOutput.bytesPerSample);
            
#if URBAN_INTERNAL
            void *baseAddress = (void *)TERABYTES(2);
#else
            void *baseAddress = 0;
#endif
            
            Game_Memory gameMem = {};
            gameMem.permanentStorageSize = MEGABYTES(64);
            gameMem.transientStorageSize = GIGABYTES(4);
            gameMem.Debug_PlatformReadFile = Debug_PlatformReadFile;
            gameMem.Debug_PlatformFreeFileMem = Debug_PlatformFreeFileMem;
            gameMem.Debug_PlatformWriteFile = Debug_PlatformWriteFile;
            
            // TODO(bSalmon): System Metrics
            state.totalSize = gameMem.permanentStorageSize + gameMem.transientStorageSize;
            state.gameMemBlock = VirtualAlloc(baseAddress, (mem_index)state.totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            gameMem.permanentStorage = state.gameMemBlock;
            gameMem.transientStorage = (u8 *)gameMem.permanentStorage + gameMem.permanentStorageSize;
            
#if URBAN_WIN32
            for (s32 replayIndex = 0; replayIndex < ARRAY_COUNT(state.replayBuffers); ++replayIndex)
            {
                SDLPL_ReplayBuffer *currBuffer = &state.replayBuffers[replayIndex];
                
                char filepath[SDL_STATE_FILE_PATH_MAX];
                SDLPL_GetInputFileLocation(&state, false, replayIndex, sizeof(filepath), filepath);
                currBuffer->fileHandle = CreateFileA(currBuffer->replayFilename, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);
                
                currBuffer->memoryMap = CreateFileMapping(currBuffer->fileHandle, 0, PAGE_READWRITE, (state.totalSize >> 32), (state.totalSize & 0xFFFFFFFF), 0);
                
                currBuffer->memBlock = MapViewOfFile(currBuffer->memoryMap, FILE_MAP_ALL_ACCESS, 0, 0, state.totalSize);
            }
#endif
            
            if (samples && gameMem.permanentStorage && gameMem.transientStorage)
            {
                Game_Input input[2] = {};
                Game_Input *newInput = &input[0];
                Game_Input *oldInput = &input[1];
                
#if URBAN_INTERNAL
                // NOTE(bSalmon): 30 DebugTimeMarkers because it is half of my monitor's refresh rate
                s32 debugTimeMarkerIndex = 0;
                SDLPL_DebugTimeMarker debugTimeMarkers[30] = {};
#endif
                
                b32 soundIsValid = false;
                
                SDLPL_GameCode gameCode = SDLPL_LoadGameCode(gameCodeDLLPath, tempDLLPath, lockFilePath);
                
                u64 flipWallClock = SDL_GetPerformanceCounter();
                u64 lastCounter = SDL_GetPerformanceCounter();
                u64 lastCycleCount = __rdtsc();
                while (running)
                {
                    newInput->deltaTime = targetSecondsPerFrame;
                    
#if URBAN_INTERNAL && URBAN_WIN32
                    FILETIME newDLLWriteTime = SDLPL_GetLastWriteTime(gameCodeDLLPath);
                    if (CompareFileTime(&newDLLWriteTime, &gameCode.dllLastWriteTime) != 0)
                    {
                        SDL_Delay(33);
                        SDLPL_UnloadGameCode(&gameCode);
                        gameCode = SDLPL_LoadGameCode(gameCodeDLLPath, tempDLLPath, lockFilePath);
                    }
#endif
                    
                    Game_Controller *oldKeyboard = GetGameController(oldInput, 0);
                    Game_Controller *newKeyboard = GetGameController(newInput, 0);
                    *newKeyboard = {};
                    newKeyboard->isConnected = true;
                    
                    for (s32 buttonIndex = 0; buttonIndex < ARRAY_COUNT(newKeyboard->buttons); ++buttonIndex)
                    {
                        newKeyboard->buttons[buttonIndex].endedDown = oldKeyboard->buttons[buttonIndex].endedDown;
                    }
                    
                    SDL_Event event;
                    while (SDL_PollEvent(&event))
                    {
                        SDLPL_HandleEvent(&event, sdlControllers, newKeyboard, &state, &running, &pause);
                    }
                    
                    if (!pause)
                    {
                        // NOTE(bSalmon): Controller Index set to 1 to avoid overwrite of Keyboard
                        for (s32 controllerIndex = 1; controllerIndex < MAX_CONTROLLERS; ++controllerIndex)
                        {
                            if (SDL_GameControllerGetAttached(sdlControllers[controllerIndex]))
                            {
                                Game_Controller *oldController = GetGameController(oldInput, controllerIndex);
                                Game_Controller *newController = GetGameController(newInput, controllerIndex);
                                
                                newController->isConnected = true;
                                
                                
                                // NOTE(bSalmon): Make sure the deadzones feel alright
                                newController->lAverageX = SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlControllers[controllerIndex], SDL_CONTROLLER_AXIS_LEFTX), 1);
                                
                                newController->lAverageY = -SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlControllers[controllerIndex], SDL_CONTROLLER_AXIS_LEFTY), 1);
                                
                                newController->rAverageX = SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlControllers[controllerIndex], SDL_CONTROLLER_AXIS_RIGHTX), 1);
                                
                                newController->rAverageY = -SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlControllers[controllerIndex], SDL_CONTROLLER_AXIS_RIGHTY), 1);
                                
                                if (newController->lAverageX != 0.0f ||
                                    newController->lAverageY != 0.0f ||
                                    newController->rAverageX != 0.0f ||
                                    newController->rAverageY != 0.0f)
                                {
                                    newController->isAnalog = true;
                                }
                                
                                f32 stickThreshold = 0.5f;
                                SDLPL_ProcessControllerButtons(&(oldController->lStickUp), &(newController->lStickUp), newController->lAverageY > stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->lStickDown), &(newController->lStickDown), newController->lAverageY < -stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->lStickLeft), &(newController->lStickLeft), newController->lAverageX < -stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->lStickRight), &(newController->lStickRight), newController->lAverageX > stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->rStickUp), &(newController->rStickUp), newController->rAverageY > stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->rStickDown), &(newController->rStickDown), newController->rAverageY < -stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->rStickLeft), &(newController->rStickLeft), newController->rAverageX < -stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->rStickRight), &(newController->rStickRight), newController->rAverageX > stickThreshold);
                                
                                SDLPL_ProcessControllerButtons(&(oldController->dPadUp), &(newController->dPadUp), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_DPAD_UP));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->dPadDown), &(newController->dPadDown), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_DPAD_DOWN));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->dPadLeft), &(newController->dPadLeft), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_DPAD_LEFT));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->dPadRight), &(newController->dPadRight), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->faceUp), &(newController->faceUp), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_Y));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->faceDown), &(newController->faceDown), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_A));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->faceLeft), &(newController->faceLeft), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_X));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->faceRight), &(newController->faceRight), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_B));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->lShoulder), &(newController->lShoulder), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->rShoulder), &(newController->rShoulder), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->lThumbClick), &(newController->lThumbClick), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_LEFTSTICK));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->rThumbClick), &(newController->rThumbClick), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSTICK));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->back), &(newController->back), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_BACK));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->start), &(newController->start), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_START));
                                
                                SDLPL_ProcessControllerButtons(&(oldController->console), &(newController->console), SDL_GameControllerGetButton(sdlControllers[controllerIndex], SDL_CONTROLLER_BUTTON_GUIDE));
                            }
                        }
                        
                        Game_BackBuffer gameBackBuffer = {};
                        gameBackBuffer.memory = globalBackBuffer.memory;
                        gameBackBuffer.width = globalBackBuffer.width;
                        gameBackBuffer.height = globalBackBuffer.height;
                        gameBackBuffer.bytesPerPixel = globalBackBuffer.bytesPerPixel;
                        gameBackBuffer.pitch = globalBackBuffer.pitch;
                        
#if URBAN_WIN32
                        if (state.inputRecordingIndex)
                        {
                            SDLPL_RecordInput(&state, newInput);
                        }
                        
                        if (state.inputPlaybackIndex)
                        {
                            SDLPL_PlaybackInput(&state, newInput);
                        }
#endif
                        
                        if (gameCode.UpdateRender)
                        {
                            gameCode.UpdateRender(&gameBackBuffer, newInput, &gameMem);
                        }
                        
                        u64 audioWallClock = SDL_GetPerformanceCounter();
                        f32 frameBeginToAudioSeconds = SDLPL_GetSecondsElapsed(flipWallClock, audioWallClock);
                        
                        // NOTE(bSalmon): Sound Output Test
                        SDL_LockAudio();
                        
                        if (!soundIsValid)
                        {
                            audioOutput.runningSampleIndex = audioRingBuffer.writeCursor / audioOutput.bytesPerSample;
                            soundIsValid = true;
                        }
                        
                        s32 lockByte = (audioOutput.runningSampleIndex * audioOutput.bytesPerSample) % audioOutput.secondaryBufferSize;
                        
                        s32 expectedAudioBytesPerFrame = (s32)(((f32)audioOutput.sampleRate * (f32)audioOutput.bytesPerSample) / monitorRefreshHz);
                        f32 secondsLeftUntilFlip = targetSecondsPerFrame - frameBeginToAudioSeconds;
                        s32 expectedBytesUntilFlip = (s32)((secondsLeftUntilFlip / targetSecondsPerFrame) / (f32)expectedAudioBytesPerFrame);
                        
                        s32 expectedFrameBoundary = audioRingBuffer.playCursor + expectedBytesUntilFlip;
                        
                        s32 safeWriteCursor = audioRingBuffer.writeCursor;
                        if (safeWriteCursor < audioRingBuffer.playCursor)
                        {
                            safeWriteCursor += audioOutput.secondaryBufferSize;
                        }
                        ASSERT(safeWriteCursor >= audioRingBuffer.playCursor);
                        safeWriteCursor += audioOutput.safetyBytes;
                        
                        b32 audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundary;
                        
                        s32 targetCursor = 0;
                        if (audioCardIsLowLatency)
                        {
                            targetCursor = expectedFrameBoundary + expectedAudioBytesPerFrame;
                        }
                        else
                        {
                            targetCursor = audioRingBuffer.writeCursor + expectedAudioBytesPerFrame + audioOutput.safetyBytes;
                        }
                        targetCursor = targetCursor % audioOutput.secondaryBufferSize;
                        
                        s32 writeBytes = 0;
                        if (lockByte > targetCursor)
                        {
                            writeBytes = audioOutput.secondaryBufferSize - lockByte;
                            writeBytes += targetCursor;
                        }
                        else
                        {
                            writeBytes = targetCursor - lockByte;
                        }
                        
                        SDL_UnlockAudio();
                        
                        Game_AudioBuffer gameAudioBuffer = {};
                        gameAudioBuffer.sampleRate = audioOutput.sampleRate;
                        gameAudioBuffer.sampleCount = writeBytes / audioOutput.bytesPerSample;
                        gameAudioBuffer.samples = samples;
                        
                        if (gameCode.GetAudioSamples)
                        {
                            gameCode.GetAudioSamples(&gameMem, &gameAudioBuffer);
                        }
                        
                        if (writeBytes > 0)
                        {
                            SDLPL_FillAudioBuffer(&audioOutput, &audioRingBuffer, &gameAudioBuffer, lockByte, writeBytes);
                        }
                        
                        if (SDLPL_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter()) < targetSecondsPerFrame)
                        {
                            s32 sleepTime = (s32)((targetSecondsPerFrame - SDLPL_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter())) * 1000) - 1;
                            if (sleepTime > 0)
                            {
                                SDL_Delay(sleepTime);
                            }
                            
                            while (SDLPL_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter()) < targetSecondsPerFrame)
                            {
                                // Spin
                            }
                        }
                        
                        u64 endCounter = SDL_GetPerformanceCounter();
                        
#if URBAN_INTERNAL
                        SDLPL_DebugDrawAudioSyncDisplay(debugTimeMarkers, &audioOutput, ARRAY_COUNT(debugTimeMarkers), targetSecondsPerFrame);
#endif
                        
                        SDLPL_PresentBuffer(window, renderer, &globalBackBuffer);
                        
#if URBAN_INTERNAL
                        SDLPL_DebugTimeMarker *marker = &debugTimeMarkers[debugTimeMarkerIndex++];
                        if (debugTimeMarkerIndex > ARRAY_COUNT(debugTimeMarkers))
                        {
                            debugTimeMarkerIndex = 0;
                        }
                        
                        marker->playCursor = audioRingBuffer.playCursor;
                        marker->writeCursor = audioRingBuffer.writeCursor;
#endif
                        
                        u64 endCycleCount = __rdtsc();
                        u64 counterElapsed = endCounter - lastCounter;
                        u64 cyclesElapsed = endCycleCount - lastCycleCount;
                        
                        f64 msPerFrame = (1000.0f * (f64)counterElapsed) / (f64)perfCountFreq;
                        f64 fps = (f64)perfCountFreq / (f64)counterElapsed;
                        f64 mcPerFrame = (f64)cyclesElapsed / (1000.0f * 1000.0f);
                        
                        printf("Frame Stats: %.02fms/f, %.02ff/s, %.02fmc/f\n", msPerFrame, fps, mcPerFrame);
                        
                        lastCycleCount = endCycleCount;
                        lastCounter = endCounter;
                        
                        SWAP(newInput, oldInput);
                    }
                }
            }
        }
    }
    
    SDL_Quit();
    return 0;
}