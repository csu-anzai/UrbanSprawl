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

// NOTE(bSalmon): Platform TODOs
// TODO(bSalmon): Hardware Acceleration
// TODO(bSalmon): Functioning Fullscreen on Multimonitor setups
// TODO(bSalmon): Remove Debug Platform File Functions when viable

// TODO(bSalmon): Replace VirtualAllocs/Frees with custom mmap, malloc, free
// TODO(bSalmon): SFML Sound? Other Sound libraries?

// IMPORTANT(bSalmon): SIGNIFICANT PORTIONS OF THE PLATFORM LAYER ARE CURRENTLY ONLY AVAILABLE TO WINDOWS 
// MACHINES DUE TO THIS CURRENTLY BEING A PROTOTYPE PLATFORM LAYER TO SUPPORT THE IN DEVELOPMENT GAME,
// RESTRUCTURE IS NEEDED LATER TO MOVE AWAY FROM THIS WIN32 DEPENDENCE

#if URBAN_WIN32
#include <Windows.h>
#endif

#include "SDL/SDL.h"
#include "SDL/SDL_net.h"

#include "sdl_urban.h"
#include "urban.h"
#include "sdl_urban_dev.cpp"

#include <stdio.h>
#include <direct.h>

global_var SDLPL_BackBuffer globalBackBuffer = {};

#if URBAN_INTERNAL && URBAN_WIN32
//////////////////////// DEBUG PLATFORM CODE //////////////////////
// NOTE(bSalmon): THIS IS ALL WINDOWS CODE, ANY CODE USING THIS OFF WINDOWS MACHINES WILL FAIL

