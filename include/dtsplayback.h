#pragma once
#include <alsa/asoundlib.h>
#include "libaptx100.h"
#include "dtstimecode.h"

#define AUDIO_START 102

typedef enum dts_channel { LEFT, RIGHT, CENTER, LEFT_SURROUND, RIGHT_SURROUND } dts_channel;

//DTS .AUD and .AUE file header - 95 bytes long
typedef struct dts_header {
    unsigned char dts_title[60];
    unsigned char dts_lang[8];
    unsigned char dts_studio[6];
    unsigned char dts_unknown1;
    unsigned char dts_backup_type;
    unsigned short dts_unknown2;
    unsigned short dts_reel;
    unsigned short dts_serial;
    unsigned short dts_tracks;
    unsigned char dts_start_frms;
    unsigned char dts_start_secs;
    unsigned char dts_start_mins;
    unsigned char dts_start_hrs;
    unsigned char dts_end_frms;
    unsigned char dts_end_secs;
    unsigned char dts_end_mins;
    unsigned char dts_end_hrs;
    //Only present on .AUE and .APX files, otherwise audio data starts here (Byte 92)
    unsigned char dts_encrypted;
    unsigned char dts_encryption_key[2];
} dts_header;

/*
    Retrieves header information from a DTS .AUD or .AUE file
    Parameters:
        fptr - A pointer to the .AUD or .AUE file to be read
        header - A pointer to a dts_header
*/
void get_dts_header(FILE* fptr, dts_header *header);

/*
    Retrieves and decodes to PCM a given frame of apt-X100 audio for a given channel from a DTS AUD file
    Parameters:
        fptr - A pointer to the .AUD file to be read
        channel - The dts_channel to be read from
        frame - The frame number to read
        pcmBuf - The buffer to write the decoded PCM samples to
        pcmContext - A buffer of two PCM samples, since DTS frames are 367.5 apt-X100 samples long,
                    even numbered frames will decode 368 samples, and store the extra 2 pcm samples
                    back to this buffer to be added to the start of the next frame when decoded.
        aptxContext - The aptxContext for the given channel
*/
void decode_dts_frame(FILE* fptr, int channel, int frame, short *pcmBuf, short *pcmContext, aptxCtx_t *aptxContext);

/*
    Plays back the reel soundtrack contained in a DTS .AUD file
    Parameters:
        fptr - A pointer to the .AUD file to be played
        timecode - A pointer to the shared memory containing timecode information to slave playback to
        current_serial - The intended serial number of the reel to playback
        current_reel - The intended reel number of the reel to playback
        pcm_handle - A pointer to the ALSA pcm device to utilize for playback
*/
void dts_reel_playback(FILE* fptr, dts_timecode *timecode, unsigned short current_serial, unsigned char current_reel, snd_pcm_t *pcm_handle);