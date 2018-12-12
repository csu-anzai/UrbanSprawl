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

s32 main(s32 argc, char *argv[])
{
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
    
    while (true)
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
            printf("\tAddress: %x:%x\n", inPacket->address.host, inPacket->address.port);
            
            printf("Processing Input...\n");
            
            Game_Input *input = (Game_Input *)inPacket->data;
            u8 dataInput[OUTGOING_PACKET_SIZE];
            
            local_persist s32 xOffset = 0;
            local_persist s32 yOffset = 0;
            local_persist s32 toneHz = 0;
            
            for (s32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); ++controllerIndex)
            {
                Game_Controller *controller = GetGameController(input, controllerIndex);
                if (controller->isAnalog)
                {
                    s32 index = 0;
                    
                    xOffset += (s32)(4.0f * controller->lAverageX);
                    yOffset -= (s32)(4.0f * controller->lAverageY);
                    toneHz = 256 + (s32)(128.0f * (controller->rAverageY));
                    
                    memcpy(&xOffset, &dataInput[index], sizeof(xOffset));
                    index += sizeof(xOffset);
                    
                    memcpy(&yOffset, &dataInput[index], sizeof(yOffset));
                    index += sizeof(yOffset);
                    
                    memcpy(&toneHz, &dataInput[index], sizeof(toneHz));
                    index += sizeof(toneHz);
                }
                else
                {
                    if (controller->dPadLeft.endedDown)
                    {
                        xOffset -= 4;
                        
                        memcpy(&xOffset, &dataInput[0], sizeof(xOffset));
                    }
                    if (controller->dPadRight.endedDown)
                    {
                        xOffset += 4;
                        
                        memcpy(&xOffset, &dataInput[0], sizeof(xOffset));
                    }
                    if (controller->dPadUp.endedDown)
                    {
                        yOffset -= 4;
                        
                        memcpy(&yOffset, &dataInput[4], sizeof(yOffset));
                    }
                    if (controller->dPadDown.endedDown)
                    {
                        yOffset += 4;
                        
                        memcpy(&yOffset, &dataInput[4], sizeof(yOffset));
                    }
                }
                
                if (controller->faceDown.endedDown)
                {
                    toneHz = 5;
                    
                    memcpy(&toneHz, &dataInput[8], sizeof(toneHz));
                }
            }
            printf("Input Processing Done.\n");
            
            // Send Packet back to Client
            UDPpacket *outPacket;
            outPacket = SDLNet_AllocPacket(OUTGOING_PACKET_SIZE);
            
            outPacket->data = dataInput;
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
    }
    
    SDLNet_Quit();
    SDL_Quit();
    
    return 0;
}