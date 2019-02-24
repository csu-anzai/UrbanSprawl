/*
Project: Urban Sprawl PoC
File: urban_bitmap.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_BITMAP_H

#pragma pack(push, 1)
struct BitmapHeader
{
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmapOffset;
    
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bitsPerPixel;
    u32 compression;
    u32 sizeOfBitmap;
    s32 horzResolution;
    s32 vertResolution;
    u32 coloursUsed;
    u32 coloursImportant;
    
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
};
#pragma pack(pop)

struct LoadedBitmap
{
    s32 width;
    s32 height;
    u32 *pixelData;
};

struct CharacterBitmaps
{
    s32 alignX;
    s32 alignY;
    LoadedBitmap head;
    LoadedBitmap torso;
    LoadedBitmap legs;
    LoadedBitmap feet;
};

#define URBAN_BITMAP_H
#endif // URBAN_BITMAP_H