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

#define SOCKET_BUFFER_SIZE 1024

s32 main(s32 argc, char *argv[])
{
    u16 port = 1842;
    UDPsocket udpSock;
    UDPpacket *packet = {};
    
    
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
    
    packet = SDLNet_AllocPacket(512);
    if (!packet)
    {
        printf("Failed to Allocate Packet: %s\n", SDLNet_GetError());
    }
    
    while (true)
    {
        if(SDLNet_UDP_Recv(udpSock, packet))
        {
            printf("\nUDP Packet incoming\n");
            printf("\tChannel:  %d\n", packet->channel);
            printf("\tData:  %s\n", (char *)packet->data);
            printf("\tLen:  %d\n", packet->len);
            printf("\tMaxlen:  %d\n", packet->maxlen);
            printf("\tStatus:  %d\n", packet->status); printf("\tAddress: %x:%x\n", packet->address.host, packet->address.port);
        }
    }
    
    SDLNet_FreePacket(packet);
    SDLNet_Quit();
    SDL_Quit();
    
    return 0;
}