DEBUG_PLATFORM_FREE_FILE_MEM(Debug_PlatformFreeFileMem)
{
    if (memory)
    {
        free(memory);
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
            result.contents = malloc(fileSize32);
            if (result.contents)
            {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead))
                {
                    result.contentsSize = fileSize32;
                }
                else
                {
                    free(result.contents);
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

// TODO(bSalmon): Un-Win32 this
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
    // TODO(bSalmon): Should memorySize be stored in the game struct as well?
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

internal_func void SDLPL_ProcessControllerButtons(Game_ButtonState *oldState, Game_ButtonState *newState, SDL_GameController *controller, SDL_GameControllerButton button)
{
    s32 buttonValue = SDL_GameControllerGetButton(controller, button);
    newState->endedDown = buttonValue;
    newState->halfTransitionCount += ((newState->endedDown == oldState->endedDown) ? 0 : 1);
}

internal_func void SDLPL_ProcessControllerStickButtons(Game_ButtonState *oldState, Game_ButtonState *newState, s32 buttonValue)
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
        result = (f32)((axisValue - deadzoneThreshold) / (32767.0f - deadzoneThreshold));
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

// TODO(bSalmon): Fix this for multimonitor setups
internal_func void SDLPL_ToggleFullscreen(SDL_Window *window)
{
    u32 flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
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

internal_func void SDLPL_InitNetworkSocket(UDPsocket *udpSock, IPaddress *address, char *hostName)
{
    u16 port = 1842;
    *udpSock = SDLNet_UDP_Open(0);
    if (!udpSock)
    {
        printf("SDLNet_UDP_Open Failed: %s\n", SDLNet_GetError());
    }
    
    s32 hostFound = SDLNet_ResolveHost(address, hostName, port);
    if (hostFound == -1)
    {
        printf("Failed to Resolve Host: %s\n", SDLNet_GetError());
    }
    else
    {
        
        char *constructedAddress = ConstructIPString(address->host, address->port);
        printf("Resolved Host at IP Address: %s\n", constructedAddress);
        
    }
}

internal_func void SDLPL_HandleEvent(SDL_Event *event, SDL_GameController **controller, Game_Controller *keyboardController, SDLPL_State *state, b32 *running, b32 *pause)
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
        {
            if (!*controller)
            {
                *controller = SDL_GameControllerOpen(event->cdevice.which);
                const char *controllerName = SDL_GameControllerNameForIndex(event->cdevice.which);
                printf("Controller %i with Name '%s' has been connected\n", event->cdevice.which, controllerName);
            }
            break;
        }
        
        case SDL_CONTROLLERDEVICEREMOVED:
        {
            if (*controller)
            {
                SDL_GameControllerClose(*controller);
            }
            
            controller = 0;
            
            break;
        }
        
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_Keycode keycode = event->key.keysym.sym;
            
            b32 altKeyWasDown = (event->key.keysym.mod & KMOD_ALT);
            b32 shiftKeyWasDown = (event->key.keysym.mod & KMOD_SHIFT);
            b32 isDown = (event->key.state == SDL_PRESSED);
            
            if (event->key.repeat == 0)
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
#endif
                if (isDown)
                {
                    if (keycode == SDLK_F4 && altKeyWasDown)
                    {
                        *running = false;
                    }
                    else if (keycode == SDLK_RETURN && altKeyWasDown)
                    {
                        SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                        if (window)
                        {
                            SDLPL_ToggleFullscreen(window);
                        }
                    }
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
    _chdir("../data");
    
    u32 sdlInitFlags = (SDL_INIT_VIDEO |
                        SDL_INIT_GAMECONTROLLER |
                        SDL_INIT_AUDIO |
                        SDL_INIT_TIMER);
    if (SDL_Init(sdlInitFlags) != 0)
    {
        printf("SDL_Init Failed: %s\n", SDL_GetError());
        return 1;
    }
    if (SDLNet_Init() == -1)
    {
        printf("SDLNet_Init Failed: %s\n", SDLNet_GetError());
        return 2;
    }
    
#if URBAN_INTERNAL
    SDL_ShowCursor(SDL_ENABLE);
#else
    SDL_ShowCursor(SDL_DISABLE);
#endif
    
    b32 multiplayer = false;
    if (argc >= 2)
    {
        multiplayer = (strcmp(argv[1], "m") == 0);
    }
    
    // Setup client info here so it can be used later if multiplayer is active
    SDLPL_ClientInfo clientInfo = {};
    
    // NOTE(bSalmon): This is necessary due to some incorrect behaviour with some Controller Axis'
    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
    
    u64 perfCountFreq = SDL_GetPerformanceFrequency();
    
    SDLPL_State state = {};
    SDLPL_GetExeFileName(&state);
    
    char gameCodeDLLPath[SDL_STATE_FILE_PATH_MAX];
    SDLPL_BuildExePathFileName(&state, "urban.dll", sizeof(gameCodeDLLPath), gameCodeDLLPath);
    
    char tempDLLPath[SDL_STATE_FILE_PATH_MAX];
    SDLPL_BuildExePathFileName(&state, "urban_temp.dll", sizeof(tempDLLPath), tempDLLPath);
    
    char lockFilePath[SDL_STATE_FILE_PATH_MAX];
    SDLPL_BuildExePathFileName(&state, "lock.tmp", sizeof(lockFilePath), lockFilePath);
    
    SDL_Window *window = SDL_CreateWindow("Urban Sprawl Pre-Alpha",
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
            
            // Set up Socket
            UDPsocket udpSock = 0;
            IPaddress address = {};
            if (multiplayer)
            {
                SDLPL_InitNetworkSocket(&udpSock, &address, argv[2]);
            }
            
            // NOTE(bSalmon): Only one controller can be used for the game.
            SDL_GameController *sdlController = {};
            s16 controllerDeadzone = 8000;
            
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
            
            if (gameMem.permanentStorage && gameMem.transientStorage)
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
                        SDLPL_HandleEvent(&event, &sdlController, newKeyboard, &state, &running, &pause);
                    }
                    
                    if (!pause)
                    {
                        
                        // NOTE(bSalmon): Used to discern whether the game packet sent to the game layer is the connection response
                        b32 isConnectPacket = false;
                        
                        /* // TODO(bSalmon): Reimplement Mouse Control
                        SDL_Point mouseLoc = {};
                        u32 mouseMask = SDL_GetMouseState(&mouseLoc.x, &mouseLoc.y);
                        newInput->mouseX = mouseLoc.x;
                        newInput->mouseY = mouseLoc.y;
                        
                        u32 SDLPL_ButtonID[3] =
                        {
                            SDL_BUTTON_LMASK,
                            SDL_BUTTON_MMASK,
                            SDL_BUTTON_RMASK
                        };
                        
                        for (u32 buttonIndex = 0; buttonIndex < 3; ++buttonIndex)
                        {
                            newInput->mouseButtons[buttonIndex] = oldInput->mouseButtons[buttonIndex];
                            newInput->mouseButtons[buttonIndex].halfTransitionCount = 0;
                            SDLPL_ProcessKeyPress(&newInput->mouseButtons[buttonIndex], mouseMask & SDLPL_ButtonID[buttonIndex]);
                        }
                        */
                        
                        // Game Controller Handling
                        if (SDL_GameControllerGetAttached(sdlController))
                        {
                            Game_Controller *oldController = GetGameController(oldInput, 1);
                            Game_Controller *newController = GetGameController(newInput, 1);
                            
                            newController->isConnected = true;
                            
                            // TODO(bSalmon): Deadzones currently not restrictive enough
                            newController->lAverageX = SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX), controllerDeadzone);
                            
                            newController->lAverageY = -SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTY), controllerDeadzone);
                            
                            newController->rAverageX = SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTX), controllerDeadzone);
                            
                            newController->rAverageY = -SDLPL_ProcessControllerAxis(SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTY), controllerDeadzone);
                            
                            if (newController->lAverageX != 0.0f ||
                                newController->lAverageY != 0.0f ||
                                newController->rAverageX != 0.0f ||
                                newController->rAverageY != 0.0f)
                            {
                                newController->isAnalog = true;
                            }
                            
                            f32 stickThreshold = 0.5f;
                            SDLPL_ProcessControllerStickButtons(&(oldController->lStickUp), &(newController->lStickUp), newController->lAverageY > stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->lStickDown), &(newController->lStickDown), newController->lAverageY < -stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->lStickLeft), &(newController->lStickLeft), newController->lAverageX < -stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->lStickRight), &(newController->lStickRight), newController->lAverageX > stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->rStickUp), &(newController->rStickUp), newController->rAverageY > stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->rStickDown), &(newController->rStickDown), newController->rAverageY < -stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->rStickLeft), &(newController->rStickLeft), newController->rAverageX < -stickThreshold);
                            
                            SDLPL_ProcessControllerStickButtons(&(oldController->rStickRight), &(newController->rStickRight), newController->rAverageX > stickThreshold);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->dPadUp), &(newController->dPadUp), sdlController, SDL_CONTROLLER_BUTTON_DPAD_UP);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->dPadDown), &(newController->dPadDown), sdlController, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->dPadLeft), &(newController->dPadLeft), sdlController, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->dPadRight), &(newController->dPadRight), sdlController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->faceUp), &(newController->faceUp), sdlController, SDL_CONTROLLER_BUTTON_Y);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->faceDown), &(newController->faceDown), sdlController, SDL_CONTROLLER_BUTTON_A);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->faceLeft), &(newController->faceLeft), sdlController, SDL_CONTROLLER_BUTTON_X);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->faceRight), &(newController->faceRight), sdlController, SDL_CONTROLLER_BUTTON_B);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->lShoulder), &(newController->lShoulder), sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->rShoulder), &(newController->rShoulder), sdlController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->lThumbClick), &(newController->lThumbClick), sdlController, SDL_CONTROLLER_BUTTON_LEFTSTICK);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->rThumbClick), &(newController->rThumbClick), sdlController, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->back), &(newController->back), sdlController, SDL_CONTROLLER_BUTTON_BACK);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->start), &(newController->start), sdlController, SDL_CONTROLLER_BUTTON_START);
                            
                            SDLPL_ProcessControllerButtons(&(oldController->console), &(newController->console), sdlController, SDL_CONTROLLER_BUTTON_GUIDE);
                        }
                        
                        Game_BackBuffer gameBackBuffer = {};
                        gameBackBuffer.memory = globalBackBuffer.memory;
                        gameBackBuffer.width = globalBackBuffer.width;
                        gameBackBuffer.height = globalBackBuffer.height;
                        gameBackBuffer.bytesPerPixel = globalBackBuffer.bytesPerPixel;
                        gameBackBuffer.pitch = globalBackBuffer.pitch;
                        
                        Game_NetworkPacket gamePacket = {};
                        if (multiplayer)
                        {
                            if (!clientInfo.isConnected && !clientInfo.attemptingConnection)
                            {
                                // On connection to the server
                                clientInfo.attemptingConnection = true;
                                
                                // Send 1 byte of trash to signal the server to send us game info
                                UDPpacket *outPacket;
                                outPacket = SDLNet_AllocPacket(1);
                                u8 trashVar = 0xFF;
                                
                                outPacket->data = &trashVar;
                                outPacket->address.host = address.host;
                                outPacket->address.port = address.port;
                                outPacket->len = 1;
                                
                                if (SDLNet_UDP_Send(udpSock, -1, outPacket))
                                {
                                    printf("\nConnecting to Server at %s...\n", argv[2]);
                                }
                                
                                SDLNet_FreePacket(outPacket);
                                
                                // TODO(bSalmon): Start timeout timer here
                                
                                // TODO(bSalmon): Handle if the client fails to connect
                                // Receive packets until the client is connected
                                UDPpacket *connectedPacket;
                                connectedPacket = SDLNet_AllocPacket(INCOMING_CONNECTION_PACKET_SIZE);
                                
                                while(clientInfo.attemptingConnection)
                                {
                                    if(SDLNet_UDP_Recv(udpSock, connectedPacket))
                                    {
                                        printf("\nUDP Packet incoming\n");
                                        printf("\tChannel:  %d\n", connectedPacket->channel);
                                        printf("\tStatus:  %d\n", connectedPacket->status);
                                        
                                        char *constructedAddress = ConstructIPString(connectedPacket->address.host, connectedPacket->address.port);
                                        printf("\tAddress: %s\n", constructedAddress);
                                        
                                        gamePacket.data = (u8 *)malloc(connectedPacket->len);
                                        
                                        CopyMem(gamePacket.data, connectedPacket->data, connectedPacket->len);
                                        
                                        clientInfo.clientID = connectedPacket->data[0];
                                        
                                        clientInfo.isConnected = true;
                                        clientInfo.attemptingConnection = false;
                                        isConnectPacket = true;
                                        
                                        SDLNet_FreePacket(connectedPacket);
                                    }
                                    else
                                    {
                                        // TODO(bSalmon): Update a counter to decide when to time out
                                    }
                                }
                            }
                            else
                            {
                                // Send Packet to Server
                                UDPpacket *outPacket = SDLNet_AllocPacket(sizeof(Game_Input) + sizeof(u8));
                                
                                CopyMem(&outPacket->data[0], &clientInfo.clientID, sizeof(u8));
                                CopyMem(&outPacket->data[1], newInput, sizeof(Game_Input));
                                outPacket->address.host = address.host;
                                outPacket->address.port = address.port;
                                outPacket->len = sizeof(Game_Input) + sizeof(u8);
                                
                                if (SDLNet_UDP_Send(udpSock, -1, outPacket))
                                {
                                    printf("\nPacket Sent containing: %s\n", (char *)outPacket->data);
                                }
                                
                                SDLNet_FreePacket(outPacket);
                                
                                // Receive Packet from Server
                                UDPpacket *inPacket;
                                inPacket = SDLNet_AllocPacket(INCOMING_PACKET_SIZE);
                                if(SDLNet_UDP_Recv(udpSock, inPacket))
                                {
                                    printf("\nUDP Packet incoming\n");
                                    printf("\tChannel:  %d\n", inPacket->channel);
                                    printf("\tStatus:  %d\n", inPacket->status);
                                    
                                    char *constructedAddress = ConstructIPString(inPacket->address.host, inPacket->address.port);
                                    printf("\tAddress: %s\n", constructedAddress);
                                    
                                    gamePacket.data = (u8 *)malloc(inPacket->len);
                                    
                                    CopyMem(gamePacket.data, inPacket->data, inPacket->len);
                                    
                                    SDLNet_FreePacket(inPacket);
                                }
                            }
                        }
                        
                        if (gameCode.UpdateRender)
                        {
                            gameCode.UpdateRender(&gameBackBuffer, newInput, &gameMem, multiplayer, isConnectPacket, &gamePacket, clientInfo.clientID);
                        }
                        free(gamePacket.data);
                        
                        Game_AudioBuffer gameAudioBuffer = {};
                        
                        if (gameCode.GetAudioSamples)
                        {
                            gameCode.GetAudioSamples(&gameMem, &gameAudioBuffer);
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
                        /*
#if URBAN_INTERNAL
                        SDLPL_DebugDrawAudioSyncDisplay(debugTimeMarkers, &audioOutput, ARRAY_COUNT(debugTimeMarkers), targetSecondsPerFrame);
#endif
                        */
                        
                        SDLPL_PresentBuffer(window, renderer, &globalBackBuffer);
                        
                        /*
#if URBAN_INTERNAL
                        SDLPL_DebugTimeMarker *marker = &debugTimeMarkers[debugTimeMarkerIndex++];
                        if (debugTimeMarkerIndex > ARRAY_COUNT(debugTimeMarkers))
                        {
                            debugTimeMarkerIndex = 0;
                        }
                        
                        marker->playCursor = audioRingBuffer.playCursor;
                        marker->writeCursor = audioRingBuffer.writeCursor;
#endif
                        */
                        
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
    
    SDLNet_Quit();
    SDL_Quit();
    return 0;
}