
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
    
    u32 currSegmentX;
    u32 currSegmentY;
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
    u32 priCursor;
    u32 secCursor;
    s32 totalCursors;
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

inline void DrawRect(Win32_BackBuffer *backBuffer, f32 minX, f32 minY, f32 maxX, f32 maxY, u32 colour)
{
    u8 *endOfBuffer = (u8 *)backBuffer->memory + (backBuffer->pitch * backBuffer->height);
    
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
    
    if (roundedMaxX > backBuffer->width)
    {
        roundedMaxX = backBuffer->width;
    }
    
    if (roundedMaxY > backBuffer->height)
    {
        roundedMaxY = backBuffer->height;
    }
    
    for (s32 x = roundedMinX; x < roundedMaxX; ++x)
    {
        u8 *pixel = (u8 *)backBuffer->memory + (x * backBuffer->bytesPerPixel) + (roundedMinY * backBuffer->pitch);
        
        for (s32 y = roundedMinY; y < roundedMaxY; ++y)
        {
            if ((pixel >= backBuffer->memory) && ((pixel + 4) <= endOfBuffer))
            {
                *(u32 *)pixel = colour;
            }
            
            pixel += backBuffer->pitch;
        }
    }
}

inline b32 Win32_DrawGUIBox(Win32_BackBuffer *backBuffer, Win32_Input *input, u32 cursor, s32 minX, s32 minY, s32 maxX, s32 maxY, u32 boxColour, u32 rectColour)
{
    // NOTE(bSalmon): As this function is used for GUI buttons, this returns true if the button is clicked
    b32 result = false;
    
    if (input->priCursor == cursor)
    {
        boxColour = 0xFFFF0000;
    }
    
    Win32_DrawHorizontalLine(backBuffer, minY, minX, maxX, boxColour);
    Win32_DrawHorizontalLine(backBuffer, maxY, minX, maxX, boxColour);
    Win32_DrawVerticalLine(backBuffer, minX, minY, maxY, boxColour);
    Win32_DrawVerticalLine(backBuffer, maxX, minY, maxY, boxColour);
    
    if (input->priClicked)
    {
        if ((input->mouseLoc.x >= minX) && (input->mouseLoc.x < maxX) &&
            (input->mouseLoc.y >= minY) && (input->mouseLoc.y < maxY))
        {
            result = true;
            input->priClicked = false;
        }
    }
    
    s32 rectPadding = (maxX - minX) / 4;
    DrawRect(backBuffer, (f32)(minX + rectPadding), (f32)(minY + rectPadding), (f32)(maxX - rectPadding), (f32)(maxY - rectPadding), rectColour);
    
    return result;
}

inline void Win32_ClearTileMap(TileMap *tileMap)
{
    u32 tileMapSizeChunks = (tileMap->chunkCountX * tileMap->chunkCountY);
    for (u32 i = 0; i < tileMapSizeChunks; ++i)
    {
        free(tileMap->chunks[i].tiles);
    }
    
    free(tileMap->chunks);
    tileMap->chunks = (TileChunk *)calloc(tileMapSizeChunks, sizeof(TileChunk));
    
    for (u32 i = 0; i < tileMapSizeChunks; ++i)
    {
        tileMap->chunks[i].tiles = (u32 *)calloc((tileMap->chunkDim * tileMap->chunkDim), sizeof(u32));
    }
}