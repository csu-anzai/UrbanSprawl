/*
Project: Urban Sprawl
File: urban_intrinsics.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_INTRINSICS_H

#include <intrin.h>

struct BitScanResult
{
    b32 found;
    u32 index;
};

inline s32 RoundF32ToS32(f32 value)
{
    s32 result = _mm_cvtss_si32(_mm_set_ss(value));
    return result;
}

inline s32 RoundF32ToU32(f32 value)
{
    u32 result = (u32)(_mm_cvtss_si32(_mm_set_ss(value)));
    return result;
}

// TODO(bSalmon): Should SSE4.1 be used?
inline s32 FloorF32ToS32(f32 value)
{
    s32 result = _mm_cvtss_si32(_mm_floor_ps(_mm_set_ss(value)));
    return result;
}

inline s32 TruncateF32ToS32(f32 value)
{
    s32 result = _mm_cvtt_ss2si(_mm_set_ss(value));
    return result;
}

inline BitScanResult FindLeastSignificantSetBit(u32 value)
{
    BitScanResult result = {};
    
#if URBAN_WIN32
    result.found = _BitScanForward((unsigned long *)&result.index, value);
#else
    for (u32 test = 0; test < 32; ++test)
    {
        if (value & (1 << test))
        {
            result.index = test;
            result.found = true;
            break;
        }
    }
#endif
    
    return result;
}

#define URBAN_INTRINSICS_H
#endif