//Background process to decode dts timecode from projector reader and pass timecode to main dts playback program
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "dtstimecode.h"

int main () {
    int fd = shm_open("/timecode", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(dts_timecode)); 
    struct dts_timecode *timecode = mmap(NULL, sizeof(dts_timecode), PROT_WRITE, MAP_SHARED, fd, 0);
    timecode->reel = 0;
    timecode->serial = 0;
    timecode->frame = 0;
    timecode->frame = 36562;
    timecode->reel = 6;
    timecode->serial = 65414;
    while(timecode->frame < 37912) {
        timecode->frame++;
        usleep(33333);
    }
    timecode->reel = 7;
    timecode->serial = 65414;
    timecode->frame = 95;
    while(true) {
        timecode->frame++;
        usleep(33333);
    }
} //main