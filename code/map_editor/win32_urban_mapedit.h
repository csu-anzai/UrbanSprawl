/*
Project: Urban Sprawl Map Editor
File: win32_urban_mapedit.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef WIN32_URBAN_MAPEDIT_H

struct Win32_Menus
{
    HMENU file;
    HMENU tools;
};

struct Win32_BackBuffer
{
    // NOTE(bSalmon): 32-bit wide, Mem Order BB GG RR xx
	BITMAPINFO info;
	void *memory;
	s32 width;
	s32 height;
	s32 pitch;
	s32 bytesPerPixel;
};

struct Win32_MapInfo
{
    s32 segmentDimTiles;
    u32 segmentDimChunks;
    f32 tileSide;
    
    u32 gridLineCount;
    u32 gridDimPixels;
    
    s32 padding;
    s32 gridLeft;
    s32 gridRight;
    s32 gridTop;
    s32 gridBottom;
    
    v2<u32> currSegment;
    u32 currZ;
};

struct Win32_SelectorInfo
{
    f32 tileSide;
    
    u32 gridLineCount;
    u32 gridDimPixels;
    
    s32 padding;
    s32 gridLeft;
    s32 gridRight;
    s32 gridTop;
    s32 gridBottom;
    
    POINT *currSelection;
};

struct Win32_Input
{
    b32 priClicked;
    b32 secClicked;
    POINT mouseLoc;
    u8 priCursor;
    u8 secCursor;
    u8 totalCursors;
    b32 rectCursor;
    b32 topLeftSet;
};

struct Win32_WindowDimensions
{
	s32 width;
	s32 height;
};

inline Win32_WindowDimensions Win32_GetWindowDimensions(HWND window)
{
	Win32_WindowDimensions result;
	
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
	
	return result;
}

inline void Win32_DrawVerticalLine(Win32_BackBuffer *backBuffer, s32 x, s32 top, s32 bottom, u32 colour)
{
    u8 *pixel = ((u8 *)backBuffer->memory + x * backBuffer->bytesPerPixel + top * backBuffer->pitch);
    
    for (s32 y = top; y < bottom; ++y)
    {
        *(u32 *)pixel = colour;
        pixel += backBuffer->pitch;
    }
}

inline void Win32_DrawHorizontalLine(Win32_BackBuffer *backBuffer, s32 y, s32 left, s32 right, u32 colour)
{
    u8 *pixel = ((u8 *)backBuffer->memory + left * backBuffer->bytesPerPixel + y * backBuffer->pitch);
    
    for (s32 x = left; x < right; ++x)
    {
        *(u32 *)pixel = colour;
        pixel += backBuffer->bytesPerPixel;
    }
}

inline void DrawRect(Win32_BackBuffer *backBuffer, v2<f32>min, v2<f32>max, u32 colour)
{
    u8 *endOfBuffer = (u8 *)backBuffer->memory + (backBuffer->pitch * backBuffer->height);
    
    v2<s32> roundedMin = {};
    roundedMin.x = RoundF32ToS32(min.x);
    roundedMin.y = RoundF32ToS32(min.y);
    
    v2<s32> roundedMax = {};
    roundedMax.x = RoundF32ToS32(max.x);
    roundedMax.y = RoundF32ToS32(max.y);
    
    if (roundedMin.x < 0)
    {
        roundedMin.x = 0;
    }
    
    if (roundedMin.y < 0)
    {
        roundedMin.y = 0;
    }
    
    if (roundedMax.x > backBuffer->width)
    {
        roundedMax.x = backBuffer->width;
    }
    
    if (roundedMax.y > backBuffer->height)
    {
        roundedMax.y = backBuffer->height;
    }
    
    for (s32 x = roundedMin.x; x < roundedMax.x; ++x)
    {
        u8 *pixel = (u8 *)backBuffer->memory + (x * backBuffer->bytesPerPixel) + (roundedMin.y * backBuffer->pitch);
        
        for (s32 y = roundedMin.y; y < roundedMax.y; ++y)
        {
            if ((pixel >= backBuffer->memory) && ((pixel + 4) <= endOfBuffer))
            {
                *(u32 *)pixel = colour;
            }
            
            pixel += backBuffer->pitch;
        }
    }
}

inline b32 Win32_DrawGUIBox(Win32_BackBuffer *backBuffer, Win32_Input *input, u32 condition, u32 active, v2<s32> min, v2<s32> max, u32 boxColour, u32 rectColour)
{
    // NOTE(bSalmon): As this function is used for GUI buttons, this returns true if the button is clicked
    b32 result = false;
    
    if (condition == active)
    {
        boxColour = 0xFFFF0000;
    }
    
    Win32_DrawHorizontalLine(backBuffer, min.y, min.x, max.x, boxColour);
    Win32_DrawHorizontalLine(backBuffer, max.y, min.x, max.x, boxColour);
    Win32_DrawVerticalLine(backBuffer, min.x, min.y, max.y, boxColour);
    Win32_DrawVerticalLine(backBuffer, max.x, min.y, max.y, boxColour);
    
    if (input->priClicked)
    {
        if ((input->mouseLoc.x >= min.x) && (input->mouseLoc.x < max.x) &&
            (input->mouseLoc.y >= min.y) && (input->mouseLoc.y < max.y))
        {
            result = true;
            input->priClicked = false;
        }
    }
    
    s32 rectPadding = (max.x - min.x) / 4;
    min = min + rectPadding;
    max = max - rectPadding;
    
    v2<f32> minF = {};
    minF.x = (f32)min.x;
    minF.y = (f32)min.y;
    
    v2<f32> maxF = {};
    maxF.x = (f32)max.x;
    maxF.y = (f32)max.y;
    
    DrawRect(backBuffer, minF, maxF, rectColour);
    
    return result;
}

inline void Win32_ClearTileMap(TileMap *tileMap)
{
    u32 tileMapSizeChunks = (tileMap->chunkCount.x * tileMap->chunkCount.y) * tileMap->chunkCount.z;
    for (u32 i = 0; i < tileMapSizeChunks; ++i)
    {
        free(tileMap->chunks[i].tiles);
    }
    
    free(tileMap->chunks);
    tileMap->chunks = (TileChunk *)calloc(tileMapSizeChunks, sizeof(TileChunk));
    
    for (u32 i = 0; i < tileMapSizeChunks; ++i)
    {
        tileMap->chunks[i].tiles = (u8 *)calloc((tileMap->chunkDim * tileMap->chunkDim), sizeof(u8));
    }
}

#define WIN32_URBAN_MAPEDIT_H
#endif // WIN32_URBAN_MAPEDIT_H