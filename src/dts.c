#include <stdio.h>
#include <stdlib.h>

#include "dts.h"
#include "libaptx100.h"

void get_dts_header(FILE* fptr, dts_header *header) {
    if (fread(header, sizeof(dts_header), 1, fptr) != 1) {
        fprintf(stderr, "Cannot read header from file");
        exit(-1);
    }
} //get_dts_header                          

void get_dts_frame(FILE* fptr, int channel, int frame, unsigned short *aptxBuf) {
    
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

    //Initializing APT-X100 decoder
    aptxCtx_t *aptxContext;
    aptxContext = aptxCreate(0, -1, 0, 1);
    aptxDecInit(aptxContext, 1);

    //Setting up encoded (APT-X100) and decoded (PCM) buffers
    short pcmBuf[2940];
    unsigned short aptxBuf[735];

    //Setting up output file for PCM data
    FILE *output;

    // Open a file in append binary mode
    output = fopen("/home/connor-loosemore/sambashare/test_output.pcm", "ab");
    if (output == NULL) {
        perror("Error opening file");
        return 1;
    }

    dts_channel channel = LEFT;

    //main decoding loop
    for (int i = 0; i < 13905; i++) { //iterate over the file frame by frame
        //Fetching encoded samples from the DTS file
        get_dts_frame(fptr, channel, i, aptxBuf);

        //Decoding the fetched samples
        aptxDecode(aptxContext, 0, 2940, 0, pcmBuf, 1, aptxBuf);

        // Write the decoded frame to the file
        fwrite(pcmBuf, sizeof(short), 2940, output);
    }

    // Close the output file
    fclose(output);
    // Close the DTS file
    fclose(fptr);

    return 0;
}