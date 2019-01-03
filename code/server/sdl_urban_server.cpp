/*
Project: Urban Sprawl Server
File: sdl_urban_server.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/
#if URBAN_WIN32
#include <Windows.h>
#endif

#include "../SDL/SDL.h"
#include "../SDL/SDL_net.h"

#include "../urban.cpp"
#include <stdio.h>

#define OUTGOING_PACKET_SIZE 128

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

internal_func f32 SDLS_GetSecondsElapsed(u64 old, u64 curr)
{
    f32 result = 0.0f;
    result = ((f32)(curr - old) / (f32)SDL_GetPerformanceFrequency());
    return result;
}

internal_func UDPsocket SDLS_InitSocket()
{
    UDPsocket result = 0;
    u16 port = 1842;
    
    u32 sdlInitFlags = (SDL_INIT_VIDEO |
                        SDL_INIT_GAMECONTROLLER |
                        SDL_INIT_AUDIO |
                        SDL_INIT_TIMER);
    if (SDL_Init(sdlInitFlags) != 0)
    {
        printf("SDL_Init Failed: %s\n", SDL_GetError());
    }
    if (SDLNet_Init() == -1)
    {
        printf("SDLNet_Init Failed: %s\n", SDLNet_GetError());
    }
    
    result = SDLNet_UDP_Open(port);
    if (!result)
    {
        printf("SDLNet_UDP_Open Failed: %s\n", SDLNet_GetError());
        SDL_Delay(3000);
    }
    else
    {
        printf("UDP Socket Opened at Port: %d\n", port);
    }
    
    return result;
}

s32 main(s32 argc, char *argv[])
{
    b32 running = true;
    UDPsocket udpSock = SDLS_InitSocket();
    
    u64 perfCountFreq = SDL_GetPerformanceFrequency();
    
    // NOTE(bSalmon): 60Hz tick rate
    s32 serverTickRate = 60;
    f32 targetTickRate = 1.0f / (f32)serverTickRate;
    
#if URBAN_INTERNAL
    void *baseAddress = (void *)TERABYTES(3);
#else
    void *baseAddress = 0;
#endif
    
    Game_Memory gameMem = {};
    gameMem.permanentStorageSize = MEGABYTES(64);
    gameMem.transientStorageSize = GIGABYTES(2);
    gameMem.Debug_PlatformReadFile = Debug_PlatformReadFile;
    gameMem.Debug_PlatformFreeFileMem = Debug_PlatformFreeFileMem;
    gameMem.Debug_PlatformWriteFile = Debug_PlatformWriteFile;
    
    // TODO(bSalmon): System Metrics
    u64 totalSize = gameMem.permanentStorageSize + gameMem.transientStorageSize;
    gameMem.permanentStorage = VirtualAlloc(baseAddress, (mem_index)totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    gameMem.transientStorage = (u8 *)gameMem.permanentStorage + gameMem.permanentStorageSize;
    
    // Initialise Map
    
    
    Game_Memory *memory = &gameMem;
    u64 lastCounter = SDL_GetPerformanceCounter();
    while (running)
    {
        ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
        
        Game_State *gameState = (Game_State *)memory->permanentStorage;
        if (!memory->memInitialised)
        {
            gameState->playerPos = {};
            
            InitMap(gameState, memory);
            
            memory->memInitialised = true;
        }
        
        World *world = gameState->world;
        TileMap *tileMap = world->tileMap;
        
        f32 playerHeight = tileMap->tileSideMeters * 0.9f;
        f32 playerWidth = 0.5f * playerHeight;
        
        UDPpacket *inPacket;
        inPacket = SDLNet_AllocPacket(512);
        if (!inPacket)
        {
            printf("Failed to Allocate Packet: %s\n", SDLNet_GetError());
        }
        
        if(SDLNet_UDP_Recv(udpSock, inPacket))
        {
            // Prepare the Outbound Packet to be filled
            UDPpacket *outPacket;
            outPacket = SDLNet_AllocPacket(OUTGOING_PACKET_SIZE);
            
            // Process Received Packet
            printf("\nUDP Packet incoming\n");
            printf("\tChannel:  %d\n", inPacket->channel);
            printf("\tLen:  %d\n", inPacket->len);
            printf("\tMaxlen:  %d\n", inPacket->maxlen);
            
            char *constructedAddress = ConstructIPString(inPacket->address.host, inPacket->address.port);
            printf("\tAddress: %s\n", constructedAddress);
            
            printf("Processing Input...\n");
            
            Game_Input *input = (Game_Input *)inPacket->data;
            
            s32 index = 0;
            if (Input(input, gameState, tileMap, playerWidth))
            {
                DataBlockFill<u8, TileMapPosition>(outPacket->data, &gameState->playerPos, &index);
            }
            
            printf("Input Processing Done.\n");
            
            // Send Packet back to the Client
            outPacket->address.host = inPacket->address.host;
            outPacket->address.port = inPacket->address.port;
            outPacket->len = sizeof(TileMapPosition);
            
            if (SDLNet_UDP_Send(udpSock, -1, outPacket))
            {
                printf("Packet Sent containing: %s\n", (u8 *)outPacket->data);
            }
            
            SDLNet_FreePacket(outPacket);
        }
        
        SDLNet_FreePacket(inPacket);
        
        if (SDLS_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter()) < targetTickRate)
        {
            s32 sleepTime = (s32)((targetTickRate - SDLS_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter())) * 1000) - 1;
            if (sleepTime > 0)
            {
                SDL_Delay(sleepTime);
            }
            else
            {
                printf("MISSED TICK RATE\n\n");
            }
            
            while (SDLS_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter()) < targetTickRate)
            {
                // Spin
            }
        }
        
        u64 endCounter = SDL_GetPerformanceCounter();
        lastCounter = endCounter;
    }
    
    SDLNet_Quit();
    SDL_Quit();
    
    return 0;
}