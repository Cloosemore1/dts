#pragma once

typedef struct dts_timecode {
    unsigned short frame;
    unsigned short serial;
    unsigned char reel;
} dts_timecode;