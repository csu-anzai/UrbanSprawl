/*
Project: Urban Sprawl
File: sdl_urban.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "../SDL/SDL.h"
#include "../SDL/SDL_net.h"

#include "../urban.h"
#include <stdio.h>
#include <cstring>

#define OUTGOING_PACKET_SIZE 128

internal_func f32 SDLPL_GetSecondsElapsed(u64 old, u64 curr)
{
    f32 result = 0.0f;
    result = ((f32)(curr - old) / (f32)SDL_GetPerformanceFrequency());
    return result;
}

s32 main(s32 argc, char *argv[])
{
    b32 running = true;
    u16 port = 1842;
    UDPsocket udpSock;
    
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
    
    u64 perfCountFreq = SDL_GetPerformanceFrequency();
    
    udpSock = SDLNet_UDP_Open(port);
    if (!udpSock)
    {
        printf("SDLNet_UDP_Open Failed: %s\n", SDLNet_GetError());
        SDL_Delay(3000);
        return 3;
    }
    else
    {
        printf("UDP Socket Opened at Port: %d\n", port);
    }
    
    // NOTE(bSalmon): 60Hz tick rate
    s32 serverTickRate = 60;
    f32 targetTickRate = 1.0f / (f32)serverTickRate;
    
    u64 lastCounter = SDL_GetPerformanceCounter();
    while (running)
    {
        UDPpacket *inPacket;
        inPacket = SDLNet_AllocPacket(512);
        if (!inPacket)
        {
            printf("Failed to Allocate Packet: %s\n", SDLNet_GetError());
        }
        
        if(SDLNet_UDP_Recv(udpSock, inPacket))
        {
            printf("\nUDP Packet incoming\n");
            printf("\tChannel:  %d\n", inPacket->channel);
            printf("\tLen:  %d\n", inPacket->len);
            printf("\tMaxlen:  %d\n", inPacket->maxlen);
            u8 ip1 = (inPacket->address.host >> 24) & 0xFF;
            u8 ip2 = (inPacket->address.host >> 16) & 0xFF;
            u8 ip3 = (inPacket->address.host >> 8) & 0xFF;
            u8 ip4 = inPacket->address.host & 0xFF;
            u16 ipPort = inPacket->address.port;
            printf("\tAddress: %d.%d.%d.%d:%d\n", ip4, ip3, ip2, ip1, ipPort);
            
            printf("Processing Input...\n");
            
            Game_Input *input = (Game_Input *)inPacket->data;
            
            s32 index = 0;
            s32 xOffset = 0;
            s32 yOffset = 0;
            s32 toneHz = 0;
            
            u8 dataInput[16];
            
            for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
            {
                // TODO(bSalmon): This might mean the keyboard input probably won't work for the moment
                index = 0;
                xOffset = 0;
                yOffset = 0;
                toneHz = 0;
                
                Game_Controller *controller = GetGameController(input, controllerIndex);
                if (controller->isAnalog)
                {
                    xOffset = (s32)(4.0f * controller->lAverageX);
                    yOffset = -(s32)(4.0f * controller->lAverageY);
                    toneHz = 256 + (s32)(128.0f * (controller->rAverageY));
                }
                else
                {
                    if (controller->dPadLeft.endedDown)
                    {
                        xOffset = -4;
                    }
                    if (controller->dPadRight.endedDown)
                    {
                        xOffset = 4;
                    }
                    if (controller->dPadUp.endedDown)
                    {
                        yOffset = -4;
                    }
                    if (controller->dPadDown.endedDown)
                    {
                        yOffset = 4;
                    }
                }
                
                if (controller->faceDown.endedDown)
                {
                    toneHz = 5;
                }
                
                memcpy(&dataInput[index], &xOffset, sizeof(xOffset));
                index += sizeof(xOffset);
                
                memcpy(&dataInput[index], &yOffset, sizeof(yOffset));
                index += sizeof(yOffset);
                
                memcpy(&dataInput[index], &toneHz, sizeof(toneHz));
                index += sizeof(toneHz);
            }
            printf("Input Processing Done.\n");
            
            // Send Packet back to Client
            UDPpacket *outPacket;
            outPacket = SDLNet_AllocPacket(OUTGOING_PACKET_SIZE);
            
            memcpy(outPacket->data, &dataInput, sizeof(dataInput));
            outPacket->address.host = inPacket->address.host;
            outPacket->address.port = inPacket->address.port;
            outPacket->len = 12;
            
            if (SDLNet_UDP_Send(udpSock, -1, outPacket))
            {
                printf("Packet Sent containing: %s\n", (char *)outPacket->data);
            }
            
            SDLNet_FreePacket(outPacket);
        }
        
        SDLNet_FreePacket(inPacket);
        
        if (SDLPL_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter()) < targetTickRate)
        {
            s32 sleepTime = (s32)((targetTickRate - SDLPL_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter())) * 1000) - 1;
            if (sleepTime > 0)
            {
                SDL_Delay(sleepTime);
            }
            else
            {
                printf("MISSED TICK RATE\n\n");
            }
            
            while (SDLPL_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter()) < targetTickRate)
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