#pragma once

typedef enum
{
    PS_CHANNEL_A,
    PS_CHANNEL_B,
    PS_CHANNEL_C,
    PS_CHANNEL_D,
    PS_CHANNEL_E,
    PS_CHANNEL_F,
    PS_CHANNEL_G,
    PS_CHANNEL_H,
    PS_CHANNEL_INVALID
} PS_CHANNEL;

typedef enum
{
    PS_AC,
    PS_DC,
    PS_DC_1M = PS_DC,
    PS_DC_50R
} PS_COUPLING;

typedef enum
{
    ATTEN_1X,
    ATTEN_10X,
    ATTEN_100X,
    ATTEN_1000X
} ATTEN_T;

typedef enum
{
    LOW_NOISE,
    HIGH_NOISE
} SamplingMode_T;