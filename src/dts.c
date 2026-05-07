#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

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
    char filename[] = "R6T5.AUD";

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

    //Setting up aptx decoders and audio buffers for each channel
    int channels = 5;
    aptxCtx_t *aptxContexts[channels];
    short *pcmBuf[channels];
    unsigned short *aptxBuf[channels];
    short outputBuf[2940 * channels];

    for (int i = 0; i < channels; i++) {
        aptxContexts[i] = aptxCreate(0, -1, 0, 1);
        aptxDecInit(aptxContexts[i], 1);
        pcmBuf[i] = malloc(sizeof(short) * 2940);
        aptxBuf[i] = malloc(sizeof(unsigned short) * 735);
    }

    //Setting up sound library
    snd_pcm_t *handle;
    int err;

    //Open PCM device for playback
    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        return 1;
    }

    //Set hardware parameters: sample format, interleaved?, channels, sample rate [hz], soft-resample?, latency(us)
    if ((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, 44100, 1, 500000)) < 0) {
        fprintf(stderr, "Setting parameters error: %s\n", snd_strerror(err));
        return 1;
    }

    int minute = 0;
    int second = 0;
    int curr_frame = 0;
    //main decoding loop
    for (int frame = 7500; frame < 18900; frame++) { //iterate over the file frame by frame
        for (int channel = 0; channel < channels; channel++) {
            get_dts_frame(fptr, channel, frame, aptxBuf[channel]);
            aptxDecode(aptxContexts[channel], 0, 2940, 0, pcmBuf[channel], 1, aptxBuf[channel]);
        }

        minute = frame/900;
        second = (frame%900)/15;
        curr_frame = (frame%15) * 2;
        printf("\r%02d:%02d:%02d", minute, second, curr_frame);
        fflush(stdout);

        //combining all the samples
        for (int j = 0; j < (2940 * channels); j++) {
            outputBuf[j] = pcmBuf[j%channels][j/channels];
            //outputBuf[j] = *(*(pcmBuf + (j%channels) + (j/channels)));
        }

        snd_pcm_writei(handle, outputBuf, 2940);
    }

    // Close the DTS file
    fclose(fptr);

    snd_pcm_drain(handle);
    snd_pcm_close(handle);

    return 0;
}