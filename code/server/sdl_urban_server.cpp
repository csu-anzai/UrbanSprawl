/*
Project: Urban Sprawl Server
File: sdl_urban_server.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

/*
CLIENT -> SERVER PACKET STRUCTURE:

On 1st Con:
1 byte of trash

Normal:
   | ClientID | NewInput |
   
SERVER -> CLIENT PACKET STRUCTURE:

On 1st Con:
 | ClientID | World |
 
Normal:
 | players |
 
// TODO(bSalmon): Can the players stuff be packaged better?
 e.g. Only package each players.exists, players.facingDir, players.pos?
 ServerPlayerInfo: exists (1), facingDir (1), pos (20), padding (2)
*/

#if URBAN_WIN32
#include <Windows.h>
#endif

#include "../SDL/SDL.h"
#include "../SDL/SDL_net.h"

#include "../urban.cpp"
#include "../urban_memory.h"

#include <stdio.h>
#include <direct.h>

#define OUTGOING_CONNECTION_PACKET_SIZE KILOBYTES(10)
#define OUTGOING_PACKET_SIZE 512

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
    _chdir("../../data");
    
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
    
    Game_Memory *memory = &gameMem;
    u64 lastCounter = SDL_GetPerformanceCounter();
    while (running)
    {
        ASSERT(sizeof(Game_State) <= memory->permanentStorageSize);
        
        Game_State *gameState = (Game_State *)memory->permanentStorage;
        if (!memory->memInitialised)
        {
            // Initialise as a singleplayer map to send out to the clients
            InitMap_Singleplayer(gameState, memory);
            
            Player basePlayer = {};
            basePlayer.facingDir = FACING_FRONT;
            basePlayer.dims.y = gameState->world->tileMap->tileSideMeters * 0.9f;
            basePlayer.dims.x = 0.5f * basePlayer.dims.y;
            
            for(s32 playerIndex = 0; playerIndex < ARRAY_COUNT(gameState->players); ++playerIndex)
            {
                gameState->players[playerIndex] = basePlayer;
            }
            
            gameState->playerCount = 0;
            
            memory->memInitialised = true;
        }
        
        World *world = gameState->world;
        TileMap *tileMap = world->tileMap;
        
        UDPpacket *inPacket;
        inPacket = SDLNet_AllocPacket(512);
        if (!inPacket)
        {
            printf("Failed to Allocate Packet: %s\n", SDLNet_GetError());
        }
        
        if(SDLNet_UDP_Recv(udpSock, inPacket))
        {
            // TODO(bSalmon): Get player ID and process accordingly
            
            // Prepare the Outbound Packet to be filled
            UDPpacket *outPacket;
            outPacket = SDLNet_AllocPacket(OUTGOING_PACKET_SIZE);
            
            // Process Received Packet
            printf("\nUDP Packet incoming\n");
            printf("\tChannel:  %d\n", inPacket->channel);
            
            char *constructedAddress = ConstructIPString(inPacket->address.host, inPacket->address.port);
            printf("\tAddress: %s\n", constructedAddress);
            
            if (inPacket->len == 1)
            {
                UDPpacket *connectionPacket;
                connectionPacket = SDLNet_AllocPacket(OUTGOING_CONNECTION_PACKET_SIZE);
                
                // Player requesting connection
                if (gameState->playerCount < MAX_CLIENTS)
                {
                    for (s32 playerIndex = 0; playerIndex < ARRAY_COUNT(gameState->players); ++playerIndex)
                    {
                        if (gameState->players[playerIndex].id == 0)
                        {
                            // Connect the client
                            gameState->playerCount++;
                            gameState->players[playerIndex].exists = true;
                            gameState->players[playerIndex].id = gameState->playerCount;
                            
                            break;
                        }
                    }
                }
                
                printf("Connection Request received from %d:%d\n", inPacket->address.host, inPacket->address.port);
                
                printf("Preparing World to be sent back to Client\n");
                
                // Send World Info back to the Client
                connectionPacket->address.host = inPacket->address.host;
                connectionPacket->address.port = inPacket->address.port;
                
                s32 packingIndex = 0;
                DataBlockFill<u8, u8>(connectionPacket->data, &gameState->playerCount, &packingIndex);
                DataBlockFill<u8, v3<u32>>(connectionPacket->data, &tileMap->chunkCount, &packingIndex);
                DataBlockFill<u8, u32>(connectionPacket->data, &tileMap->chunkShift, &packingIndex);
                DataBlockFill<u8, u32>(connectionPacket->data, &tileMap->chunkMask, &packingIndex);
                DataBlockFill<u8, u32>(connectionPacket->data, &tileMap->chunkDim, &packingIndex);
                DataBlockFill<u8, s32>(connectionPacket->data, &tileMap->tileSidePixels, &packingIndex);
                DataBlockFill<u8, f32>(connectionPacket->data, &tileMap->tileSideMeters, &packingIndex);
                DataBlockFill<u8, f32>(connectionPacket->data, &tileMap->metersToPixels, &packingIndex);
                
                // NOTE(bSalmon):  Structure is z:0x, z:0y, z:1x, z:1y...
                s32 coordIndex = packingIndex;
                packingIndex += (sizeof(u32) * tileMap->chunkCount.z) * 2;
                
                b32 zBreak = false;
                b32 xSet = false;
                s32 xIndex = 0;
                for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
                {
                    zBreak = false;
                    xSet = false;
                    xIndex = 0;
                    for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCount.y); ++y)
                    {
                        for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCount.x); ++x)
                        {
                            u8 tileID = GetTileValue(tileMap, v3<u32>{x, y, z});
                            if (x != 0 && tileID == TILE_EMPTY && !xSet)
                            {
                                // NOTE(bSalmon): This marks where a particular row stops, as such every row must be the same length as the first row
                                xIndex = coordIndex;
                                DataBlockFill<u8, u32>(connectionPacket->data, &x, &coordIndex);
                                xSet = true;
                                break;
                            }
                            else if (x == 0 && tileID == TILE_EMPTY)
                            {
                                zBreak = true;
                                
                                // NOTE(bSalmon): This marks where the map stops, as such each row must have something occupying (x:0) to be processed
                                DataBlockFill<u8, u32>(connectionPacket->data, &y, &coordIndex);
                            }
                            if (zBreak || (xSet && x >= connectionPacket->data[xIndex]))
                            {
                                break;
                            }
                            
                            DataBlockFill<u8, u8>(connectionPacket->data, &tileID, &packingIndex);
                        }
                        
                        if (zBreak)
                        {
                            break;
                        }
                    }
                }
                
                connectionPacket->len = packingIndex;
                
                printf("World Info Prepared\n");
                
                if (SDLNet_UDP_Send(udpSock, -1, connectionPacket))
                {
                    printf("World Info sent to Client ID: %d at %s\n", gameState->playerCount, constructedAddress);
                }
                else
                {
                    printf("World Info send to Client ID: %d at %s failed\n", gameState->playerCount, constructedAddress);
                }
                
                SDLNet_FreePacket(connectionPacket);
            }
            else
            {
                u8 packetClientID = inPacket->data[0];
                
                printf("Processing Input...\n");
                
                Game_Input *input = (Game_Input *)&inPacket->data[1];
                
                s32 index = 0;
                v2<f32> ddPlayer = Input(input, gameState, tileMap, packetClientID);
                MovePlayer(input, gameState, tileMap, ddPlayer, packetClientID);
                DataBlockFill<u8, Player[8]>(outPacket->data, &gameState->players, &index);
                
                printf("Input Processing Done.\n");
                
                // Send Packet back to the Client
                outPacket->address.host = inPacket->address.host;
                outPacket->address.port = inPacket->address.port;
                outPacket->len = sizeof(Player[8]);
                
                if (SDLNet_UDP_Send(udpSock, -1, outPacket))
                {
                    printf("Packet sent to Client ID: %d at %d:%d\n", packetClientID, outPacket->address.host, outPacket->address.port);
                }
                
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