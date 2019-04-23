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

#define ZEROMEM(block, size) SetMem(block, 0, size)

// Use CopyMem to copy variables into a data buffer and progress the index so the next slot can be filled
// (and vice versa)
template <class block_type, class var_type>
inline void DataBlockFill(block_type *dataBlockStart, var_type *var, s32 *index)
{
    CopyMem(dataBlockStart + (*index / sizeof(block_type)), var, sizeof(*var));
    *index += sizeof(*var);
}

template <class var_type, class block_type>
inline void FillVariableFromDataBlock(var_type *var, block_type *dataBlockStart, s32 *index)
{
    CopyMem(var, dataBlockStart + (*index / sizeof(block_type)), sizeof(*var));
    *index += sizeof(*var);
}

// TODO(bSalmon): malloc/free/calloc

#define URBAN_MEMORY_H
#endif