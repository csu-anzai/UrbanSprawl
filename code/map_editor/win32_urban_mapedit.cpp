/*
Project: Urban Sprawl Map Editor
File: win32_urban_mapedit.cpp
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

// TODO(bSalmon): Pull out both Grid drawing to be its own function?

#include <Windows.h>

#include "../urban.h"
#include "../urban_tile.cpp"
#include "win32_urban_mapedit.h"

global_var b32 globalRunning;
global_var Win32_BackBuffer globalBackBuffer;

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
    
    HMENU tools = CreateMenu();
    menus->tools = tools;
    
    AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)file, "File");
    AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)tools, "Tools");
    
    // File Menu Items
    AppendMenuA(file, MF_STRING, 0, "New Map");
    AppendMenuA(file, MF_STRING, 0, "Open Map...");
    AppendMenuA(file, MF_STRING, 0, "Save Map...");
    AppendMenuA(file, MF_STRING, 0, "Exit Editor");
    
    // Settings Menu Items
    AppendMenuA(tools, MF_STRING, 0, "Rect Cursor");
    AppendMenuA(tools, MF_STRING, 0, "Setting 2");
    AppendMenuA(tools, MF_STRING, 0, "Setting 3");
    
    SetMenu(window, menuBar);
}

internal_func void Win32_DrawMapGrid(Win32_BackBuffer *backBuffer, Win32_MapInfo *mapInfo, TileMap *tileMap, v2<s32> min, v2<s32> max, u32 gridColour)
{
    // Draw Border
    Win32_DrawHorizontalLine(backBuffer, min.y, min.x, max.x, 0xFFFF0000);
    Win32_DrawHorizontalLine(backBuffer, max.y, min.x, max.x, 0xFFFF0000);
    Win32_DrawVerticalLine(backBuffer, min.x, min.y, max.y, 0xFFFF0000);
    Win32_DrawVerticalLine(backBuffer, max.x, min.y, max.y, 0xFFFF0000);
    
    // Draw Grid Lines
    u32 chunkCounterX = 0;
    for (f32 x = ((f32)min.x + mapInfo->tileSide); x < max.x; x += mapInfo->tileSide)
    {
        ++chunkCounterX;
        
        if (chunkCounterX == tileMap->chunkDim)
        {
            gridColour = 0xFFFF0000;
        }
        
        Win32_DrawVerticalLine(backBuffer, (s32)x, min.y, max.y, gridColour);
        
        if (chunkCounterX == tileMap->chunkDim)
        {
            gridColour = 0xFFFFFFFF;
            chunkCounterX = 0;
        }
    }
    
    u32 chunkCounterY = 0;
    for (f32 y = ((f32)min.y + mapInfo->tileSide); y < max.y; y += mapInfo->tileSide)
    {
        ++chunkCounterY;
        
        if (chunkCounterY == tileMap->chunkDim)
        {
            gridColour = 0xFFFF0000;
        }
        
        Win32_DrawHorizontalLine(backBuffer, (s32)y, min.x, max.x, gridColour);
        
        if (chunkCounterY == tileMap->chunkDim)
        {
            gridColour = 0xFFFFFFFF;
            chunkCounterY = 0;
        }
    }
}

internal_func void Win32_FillRect(TileMap *tileMap, Win32_MapInfo *mapInfo, v2<u32> absTileMin, v2<u32> absTileMax, u8 tileID)
{
    for (u32 y = absTileMin.y; y <= absTileMax.y; ++y)
    {
        for (u32 x = absTileMin.x; x <= absTileMax.x; ++x)
        {
            TileChunkPosition chunkPos = GetChunkPosition(tileMap, v3<u32>{x, y, mapInfo->currZ});
            TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
            SetTileValue(tileMap, chunk, chunkPos.relTile, tileID);
        }
    }
}

internal_func void ProcessRectCursor(Win32_Input *input, Win32_MapInfo *mapInfo, TileMap *tileMap, v2<u32> rectPos, v2<f32> min, v2<f32> max)
{
    local_persist u32 absTileTop = 0;
    local_persist u32 absTileBottom = 0;
    local_persist u32 absTileLeft = 0;
    local_persist u32 absTileRight = 0;
    
    if ((input->mouseLoc.x >= min.x) && (input->mouseLoc.x < max.x) &&
        (input->mouseLoc.y >= min.y) && (input->mouseLoc.y < max.y))
    {
        TileChunkPosition chunkPos = GetChunkPosition(tileMap, v3<u32>{rectPos.x, rectPos.y, mapInfo->currZ});
        TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
        
        if (!input->topLeftSet)
        {
            if (input->priClicked)
            {
                if (input->priCursor != TILE_STAIRS_UP && input->priCursor != TILE_STAIRS_DOWN)
                {
                    SetTileValue(tileMap, chunk, chunkPos.relTile, 0xFF);
                    absTileTop = rectPos.y;
                    absTileLeft = rectPos.x;
                    input->topLeftSet = true;
                }
                
                input->priClicked = false;
            }
        }
        else
        {
            if (input->priClicked)
            {
                absTileBottom = rectPos.y;
                absTileRight = rectPos.x;
                
                // Swap axis values if the user's second click is above or to the left of the first
                if (absTileTop > absTileBottom)
                {
                    SWAP(absTileTop, absTileBottom);
                }
                
                if (absTileLeft > absTileRight)
                {
                    SWAP(absTileLeft, absTileRight);
                }
                
                Win32_FillRect(tileMap, mapInfo, v2<u32>{absTileLeft, absTileTop}, v2<u32>{absTileRight, absTileBottom}, input->priCursor);
                
                input->topLeftSet = false;
                input->priClicked = false;
            }
        }
    }
}

internal_func void Win32_UpdateRenderEditorMap(Win32_BackBuffer *backBuffer, Win32_MapInfo *mapInfo, TileMap *tileMap, Win32_Input *input)
{
    v2<u32> beginningOfSegment = mapInfo->currSegment * mapInfo->segmentDimTiles;
    
    for (u32 y = beginningOfSegment.y; y < (beginningOfSegment.y + mapInfo->segmentDimTiles); ++y)
    {
        for (u32 x = beginningOfSegment.x; x < (beginningOfSegment.x + mapInfo->segmentDimTiles); ++x)
        {
            u8 tileID = GetTileValue(tileMap, v3<u32>{x, y, mapInfo->currZ});
            
            v2<f32> min = {};
            min.x = (f32)(mapInfo->gridLeft + ((x - beginningOfSegment.x) * mapInfo->tileSide));
            min.y = (f32)(mapInfo->gridTop + ((y - beginningOfSegment.y) * mapInfo->tileSide));
            
            v2<f32> max = min + mapInfo->tileSide;
            
            if (input->priClicked || input->secClicked)
            {
                if (!input->rectCursor)
                {
                    if ((input->mouseLoc.x >= min.x) && (input->mouseLoc.x < max.x) &&
                        (input->mouseLoc.y >= min.y) && (input->mouseLoc.y < max.y))
                    {
                        TileChunkPosition chunkPos = GetChunkPosition(tileMap, v3<u32>{x, y, mapInfo->currZ});
                        TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
                        
                        if (input->priClicked)
                        {
                            if (input->priCursor != TILE_STAIRS_UP && input->priCursor != TILE_STAIRS_DOWN)
                            {
                                SetTileValue(tileMap, chunk, chunkPos.relTile, input->priCursor);
                            }
                            else
                            {
                                if (input->priCursor == TILE_STAIRS_UP && mapInfo->currZ != tileMap->chunkCount.z)
                                {
                                    SetTileValue(tileMap, chunk, chunkPos.relTile, input->priCursor);
                                    
                                    TileChunkPosition altChunkPos = GetChunkPosition(tileMap, v3<u32>{x, y, mapInfo->currZ + 1});
                                    TileChunk *altChunk = GetTileChunk(tileMap, altChunkPos.chunk);
                                    
                                    SetTileValue(tileMap, altChunk, chunkPos.relTile, TILE_STAIRS_DOWN);
                                }
                                else if (input->priCursor == TILE_STAIRS_DOWN && mapInfo->currZ != 0)
                                {
                                    SetTileValue(tileMap, chunk, chunkPos.relTile, input->priCursor);
                                    
                                    TileChunkPosition altChunkPos = GetChunkPosition(tileMap, v3<u32>{x, y, mapInfo->currZ - 1});
                                    TileChunk *altChunk = GetTileChunk(tileMap, altChunkPos.chunk);
                                    
                                    SetTileValue(tileMap, altChunk, chunkPos.relTile, TILE_STAIRS_UP);
                                }
                            }
                            
                            input->priClicked = false;
                        }
                        else if (input->secClicked)
                        {
                            SetTileValue(tileMap, chunk, chunkPos.relTile, input->secCursor);
                            input->secClicked = false;
                        }
                    }
                }
                else
                {
                    ProcessRectCursor(input, mapInfo, tileMap, v2<u32>{x, y}, min, max);
                }
            }
            
            u32 tileColour = 0xFF4C0296;
            if (tileID == TILE_WALKABLE)
            {
                tileColour = 0xFF888888;
            }
            if (tileID == TILE_WALL)
            {
                tileColour = 0xFFFFFFFF;
            }
            if (tileID == TILE_STAIRS_UP)
            {
                tileColour = 0xFF00FFFF;
            }
            if (tileID == TILE_STAIRS_DOWN)
            {
                tileColour = 0xFF008080;
            }
            if (tileID == 0xFFFFFFFF)
            {
                tileColour = 0xFF00FF00;
            }
            
            DrawRect(backBuffer, min, max, tileColour);
        }
    }
    
    Win32_DrawMapGrid(backBuffer, mapInfo, tileMap, v2<s32>{mapInfo->gridLeft, mapInfo->gridTop}, v2<s32>{mapInfo->gridRight, mapInfo->gridBottom}, 0xFFFFFFFF);
}

internal_func void Win32_DrawSelectorGrid(Win32_BackBuffer *backBuffer, Win32_SelectorInfo *selectorInfo, u32 gridColour)
{
    v2<s32> min = {selectorInfo->gridLeft, selectorInfo->gridTop};
    v2<s32> max = {selectorInfo->gridRight, selectorInfo->gridBottom};
    
    // Draw Border
    Win32_DrawHorizontalLine(backBuffer, min.y, min.x, max.x, 0xFF00FF00);
    Win32_DrawHorizontalLine(backBuffer, max.y, min.x, max.x, 0xFF00FF00);
    Win32_DrawVerticalLine(backBuffer, min.x, min.y, max.y, 0xFF00FF00);
    Win32_DrawVerticalLine(backBuffer, max.x, min.y, max.y, 0xFF00FF00);
    
    // Draw Grid Lines
    for (f32 x = ((f32)min.x + selectorInfo->tileSide); x < max.x; x += selectorInfo->tileSide)
    {
        Win32_DrawVerticalLine(backBuffer, (s32)x, min.y, max.y, gridColour);
    }
    
    for (f32 y = ((f32)min.y + selectorInfo->tileSide); y < max.y; y += selectorInfo->tileSide)
    {
        Win32_DrawHorizontalLine(backBuffer, (s32)y, min.x, max.x, gridColour);
    }
    
    // Draw Selected Square
    s32 selectedSquareTop = (s32)((f32)min.y + (selectorInfo->tileSide * (f32)selectorInfo->currSelection->y));
    s32 selectedSquareBottom = (s32)(selectedSquareTop + selectorInfo->tileSide);
    s32 selectedSquareLeft = (s32)((f32)min.x + (selectorInfo->tileSide * (f32)selectorInfo->currSelection->x));
    s32 selectedSquareRight = (s32)(selectedSquareLeft + selectorInfo->tileSide);
    
    Win32_DrawHorizontalLine(backBuffer, selectedSquareTop, selectedSquareLeft, selectedSquareRight, 0xFFFF0000);
    Win32_DrawHorizontalLine(backBuffer, selectedSquareBottom, selectedSquareLeft, selectedSquareRight, 0xFFFF0000);
    Win32_DrawVerticalLine(backBuffer, selectedSquareLeft, selectedSquareTop, selectedSquareBottom, 0xFFFF0000);
    Win32_DrawVerticalLine(backBuffer, selectedSquareRight, selectedSquareTop, selectedSquareBottom, 0xFFFF0000);
}

internal_func void Win32_UpdateRenderSelector(Win32_BackBuffer *backBuffer, Win32_SelectorInfo *selectorInfo, Win32_Input *input)
{
    for (u32 y = 0; y < (selectorInfo->gridLineCount + 1); ++y)
    {
        for (u32 x = 0; x < (selectorInfo->gridLineCount + 1); ++x)
        {
            v2<f32> min = {(f32)(selectorInfo->gridLeft + (x * selectorInfo->tileSide)),
                (f32)(selectorInfo->gridTop + (y * selectorInfo->tileSide))};
            
            v2<f32> max = min + selectorInfo->tileSide;
            
            if (input->priClicked)
            {
                if ((input->mouseLoc.x >= min.x) && (input->mouseLoc.x < max.x) &&
                    (input->mouseLoc.y >= min.y) && (input->mouseLoc.y < max.y))
                {
                    selectorInfo->currSelection->x = (s32)((min.x - selectorInfo->gridLeft) / selectorInfo->tileSide);
                    selectorInfo->currSelection->y = (s32)((min.y - selectorInfo->gridTop) / selectorInfo->tileSide);
                    input->priClicked = false;
                }
            }
            
            u32 tileColour = 0xFF888888;
            
            DrawRect(backBuffer, min, max, tileColour);
        }
    }
    
    Win32_DrawSelectorGrid(&globalBackBuffer, selectorInfo, 0xFFFFFFFF);
}

internal_func void Win32_UpdateRenderCursorBoxes(Win32_BackBuffer *backBuffer, Win32_Input *input, u8 startCursor, u8 endCursor, v2<s32> padding, s32 boxDim, u32 boxColour)
{
    for (u8 i = startCursor; i < endCursor; ++i)
    {
        u32 rectColour = 0xFF000000;
        switch (i)
        {
            case 0:
            {
                rectColour = 0xFF4C0296;
                break;
            }
            
            case 1:
            {
                rectColour = 0xFF888888;
                break;
            }
            
            
            case 2:
            {
                rectColour = 0xFFFFFFFF;
                break;
            }
            
            
            case 3:
            {
                rectColour = 0xFF00FFFF;
                break;
            }
            
            
            case 4:
            {
                rectColour = 0xFF008080;
                break;
            }
            
            default:
            {
                break;
            }
        }
        
        s32 minX = (i * boxDim) + i;
        if (Win32_DrawGUIBox(backBuffer, input, input->priCursor, i, v2<s32>{minX, padding.y}, v2<s32>{minX + boxDim, padding.y + boxDim}, boxColour, rectColour))
        {
            input->priCursor = i;
        }
    }
}

internal_func void Win32_UpdateRenderFloorBoxes(Win32_BackBuffer *backBuffer, Win32_MapInfo *mapInfo, Win32_Input *input, u32 endFloor, v2<s32> padding, s32 boxDim, u32 boxColour)
{
    for (u32 i = 0; i < endFloor; ++i)
    {
        s32 minY = padding.y - ((i * boxDim) + i);
        if (Win32_DrawGUIBox(backBuffer, input, mapInfo->currZ, i, v2<s32>{padding.x, minY}, v2<s32>{padding.x + boxDim, minY + boxDim}, boxColour, 0xFF000000))
        {
            mapInfo->currZ = i;
        }
    }
}

internal_func void Win32_LoadMap(TileMap *tileMap, char *fileName)
{
    Win32_ClearTileMap(tileMap);
    
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        u8 *readBlock = (u8 *)malloc(((tileMap->chunkCount.x * tileMap->chunkCount.y) * (tileMap->chunkDim * tileMap->chunkDim) * tileMap->chunkCount.z));
        
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            u32 fileSize32 = SafeTruncateU64(fileSize.QuadPart);
            DWORD bytesRead;
            ReadFile(fileHandle, readBlock, fileSize32, &bytesRead, 0);
            
            for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
            {
                for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCount.y); ++y)
                {
                    for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCount.x); ++x)
                    {
                        TileChunkPosition chunkPos = GetChunkPosition(tileMap, v3<u32>{x, y, z});
                        TileChunk *chunk = GetTileChunk(tileMap, chunkPos.chunk);
                        
                        SetTileValue(tileMap, chunk, chunkPos.relTile, readBlock[(z * ((tileMap->chunkDim * tileMap->chunkCount.x) * (tileMap->chunkDim * tileMap->chunkCount.y))) + (y * (tileMap->chunkDim * tileMap->chunkCount.x)) + x]);
                    }
                }
            }
        }
        
        CloseHandle(fileHandle);
        free(readBlock);
    }
}

internal_func void Win32_SaveMap(TileMap *tileMap, char *filename)
{
    HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        u8 *writeBlock = (u8 *)malloc(((tileMap->chunkCount.x * tileMap->chunkCount.y) * (tileMap->chunkDim * tileMap->chunkDim) * tileMap->chunkCount.z));
        s32 index = 0;
        
        for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
        {
            for (u32 y = 0; y < (tileMap->chunkDim * tileMap->chunkCount.y); ++y)
            {
                for (u32 x = 0; x < (tileMap->chunkDim * tileMap->chunkCount.x); ++x)
                {
                    u8 tileID = GetTileValue(tileMap, v3<u32>{x, y, z});
                    DataBlockFill<u8, u8>(writeBlock, &tileID, &index);
                }
            }
        }
        
        DWORD bytesWritten;
        WriteFile(fileHandle, writeBlock, index, &bytesWritten, 0);
        
        CloseHandle(fileHandle);
        free(writeBlock);
    }
}

internal_func void Win32_HandleMenuCommands(Win32_Menus menus, TileMap *tileMap, Win32_Input *input, HWND window, WPARAM wParam, LPARAM lParam)
{
    // NOTE(bSalmon): Emulator Options and Settings currently only have one item so there is no need for nested if statements
    HMENU selectedMenu = (HMENU)lParam;
    s32 itemPos = (s32)wParam;
    if (selectedMenu == menus.file)
    {
        switch (itemPos)
        {
            // New Map
            case 0:
            {
                Win32_ClearTileMap(tileMap);
                break;
            }
            
            // Open Map
            case 1:
            {
                char filename[1024];
                OPENFILENAMEA openFileNameInfo = {};
                openFileNameInfo.lStructSize = sizeof(OPENFILENAMEA);
                openFileNameInfo.hwndOwner = window;
                openFileNameInfo.lpstrFilter = "All Files\0*.*\0\0";
                openFileNameInfo.lpstrFile = filename;
                openFileNameInfo.nMaxFile = 512;
                openFileNameInfo.lpstrTitle = "Open Urban Sprawl Map File";
                openFileNameInfo.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&openFileNameInfo))
                {
                    Win32_LoadMap(tileMap, filename);
                }
                
                break;
            }
            
            // Save Map
            case 2:
            {
                char filename[256];
                OPENFILENAMEA openFileNameInfo = {};
                openFileNameInfo.lStructSize = sizeof(OPENFILENAMEA);
                openFileNameInfo.hwndOwner = window;
                openFileNameInfo.lpstrFilter = "USM - Urban Sprawl Map File\0*.USM\0";
                openFileNameInfo.lpstrFile = filename;
                openFileNameInfo.nMaxFile = 512;
                openFileNameInfo.lpstrTitle = "Save Urban Sprawl Map File";
                openFileNameInfo.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                openFileNameInfo.lpstrDefExt = "USM";
                if (GetSaveFileNameA(&openFileNameInfo))
                {
                    Win32_SaveMap(tileMap, filename);
                }
                
                break;
            }
            
            // Exit Editor
            case 3:
            {
                globalRunning = false;
                break;
            }
            
            default:
            {
                break;
            }
        }
        
    }
    else if (selectedMenu == menus.tools)
    {
        switch (itemPos)
        {
            case 0:
            {
                MENUITEMINFOA menuItemInfo = {};
                menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
                if (input->rectCursor)
                {
                    menuItemInfo.fMask = MIIM_CHECKMARKS | MIIM_FTYPE | MIIM_STATE | MIIM_STRING ;
                    menuItemInfo.fType = MFT_STRING;
                    menuItemInfo.fState = MFS_UNCHECKED;
                    menuItemInfo.dwTypeData = "Rect Cursor";
                    input->rectCursor = false;
                }
                else
                {
                    menuItemInfo.fMask = MIIM_CHECKMARKS | MIIM_FTYPE | MIIM_STATE | MIIM_STRING ;
                    menuItemInfo.fType = MFT_STRING;
                    menuItemInfo.fState = MFS_CHECKED;
                    menuItemInfo.dwTypeData = "Rect Cursor";
                    input->rectCursor = true;
                }
                
                SetMenuItemInfo(selectedMenu, itemPos, TRUE, &menuItemInfo);
                break;
            }
            
            default:
            {
                break;
            }
        }
    }
}

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
            Win32_ResizeDIBSection(&globalBackBuffer, windowDim.width, windowDim.height);
            
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
            HDC deviceContext = GetDC(window);
            
            Win32_Menus menus = {};
            Win32_InitMenus(window, &menus);
            
            World *world = (World *)malloc(sizeof(World));
            world->tileMap = (TileMap *)malloc(sizeof(TileMap));
            TileMap *tileMap = world->tileMap;
            
            tileMap->chunkShift = 4;
            tileMap->chunkMask = (1 << tileMap->chunkShift) - 1;
            tileMap->chunkDim = (1 << tileMap->chunkShift);
            
            tileMap->chunkCount = {128, 128, 2};
            tileMap->chunks = (TileChunk *)calloc((tileMap->chunkCount.x * tileMap->chunkCount.y) * tileMap->chunkCount.z, sizeof(TileChunk));
            
            for (u32 z = 0; z < tileMap->chunkCount.z; ++z)
            {
                for (u32 y = 0; y < tileMap->chunkCount.y; ++y)
                {
                    for (u32 x = 0; x < tileMap->chunkCount.x; ++x)
                    {
                        // NOTE(bSalmon): Use calloc to zero the tiles
                        tileMap->chunks[x + (tileMap->chunkCount.x * y) + ((tileMap->chunkCount.x * tileMap->chunkCount.y) * z)].tiles =(u8 *)calloc((tileMap->chunkDim * tileMap->chunkDim), sizeof(u8));
                    }
                }
            }
            
            Win32_MapInfo mapInfo = {};
            mapInfo.segmentDimChunks = 4;
            mapInfo.segmentDimTiles = tileMap->chunkDim * mapInfo.segmentDimChunks;
            mapInfo.gridLineCount = mapInfo.segmentDimTiles - 1;
            mapInfo.currSegment = {};
            mapInfo.currZ = 0;
            
            Win32_SelectorInfo selectorInfo = {};
            selectorInfo.currSelection = (POINT *)malloc(sizeof(POINT));
            selectorInfo.currSelection->x = 0;
            selectorInfo.currSelection->y = 0;
            
            Win32_Input input = {};
            input.priCursor = 1;
            input.secCursor = 0;
            input.totalCursors = 5;
            input.rectCursor = false;
            input.topLeftSet = false;
            
            while (globalRunning)
            {
                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                {
                    switch (message.message)
                    {
                        case WM_MENUCOMMAND:
                        {
                            Win32_HandleMenuCommands(menus, tileMap, &input, window, message.wParam, message.lParam);
                            break;
                        }
                        
                        case WM_QUIT:
                        {
                            globalRunning = false;
                            break;
                        }
                        
                        case WM_LBUTTONDOWN:
                        {
                            input.priClicked = true;
                            
                            POINT mouseLocation;
                            GetCursorPos(&mouseLocation);
                            ScreenToClient(window, &mouseLocation);
                            input.mouseLoc = mouseLocation;
                            
                            break;
                        }
                        
                        case WM_RBUTTONDOWN:
                        {
                            input.secClicked = true;
                            
                            POINT mouseLocation;
                            GetCursorPos(&mouseLocation);
                            ScreenToClient(window, &mouseLocation);
                            input.mouseLoc = mouseLocation;
                            
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
                
                // NOTE(bSalmon): 1 pixel padding
                mapInfo.padding = 1;
                mapInfo.gridRight = (globalBackBuffer.width - 1) - mapInfo.padding;
                mapInfo.gridBottom = (globalBackBuffer.height - 1) - mapInfo.padding;
                
                mapInfo.gridTop = mapInfo.padding;
                mapInfo.gridDimPixels = mapInfo.gridBottom - mapInfo.gridTop;
                mapInfo.tileSide = (f32)mapInfo.gridDimPixels / (f32)mapInfo.segmentDimTiles;
                mapInfo.gridLeft = mapInfo.gridRight - (mapInfo.gridBottom - mapInfo.gridTop);
                
                Win32_UpdateRenderEditorMap(&globalBackBuffer, &mapInfo, tileMap, &input);
                
                selectorInfo.gridLineCount = (tileMap->chunkCount.x / 4) - 1;
                selectorInfo.gridDimPixels = globalBackBuffer.width / 4;
                selectorInfo.padding = 1;
                selectorInfo.gridLeft = selectorInfo.padding;
                selectorInfo.gridRight = selectorInfo.gridLeft + selectorInfo.gridDimPixels;
                selectorInfo.gridBottom = (globalBackBuffer.height - 1) - selectorInfo.padding;
                selectorInfo.gridTop = selectorInfo.gridBottom - selectorInfo.gridDimPixels;
                selectorInfo.tileSide = (f32)selectorInfo.gridDimPixels / ((f32)tileMap->chunkCount.x / (f32)mapInfo.segmentDimChunks);
                
                Win32_UpdateRenderSelector(&globalBackBuffer, &selectorInfo, &input);
                
                mapInfo.currSegment.x = selectorInfo.currSelection->x;
                mapInfo.currSegment.y = selectorInfo.currSelection->y;
                
                if (!input.topLeftSet)
                {
                    s32 boxDim = (globalBackBuffer.height / 2) / 10;
                    
                    Win32_UpdateRenderCursorBoxes(&globalBackBuffer, &input, 0, input.totalCursors, v2<s32>{1, 1}, boxDim, 0xFFFFFFFF);
                    
                    v2<s32> floorBoxPad = {mapInfo.gridLeft - (boxDim + (boxDim / 10)), globalBackBuffer.height - (boxDim + (boxDim / 10))}; 
                    Win32_UpdateRenderFloorBoxes(&globalBackBuffer, &mapInfo, &input, tileMap->chunkCount.z, floorBoxPad, boxDim, 0xFFFFFFFF);
                }
                
                Win32_WindowDimensions windowDim = Win32_GetWindowDimensions(window);
                Win32_PresentBuffer(&globalBackBuffer, deviceContext, windowDim.width, windowDim.height);
            }
        }
    }
    
    return 0;
};