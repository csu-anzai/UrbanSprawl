/*
Project: Urban Sprawl Map Editor
File: win32_urban_mapedit.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#include <Windows.h>

#include "../urban.h"

struct Win32_Menus
{
    HMENU file;
    HMENU settings;
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
    u32 chunkDim;
    u32 chunkCountX;
    u32 chunkCountY;
    u32 tileMapDim;
    u32 segmentDim;
    f32 tileSize;
    s32 segmentDimTiles;
    
    u32 gridLineCount;
    u32 gridDimPixels;
    
    s32 gridLeft;
    s32 gridTop;
    
    u32 *tileArray;
    u32 currSegment;
};

struct Win32_Input
{
    POINT mouseLoc;
};

struct Win32_WindowDimensions
{
	s32 width;
	s32 height;
};

global_var b32 globalRunning;
global_var Win32_BackBuffer globalBackBuffer;

inline Win32_WindowDimensions Win32_GetWindowDimensions(HWND window)
{
	Win32_WindowDimensions result;
	
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
	
	return result;
}
// Resize Device Independent Bitmap Section
internal_func void Win32_ResizeDIBSection(Win32_BackBuffer *backBuffer, s32 width, s32 height)
{
	if (backBuffer->memory)
	{
		VirtualFree(backBuffer->memory, 0, MEM_RELEASE);
	}
	
	backBuffer->width = width;
	backBuffer->height = height;
	backBuffer->bytesPerPixel = 4;
	
	// NOTE[bSalmon]: If biHeight is negative, bitmap is top-down
	// (first 4 bytes are the top left pixel)
	backBuffer->info.bmiHeader.biSize = sizeof(backBuffer->info.bmiHeader);
	backBuffer->info.bmiHeader.biWidth = backBuffer->width;
	backBuffer->info.bmiHeader.biHeight = -backBuffer->height;
	backBuffer->info.bmiHeader.biPlanes = 1;
	backBuffer->info.bmiHeader.biBitCount = 32;
	backBuffer->info.bmiHeader.biCompression = BI_RGB;
	
	s32 bitmapMemorySize = (backBuffer->width * backBuffer->height) * backBuffer->bytesPerPixel;
	backBuffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	backBuffer->pitch = backBuffer->width * backBuffer->bytesPerPixel;
}

// Present the Back Buffer to the screen
internal_func void Win32_PresentBuffer(Win32_BackBuffer *backBuffer, HDC deviceContext, s32 windowWidth, s32 windowHeight)
{
	StretchDIBits(deviceContext,
				  0, 0, windowWidth, windowHeight,	            /// DEST
				  0, 0, backBuffer->width, backBuffer->height,	/// SRC
				  backBuffer->memory,
				  &backBuffer->info,
				  DIB_RGB_COLORS,
				  SRCCOPY);
}

internal_func void Win32_InitMenus(HWND window, Win32_Menus *menus)
{
    HMENU menuBar = CreateMenu();
    
    MENUINFO menuBarInfo = {};
    menuBarInfo.cbSize = sizeof(MENUINFO);
    menuBarInfo.fMask = MIM_APPLYTOSUBMENUS | MIM_STYLE;
    menuBarInfo.dwStyle = MNS_NOTIFYBYPOS;
    
    SetMenuInfo(menuBar, &menuBarInfo);
    
    HMENU file = CreateMenu();
    menus->file = file;
    
    HMENU settings = CreateMenu();
    menus->settings = settings;
    
    AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)file, "File");
    AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)settings, "Settings");
    
    // File Menu Items
    AppendMenuA(file, MF_STRING, 0, "New Map");
    AppendMenuA(file, MF_STRING, 0, "Open Map...");
    AppendMenuA(file, MF_STRING, 0, "Save Map");
    AppendMenuA(file, MF_STRING, 0, "Save Map As...");
    AppendMenuA(file, MF_STRING, 0, "Exit Editor");
    
    // Settings Menu Items
    AppendMenuA(settings, MF_STRING, 0, "Setting 1");
    AppendMenuA(settings, MF_STRING, 0, "Setting 2");
    AppendMenuA(settings, MF_STRING, 0, "Setting 3");
    
    SetMenu(window, menuBar);
}

inline void Win32_DrawVerticalLine(s32 x, s32 top, s32 bottom, u32 colour)
{
    u8 *pixel = ((u8 *)globalBackBuffer.memory + x * globalBackBuffer.bytesPerPixel + top * globalBackBuffer.pitch);
    
    for (s32 y = top; y < bottom; ++y)
    {
        *(u32 *)pixel = colour;
        pixel += globalBackBuffer.pitch;
    }
}

inline void Win32_DrawHorizontalLine(s32 y, s32 left, s32 right, u32 colour)
{
    u8 *pixel = ((u8 *)globalBackBuffer.memory + left * globalBackBuffer.bytesPerPixel + y * globalBackBuffer.pitch);
    
    for (s32 x = left; x < right; ++x)
    {
        *(u32 *)pixel = colour;
        pixel += globalBackBuffer.bytesPerPixel;
    }
}

internal_func void DrawRect(f32 minX, f32 minY, f32 maxX, f32 maxY, u32 colour)
{
    u8 *endOfBuffer = (u8 *)globalBackBuffer.memory + (globalBackBuffer.pitch * globalBackBuffer.height);
    
    s32 roundedMinX = RoundF32ToS32(minX);
    s32 roundedMinY = RoundF32ToS32(minY);
    s32 roundedMaxX = RoundF32ToS32(maxX);
    s32 roundedMaxY = RoundF32ToS32(maxY);
    
    if (roundedMinX < 0)
    {
        roundedMinX = 0;
    }
    
    if (roundedMinY < 0)
    {
        roundedMinY = 0;
    }
    
    if (roundedMaxX > globalBackBuffer.width)
    {
        roundedMaxX = globalBackBuffer.width;
    }
    
    if (roundedMaxY > globalBackBuffer.height)
    {
        roundedMaxY = globalBackBuffer.height;
    }
    
    for (s32 x = roundedMinX; x < roundedMaxX; ++x)
    {
        u8 *pixel = (u8 *)globalBackBuffer.memory + (x * globalBackBuffer.bytesPerPixel) + (roundedMinY * globalBackBuffer.pitch);
        
        for (s32 y = roundedMinY; y < roundedMaxY; ++y)
        {
            if ((pixel >= globalBackBuffer.memory) && ((pixel + 4) <= endOfBuffer))
            {
                *(u32 *)pixel = colour;
            }
            
            pixel += globalBackBuffer.pitch;
        }
    }
}

internal_func void Win32_DrawMapGrid(Win32_MapInfo *mapInfo, s32 minX, s32 minY, s32 maxX, s32 maxY, u32 gridColour)
{
    // Draw Border
    Win32_DrawHorizontalLine(minY, minX, maxX, 0xFFFF0000);
    Win32_DrawHorizontalLine(maxY, minX, maxX, 0xFFFF0000);
    Win32_DrawVerticalLine(minX, minY, maxY, 0xFFFF0000);
    Win32_DrawVerticalLine(maxX, minY, maxY, 0xFFFF0000);
    
    // Draw Grid Lines
    u32 chunkCounterX = 0;
    for (f32 x = ((f32)minX + mapInfo->tileSize); x < maxX; x += mapInfo->tileSize)
    {
        ++chunkCounterX;
        
        if (chunkCounterX == mapInfo->chunkDim)
        {
            gridColour = 0xFFFF0000;
        }
        
        Win32_DrawVerticalLine((s32)x, minY, maxY, gridColour);
        
        if (chunkCounterX == mapInfo->chunkDim)
        {
            gridColour = 0xFFFFFFFF;
            chunkCounterX = 0;
        }
    }
    
    u32 chunkCounterY = 0;
    for (f32 y = ((f32)minY + mapInfo->tileSize); y < maxY; y += mapInfo->tileSize)
    {
        ++chunkCounterY;
        
        if (chunkCounterY == mapInfo->chunkDim)
        {
            gridColour = 0xFFFF0000;
        }
        
        Win32_DrawHorizontalLine((s32)y, minX, maxX, gridColour);
        
        if (chunkCounterY == mapInfo->chunkDim)
        {
            gridColour = 0xFFFFFFFF;
            chunkCounterY = 0;
        }
    }
}

internal_func void Win32_DrawTiles(Win32_MapInfo *mapInfo)
{
    u32 segmentDim = mapInfo->chunkDim * 4;
    u32 gridDim = mapInfo->chunkDim * mapInfo->chunkCountX;
    
    for (u32 y = 0; y < segmentDim; ++y)
    {
        for (u32 x = 0; x < segmentDim; ++x)
        {
            u32 tileID = mapInfo->tileArray[(((gridDim * mapInfo->currSegment) + y) * gridDim) + ((gridDim * mapInfo->currSegment) + x)];
            
            u32 tileColour = 0xFF888888;
            
            if (tileID == 1)
            {
                tileColour = 0xFFFFFFFF;
            }
            
            f32 minX = (f32)(mapInfo->gridLeft + (x * mapInfo->tileSize));
            f32 minY = (f32)(mapInfo->gridTop + (y * mapInfo->tileSize));
            f32 maxX = (f32)(minX + mapInfo->tileSize);
            f32 maxY = (f32)(minY + mapInfo->tileSize);
            
            DrawRect(minX, minY, maxX, maxY, tileColour);
        }
    }
};

LRESULT CALLBACK Win32_WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
    switch (msg)
    {
        case WM_CLOSE:
        {
            globalRunning = false;
            break;
        }
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            
            Win32_WindowDimensions windowDim = Win32_GetWindowDimensions(window);
            Win32_PresentBuffer(&globalBackBuffer, deviceContext, windowDim.width, windowDim.height);
            
            EndPaint(window, &paint);
            break;
        }
        
        default:
        {
            result = DefWindowProcA(window, msg, wParam, lParam);
            break;
        }
    }
    
    return result;
}

s32 CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, s32 showCode)
{
    globalRunning = true;
    
    Win32_ResizeDIBSection(&globalBackBuffer, 1600, 900);
    
    WNDCLASSA wndClass = {};
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndClass.lpfnWndProc = Win32_WndProc;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "MapEditorWindowClass";
    
    if (RegisterClassA(&wndClass))
    {
        HWND window = CreateWindowExA(0,
                                      wndClass.lpszClassName, "Urban Sprawl Map Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
                                      0, 0, instance, 0);
        
        if (window)
        {
            Win32_Menus menus = {};
            Win32_InitMenus(window, &menus);
            
            HDC deviceContext = GetDC(window);
            
            Win32_MapInfo mapInfo = {};
            mapInfo.chunkDim = 16;
            mapInfo.chunkCountX = 128;
            mapInfo.chunkCountY = 128;
            mapInfo.tileMapDim = mapInfo.chunkDim * mapInfo.chunkCountX;
            mapInfo.segmentDim = mapInfo.tileMapDim / 32;
            mapInfo.gridLineCount = mapInfo.segmentDim - 1;
            mapInfo.tileArray = (u32 *)malloc((128 * 128) * (16 * 16));
            mapInfo.currSegment = 0;
            
            while (globalRunning)
            {
                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                {
                    switch (message.message)
                    {
                        case WM_MENUCOMMAND:
                        {
                            // Handle Menu Commands
                            break;
                        }
                        
                        case WM_QUIT:
                        {
                            globalRunning = false;
                            break;
                        }
                        
                        default:
                        {
                            TranslateMessage(&message);
                            DispatchMessageA(&message);
                            break;
                        }
                    }
                }
                
                Win32_Input input = {};
                POINT mouseLocation;
                GetCursorPos(&mouseLocation);
                ScreenToClient(window, &mouseLocation);
                input.mouseLoc = mouseLocation;
                
                // NOTE(bSalmon): 1 pixel padding
                s32 padding = 1;
                s32 maxX = (globalBackBuffer.width - 1) - padding;
                s32 maxY = (globalBackBuffer.height - 1) - padding;
                
                mapInfo.gridTop = padding;
                mapInfo.gridDimPixels = maxY - mapInfo.gridTop;
                mapInfo.tileSize = (f32)mapInfo.gridDimPixels / (f32)mapInfo.segmentDim;
                mapInfo.segmentDimTiles = (s32)(mapInfo.gridDimPixels / mapInfo.tileSize);
                mapInfo.gridLeft = maxX - (maxY - mapInfo.gridTop);
                
                mapInfo.tileArray[2049] = 1;
                
                Win32_DrawTiles(&mapInfo);
                Win32_DrawMapGrid(&mapInfo, mapInfo.gridLeft, mapInfo.gridTop, maxX, maxY, 0xFFFFFFFF);
                
                Win32_WindowDimensions windowDim = Win32_GetWindowDimensions(window);
                Win32_PresentBuffer(&globalBackBuffer, deviceContext, windowDim.width, windowDim.height);
            }
        }
    }
    
    return 0;
};