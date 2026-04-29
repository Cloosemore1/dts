#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include "dts.h"
#include "libaptx100.h"

void get_dts_header(FILE* fptr, dts_header *header) {
    if (fread(header, sizeof(dts_header), 1, fptr) != 1) {
        fprintf(stderr, "Cannot read header from file");
        exit(-1);
    }
} //get_dts_header                          

int get_dts_frame(FILE* fptr, int channel, int frame, unsigned short *aptxBuf) {
    
    int offset = AUDIO_START + (frame * 7350) + (channel * 2);
    // if (offset % 2 != 0) {
    //     offset-=5;
    // }
    fseek(fptr, offset, SEEK_SET); //Start reading from the specified frame
    
    unsigned short current_sample[1];

    for (int i = 0; i < 735; i++) {
        fread(current_sample, 2, 1, fptr);
        aptxBuf[i] = current_sample[0];
        fseek(fptr, 8, SEEK_CUR);
    }

} //get_dts_frame

int main() {
    #pragma region Header and Setup
    struct dts_header header;
    char filename[] = "mnt/R6T5.AUD";

    FILE *fptr;
    fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Cannot open file");
        exit(-1);
    }

    get_dts_header(fptr, &header);

    //Print DTS file information
    printf("%s\n", header.dts_title);
    printf("SN - %d\n", header.dts_serial);
    printf("Reel #%d\n", header.dts_reel);
    #pragma endregion

    //Initializing APT-X100 decoder
    aptxCtx_t *aptxContextLeft;
    aptxContextLeft = aptxCreate(0, -1, 0, 1);
    aptxDecInit(aptxContextLeft, 1);

    aptxCtx_t *aptxContextRight;
    aptxContextRight = aptxCreate(0, -1, 0, 1);
    aptxDecInit(aptxContextRight, 1);

    //Setting up encoded (APT-X100) and decoded (PCM) buffers
    short pcmBufLeft[2940];
    short pcmBufRight[2940];
    unsigned short aptxBufLeft[735];
    unsigned short aptxBufRight[735];

    short outputBuf[5880];

    dts_channel channel_left = LEFT;
    dts_channel channel_right = CENTER;

    //Setting up sound library
    snd_pcm_t *handle;
    int err;

    //Open PCM device for playback
    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        return 1;
    }

    //Set hardware parameters: S16_LE, Interleaved, 2 channels, 44100Hz, soft-resample, 500ms latency
    if ((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 1, 50000)) < 0) {
        fprintf(stderr, "Setting parameters error: %s\n", snd_strerror(err));
        return 1;
    }

    int minute = 0;
    int second = 0;
    int frame = 0;
    //main decoding loop
    for (int i = 7500; i < 13900; i++) { //iterate over the file frame by frame
        //Fetching encoded samples from the DTS file
        get_dts_frame(fptr, channel_left, i, aptxBufLeft);
        get_dts_frame(fptr, channel_right, i, aptxBufRight);

        //Decoding the fetched samples
        aptxDecode(aptxContextLeft, 0, 2940, 0, pcmBufLeft, 1, aptxBufLeft);
        aptxDecode(aptxContextRight, 0, 2940, 0, pcmBufRight, 1, aptxBufRight);

        minute = i/900;
        second = (i%900)/15;
        frame = (i%15) * 2;
        printf("\r%02d:%02d:%02d", minute, second, frame);
        fflush(stdout);

        for (int j = 0; j < 5880; j++) {
            if (j % 2 == 0) {
                outputBuf[j] = pcmBufLeft[j/2];
            } else {
                outputBuf[j] = pcmBufRight[j/2];
            }
        }

        snd_pcm_writei(handle, outputBuf, 2940);

    }

    // Close the DTS file
    fclose(fptr);

    snd_pcm_drain(handle);
    snd_pcm_close(handle);

    return 0;
}