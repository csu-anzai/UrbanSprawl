/*
Project: Urban Sprawl PoC
File: urban_memory.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_MEMORY_H

inline void *CopyMem(void *dest, void *src, mem_index size)
{
    u8 *destP = (u8  *)dest;
    u8 *srcP = (u8  *)src;
    
    for (s32 i = 0; i < size; ++i)
    {
        destP[i] = srcP[i];
    }
    
    return destP;
}

inline void *SetMem(void *block, u8 value, mem_index size)
{
    u8 *pointer = (u8 *)block;
    while (size > 0)
    {
        *pointer = value;
        pointer++;
        size--;
    }
    
    return block;
}

// TODO(bSalmon): malloc/free

#define URBAN_MEMORY_H
#endif