// findtruepeak.c
// This file is part of libtruepeak.
//
// libtruepeak is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// libtruepeak is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libtruepeak.  If not, see <http://www.gnu.org/licenses/>.
// copyright Manuel Naudin 2013

/*
 a small utility for testing libtruepeak.
 Requires libsndfile
 */

#define FRAMES_PER_BUFFER 192000

#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <assert.h>
#include <math.h>
#include "truepeak/truepeak.h"

int main(int argc, const char * argv[])
{
    assert(argv[1] != NULL);
    const char *filePath = argv[1];
    printf("File : %s\n", filePath);
    SF_INFO *info = malloc(sizeof(SF_INFO));
    assert(info != NULL);
    SNDFILE *file = sf_open(filePath, SFM_READ, info);
    assert(file != NULL);
    sf_count_t totalFrames = info->frames;
    int samplingFrequency = info->samplerate;
    int nChannels = info->channels;
    printf("Channels : %d, Fs : %d, totalFrames : %lld\n", nChannels, samplingFrequency, totalFrames);
    sf_count_t framesRead = 0;
    sf_count_t totalFramesRead = 0;
    float *buffer = malloc(sizeof(float) * nChannels * FRAMES_PER_BUFFER);

    truePeakState *s = malloc(sizeof(truePeakState));
    assert(s != NULL);
    initTruePeakState(s, nChannels, samplingFrequency);
    
    while (totalFramesRead < totalFrames) {        
        framesRead = sf_readf_float(file, buffer, FRAMES_PER_BUFFER);
        totalFramesRead += framesRead;
        addSamplesFloat(s, buffer, framesRead);
    }
    int i;
    for (i = 0; i < nChannels; i++) {
        printf("Channel %d\t MaxTP : %f\n", i, 20*log10(s->maxTruePeaks[i]));
    }
    destroyTruePeakState(s);
    free(s);
    free(buffer);
    free(info);
    return 0;
}
