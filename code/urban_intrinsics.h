/*
Project: Urban Sprawl
File: urban_intrinsics.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_INTRINSICS_H

#include <intrin.h>

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

inline s32 TruncateF32ToS32(f32 value)
{
    s32 result = _mm_cvtt_ss2si(_mm_set_ss(value));
    return result;
}

#define URBAN_INTRINSICS_H
#endif