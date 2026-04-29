#define AUDIO_START 92

typedef enum dts_channel { LEFT, RIGHT, CENTER, LEFT_SURROUND, RIGHT_SURROUND } dts_channel;

//DTS .AUD and .AUE file header - 92 bytes long
typedef struct dts_header {
    unsigned char dts_title[60];
    unsigned char dts_lang[8];
    unsigned char dts_studio[6];
    unsigned char dts_unknown[4];
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
} dts_header;

/*
    Retrieves header information from a DTS .AUD or .AUE file
    Parameters:
        fptr - A pointer to the .AUD or .AUE file to be read
        header - A pointer to a dts_header
*/
void get_dts_header(FILE* fptr, dts_header *header);

/*
    Retrieves a given frame of audio information from a DTS .AUD or .AUE file
    Parameters:
        fptr - A pointer to the .AUD or .AUE file to be read
        channel - The dts_channel to be read from
        frame - The frame number to read
        aptxBuf - The buffer to write the encoded apt-X100 samples to
*/
int get_dts_frame(FILE* fptr, int channel, int frame, unsigned short *aptxBuf);