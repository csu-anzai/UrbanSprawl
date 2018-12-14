/*
Project: Urban Sprawl PoC
File: urban_memory.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_MEMORY_H

inline void *CopyMem(void *dest, void *src, mem_index size)
{
    char *destP = (char *)dest;
    char *srcP = (char *)src;
    
    for (s32 i = 0; i < size; ++i)
    {
        destP[i] = srcP[i];
    }
    
    return destP;
}

// TODO(bSalmon): malloc/free
// TODO(bSalmon): mmap

#define URBAN_MEMORY_H
#endif