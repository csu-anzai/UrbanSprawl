/*
Project: Urban Sprawl
File: urban_bitmap.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include "urban_bitmap.h"
#include "urban_math.h"

internal_func LoadedBitmap Debug_LoadBMP(char *filename, debug_platformReadFile *Debug_PlatformReadFile)
{
    LoadedBitmap result = {};
    
    Debug_ReadFileResult readResult = Debug_PlatformReadFile(filename);
    if (readResult.contentsSize != 0)
    {
        BitmapHeader *header = (BitmapHeader *)readResult.contents;
        u32 *pixels = (u32 *)((u8 *)readResult.contents + header->bitmapOffset);
        result.pixelData = pixels;
        result.width = header->width;
        result.height = header->height;
        
        ASSERT(header->compression == 3);
        
        // NOTE(bSalmon): Bytes order determined by the header
        u32 redMask = header->redMask;
        u32 greenMask = header->greenMask;
        u32 blueMask = header->blueMask;
        u32 alphaMask = ~(redMask | greenMask | blueMask);
        
        BitScanResult redShift = FindLeastSignificantSetBit(redMask);
        BitScanResult greenShift = FindLeastSignificantSetBit(greenMask);
        BitScanResult blueShift = FindLeastSignificantSetBit(blueMask);
        BitScanResult alphaShift = FindLeastSignificantSetBit(alphaMask);
        
        ASSERT(redShift.found);
        ASSERT(greenShift.found);
        ASSERT(blueShift.found);
        ASSERT(alphaShift.found);
        
        u32 *srcDest = pixels;
        for (s32 y = 0; y < header->height; ++y)
        {
            for (s32 x = 0; x < header->width; ++x)
            {
                u32 pixel = *srcDest;
                *srcDest++ = ((((pixel >> alphaShift.index) & 0xFF) << 24) |
                              (((pixel >> redShift.index) & 0xFF) << 16) |
                              (((pixel >> greenShift.index) & 0xFF) << 8) |
                              (((pixel >> blueShift.index) & 0xFF)));
            }
        }
    }
    
    return result;
}

internal_func void DrawBitmap(Game_BackBuffer *backBuffer, LoadedBitmap *bitmap, v2<f32> fPos, s32 alignX = 0, s32 alignY = 0)
{
    fPos.x -= (f32)alignX;
    fPos.y -= (f32)alignY;
    
    s32 minX = RoundF32ToS32(fPos.x);
    s32 minY = RoundF32ToS32(fPos.y);
    s32 maxX = RoundF32ToS32(fPos.x + (f32)bitmap->width);
    s32 maxY = RoundF32ToS32(fPos.y + (f32)bitmap->height);
    
    s32 srcOffsetX = 0;
    if (minX < 0)
    {
        srcOffsetX = -minX;
        minX = 0;
    }
    
    s32 srcOffsetY = 0;
    if (minY < 0)
    {
        srcOffsetY = -minY;
        minY = 0;
    }
    
    if (maxX > backBuffer->width)
    {
        maxX = backBuffer->width;
    }
    
    if (maxY > backBuffer->height)
    {
        maxY = backBuffer->height;
    }
    
    u32 *srcRow = bitmap->pixelData + bitmap->width * (bitmap->height - 1);
    srcRow += srcOffsetY * bitmap->width + srcOffsetX;
    u8 *destRow = (u8 *)backBuffer->memory + (minX * backBuffer->bytesPerPixel) + (minY * backBuffer->pitch);
    for (s32 y = minY; y < maxY; ++y)
    {
        u32 *dest = (u32 *)destRow;
        u32 *src = srcRow;
        for (s32 x = minX; x < maxX; ++x)
        {
            
            f32 a = (f32)((*src >> 24) & 0xFF) / 255.0f;
            f32 srcR = (f32)((*src >> 16) & 0xFF);
            f32 srcG = (f32)((*src >> 8) & 0xFF);
            f32 srcB = (f32)(*src & 0xFF);
            
            f32 destR = (f32)((*dest >> 16) & 0xFF);
            f32 destG = (f32)((*dest >> 8) & 0xFF);
            f32 destB = (f32)(*dest & 0xFF);
            
            f32 r = (1.0f - a) * destR + (a * srcR);
            f32 g = (1.0f - a) * destG + (a * srcG);
            f32 b = (1.0f - a) * destB + (a * srcB);
            
            *dest = (((u32)(r + 0.5f) << 16) |
                     ((u32)(g + 0.5f) << 8) |
                     (u32)(b + 0.5f));
            
            ++dest;
            ++src;
        }
        
        destRow += backBuffer->pitch;
        srcRow -= bitmap->width;
    }
}
