#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dtsplayback.h"

void get_dts_header(FILE* fptr, dts_header *header) {
    if (fread(header, sizeof(dts_header), 1, fptr) != 1) {
        fprintf(stderr, "Cannot read header from file");
        exit(-1);
    }
} //get_dts_header                          

void decode_dts_frame(FILE* fptr, int channel, int frame, short *pcmBuf, short *pcmContext, aptxCtx_t *aptxContext) {
    //Set offset to requested frame
    int offset = AUDIO_START + (frame * 3675) + (channel * 2);
     //if the frame number is divisible by two, read 368 samples
    int samples = 368;
    //if the frame number is not divisible by two, we need to take skip a "half" sample at the start and read 367 samples
    if (frame % 2 != 0) {
        offset += 5;
        samples = 367;
    }

    //Read aptX samples from the file starting at the specified offset
    fseek(fptr, offset, SEEK_SET);
    unsigned short current_sample[1];
    unsigned short *aptxBuf =  malloc(sizeof(unsigned short) * samples);
    for (int i = 0; i < samples; i++) {
        if (fread(current_sample, 2, 1, fptr) == 1) {
            aptxBuf[i] = current_sample[0];
            fseek(fptr, 8, SEEK_CUR);
        } else {
            printf("EOF");
            break; //Stop reading if EOF is reached
        }
    }

    //Decode
    short *pcmBufTemp = malloc(sizeof(short) * samples * 4);
    aptxDecode(aptxContext, 0, samples * 4, 0, pcmBufTemp, 1, aptxBuf);
    free(aptxBuf);
    //for frames divisible by two, the extra 2 decoded samples will be stored and added to the start of the next decoded frame
    if (samples == 368) {
        memcpy(pcmBuf, pcmBufTemp, samples * 8);
        pcmContext[0] = pcmBuf[1470];
        pcmContext[1] = pcmBuf[1471];
    } else {
        pcmBuf[0] = pcmContext[0];
        pcmBuf[1] = pcmContext[1];
        pcmBuf+=2;
        memcpy(pcmBuf, pcmBufTemp, samples * 8);
    }
} //get_dts_frame

void dts_reel_playback(FILE* fptr, dts_timecode *timecode, unsigned short current_serial, unsigned char current_reel, snd_pcm_t *pcm_handle) {
    //Prepare playback device
    snd_pcm_prepare(pcm_handle);
    //Get header information from soundtrack file
    struct dts_header header;
    get_dts_header(fptr, &header);
    if (header.dts_serial != current_serial | header.dts_reel != current_reel) {
        return; //If soundtrack file header information does not match expectations, break
    }
    //Other header information can be parsed here
    //Set channels - can evaluate serial number to see if DTS-ES (6.1) or DTS 5.1
    int channels = 5;

    //Setup aptx decoding buffers and context
    aptxCtx_t *aptxContexts[channels];
    short *pcmBuf[channels];
    short outputBuf[1470 * channels];
    short *pcmContexts[channels];

    for (int i = 0; i < channels; i++) {
        aptxContexts[i] = aptxCreate(0, -1, 0, 1);
        aptxDecInit(aptxContexts[i], 1);
        pcmBuf[i] = malloc(sizeof(short) * 1470);
        pcmContexts[i] = malloc(sizeof(short) * 2);
    }

    int frame;
    int prev_frame;
    //Validate that timecode is still being received for the current serial and reel
    while (timecode->serial == current_serial && timecode->reel == current_reel) {
        //playback the current frame of audio
        frame = timecode->frame;
        if (frame > prev_frame) {
            for (int channel = 0; channel < channels; channel++) {
                decode_dts_frame(fptr, channel, frame, pcmBuf[channel], pcmContexts[channel], aptxContexts[channel]);
            }
            //combining all the samples
            for (int j = 0; j < (1470 * channels); j++) {
                outputBuf[j] = pcmBuf[j%channels][j/channels];
            }
            snd_pcm_writei(pcm_handle, outputBuf, 1470);
        }
        prev_frame = frame;
    }
    //timecode reel/serial changed
    for (int i = 0; i < channels; i++) {
        aptxDelete(aptxContexts[i]);
        free(pcmBuf[i]);
        free(pcmContexts[i]);
    }
    return;
}

int main() {
    //Open shared memory to read incoming timecode
    int fd = shm_open("/timecode", O_RDONLY, 0666);
    struct dts_timecode *timecode = mmap(NULL, sizeof(dts_timecode), PROT_READ, MAP_SHARED, fd, 0);

    //Open PCM device for playback
    snd_pcm_t *pcm_handle;
    if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) {
        fprintf(stderr, "Error opening playback device\n");
        return -1;
    }
    //Set hardware parameters: sample format, interleaved?, channels, sample rate [hz], soft-resample?, latency(us)
    if (snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 5, 44100, 1, 500000) < 0) {
        fprintf(stderr, "Error setting playback parameters\n");
        return -1;
    }

    //Main playback loop
    while (true) {
        //Check if valid timecode is being received
        //If serial and reel are non-zero, attempt to open accompanying soundtrack file
        if (timecode->serial != 0 & timecode->reel != 0) {
            char filepath[30];
            snprintf(filepath, 30, "/content/aud/%d/R%dT5.AUD", timecode->serial, timecode->reel);
            FILE *fptr = fopen(filepath, "rb");
            if (fptr != NULL) { //successfully opened soundtrack file for reading
                //Playback current reel file slaved to timecode.
                //If timecode drops or reel/serial changes, playback function will return.
                dts_reel_playback(fptr, timecode, timecode->serial, timecode->reel, pcm_handle);
            } else {
                fprintf(stderr, "Cannot open soundtrack file at: %s\n", filepath);
            }
            //Close the AUD file
            fclose(fptr);
        }
    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    return 0;
}