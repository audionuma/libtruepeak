// truepeak.c
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

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#include "truepeak.h"
/*
itu BS.1770-3 coefs for 48k input signal
*/
static double COEF_0_48k[12] = {0.0017089843750, 0.0109863281250, -0.0196533203125, 0.0332031250000, -0.0594482421875, 0.1373291015625, 0.9721679687500, -0.1022949218750, 0.0476074218750, -0.0266113281250, 0.0148925781250, -0.0083007812500};
static double COEF_1_48k[12] = {-0.0291748046875, 0.0292968750000, -0.0517578125000, 0.0891113281250, -0.1665039062500, 0.4650878906250, 0.7797851562500, -0.2003173828125, 0.1015625000000, -0.0582275390625, 0.0330810546875, -0.0189208984375};

/*
coefs computed for 44100 kHz input signal with scilab trying to be close to the itu coefs. NO GUARANTEE.
x1 = (22050 - 4000)/176200
x2 = (22050 + 4000)/176200
[hn] = eqfir(48, [0 x1;x2 .5],[4 .05], [1 1]);
 */
static double COEF_0_44k[12] = {-0.01439987226416129112,0.00804428685766508424,-0.01987224313675403642,0.02961932948785491598,-0.05927231810554856040,0.13017896194978784141,0.99235970714512011792,-0.09710589955199680490,0.04833968399400932064,-0.02339943319366176444,0.01352769296977450984,-0.01180186154189016498};
static double COEF_1_44k[12] = {-0.00172904083567296252,0.02915878488169267729,-0.04609955571114128514,0.08616031626350771633,-0.15946140818094267644,0.46467030920006835437,0.75915840241856424875,-0.19886324660220641714,0.09586897108756886610,-0.05455391567180414153,0.02950325446961841319,-0.01885897116569820731};
/*
 init truePeakState
 The caller is responsible for allocating and freeing the truePeakState struct
 */
int initTruePeakState(truePeakState* state, int channels, int inSamplingRate) {
    int result = 0;
    int i;
    //to be done : all asserts to be replaced with proper error return and memory cleaning
    assert(state != NULL);
    assert(channels > 0);
    assert(inSamplingRate == 48000 || inSamplingRate == 44100);
    //number of channels in the interleaved buffers
    state->truePeakChannels = channels;
    //input signal sampling rate
    state->truePeakInSamplingRate = inSamplingRate;
    
    //maxTruePeaks will store the max true-peak for each channel
    state->maxTruePeaks = calloc(state->truePeakChannels, sizeof(double));
    assert(state->maxTruePeaks != NULL);
    
    //these four arrays will store the filter 4 phase coefs
    state->truePeakFilterSize = 12;
    state->coef0 = malloc(state->truePeakFilterSize * sizeof(double));
    assert(state->coef0 != NULL);
    state->coef1 = malloc(state->truePeakFilterSize * sizeof(double));
    assert(state->coef1 != NULL);
    state->coef2 = malloc(state->truePeakFilterSize * sizeof(double));
    assert(state->coef2 != NULL);
    state->coef3 = malloc(state->truePeakFilterSize * sizeof(double));
    assert(state->coef3 != NULL);
    
    if (state->truePeakInSamplingRate == 48000) {
        for (i = 0; i < state->truePeakFilterSize; i++) {
            state->coef0[i] = COEF_0_48k[i];
            state->coef1[i] = COEF_1_48k[i];
            state->coef2[i] = COEF_1_48k[state->truePeakFilterSize - 1 - i];
            state->coef3[i] = COEF_0_48k[state->truePeakFilterSize - 1 - i];
        }
    } else if (state->truePeakInSamplingRate == 44100) {
        for (i = 0; i < state->truePeakFilterSize; i++) {
            state->coef0[i] = COEF_0_44k[i];
            state->coef1[i] = COEF_1_44k[i];
            state->coef2[i] = COEF_1_44k[state->truePeakFilterSize - 1 - i];
            state->coef3[i] = COEF_0_44k[state->truePeakFilterSize - 1 - i];
        }
    }
    
    //we need one ring buffer per channel
    state->ringBuffers = malloc(state->truePeakChannels * sizeof(double*));
    assert(state->ringBuffers != NULL);
    //each ring buffer must store truePeakFilterSize values
    for (i = 0; i < state->truePeakChannels; i++) {
        state->ringBuffers[i] = calloc(state->truePeakFilterSize, sizeof(double));
        assert(state->ringBuffers[i] != NULL);
    }
    state->ringBufferPos = 0;
    return 0;
}

/*
 The size of the caller-provided double* samples must be state->truePeakChannels * sizeof(double) * numberOfFrames
 */
int addSamplesDouble(truePeakState* state, double* samples, size_t numberOfFrames) {
    assert(state != NULL);
    assert(samples != NULL);
    assert(numberOfFrames > 0);
    int result = 0;
    size_t frame;
    int channel, i;
    double* channelBuffer;
    double peak0, peak1, peak2, peak3;
    int channels = state->truePeakChannels;
    size_t filterSize = state->truePeakFilterSize;
    for (frame = 0; frame < numberOfFrames; frame++) {
        for (channel = 0; channel < channels; channel++) {
            channelBuffer = state->ringBuffers[channel];
            channelBuffer[state->ringBufferPos] = samples[frame * channels + channel];
            peak0 = 0.0;
            peak1 = 0.0;
            peak2 = 0.0;
            peak3 = 0.0;
            for (i = 0; i < filterSize; i++) {
                peak0 += state->coef0[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak1 += state->coef1[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak2 += state->coef2[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak3 += state->coef3[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
            }
            if (fabs(peak0) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak0);
            if (fabs(peak1) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak1);
            if (fabs(peak2) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak2);
            if (fabs(peak3) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak3);
        }
        state->ringBufferPos = (state->ringBufferPos + 1) % filterSize;
    }
    return 0;
}

/*
 The size of the caller-provided float* samples must be state->truePeakChannels * sizeof(float) * numberOfFrames
 */
int addSamplesFloat(truePeakState* state, float* samples, size_t numberOfFrames) {
    assert(state != NULL);
    assert(samples != NULL);
    assert(numberOfFrames > 0);
    int result = 0;
    size_t frame;
    int channel, i;
    double* channelBuffer;
    double peak0, peak1, peak2, peak3;
    int channels = state->truePeakChannels;
    size_t filterSize = state->truePeakFilterSize;
    for (frame = 0; frame < numberOfFrames; frame++) {
        for (channel = 0; channel < channels; channel++) {
            channelBuffer = state->ringBuffers[channel];
            channelBuffer[state->ringBufferPos] = (double)samples[frame * channels + channel];
            peak0 = 0.0;
            peak1 = 0.0;
            peak2 = 0.0;
            peak3 = 0.0;
            for (i = 0; i < filterSize; i++) {
                peak0 += state->coef0[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak1 += state->coef1[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak2 += state->coef2[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak3 += state->coef3[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
            }
            if (fabs(peak0) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak0);
            if (fabs(peak1) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak1);
            if (fabs(peak2) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak2);
            if (fabs(peak3) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak3);
        }
        state->ringBufferPos = (state->ringBufferPos + 1) % filterSize;
    }
    return 0;
}

/*
 The size of the caller-provided int* samples must be state->truePeakChannels * sizeof(int) * numberOfFrames
 */
int addSamplesInt(truePeakState* state, int* samples, size_t numberOfFrames) {
    assert(state != NULL);
    assert(samples != NULL);
    assert(numberOfFrames > 0);
    int result = 0;
    size_t frame;
    int channel, i;
    double* channelBuffer;
    double peak0, peak1, peak2, peak3;
    int channels = state->truePeakChannels;
    size_t filterSize = state->truePeakFilterSize;
    for (frame = 0; frame < numberOfFrames; frame++) {
        for (channel = 0; channel < channels; channel++) {
            channelBuffer = state->ringBuffers[channel];
            channelBuffer[state->ringBufferPos] = ((double)(samples[frame * channels + channel])) / INT_MAX;
            peak0 = 0.0;
            peak1 = 0.0;
            peak2 = 0.0;
            peak3 = 0.0;
            for (i = 0; i < filterSize; i++) {
                peak0 += state->coef0[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak1 += state->coef1[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak2 += state->coef2[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
                peak3 += state->coef3[i] * channelBuffer[(state->ringBufferPos - i + filterSize) % filterSize];
            }
            if (fabs(peak0) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak0);
            if (fabs(peak1) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak1);
            if (fabs(peak2) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak2);
            if (fabs(peak3) > state->maxTruePeaks[channel])
                state->maxTruePeaks[channel] = fabs(peak3);
        }
        state->ringBufferPos = (state->ringBufferPos + 1) % filterSize;
    }
    return 0;
}
/*
 to be called before freeing truePeakstate struct.
 */
void destroyTruePeakState(truePeakState* state) {
    int i;
    if (state->ringBuffers != NULL) {
        for (i = 0; i < state->truePeakChannels; i++) {
            if (state->ringBuffers[i] != NULL)
                free(state->ringBuffers[i]);
        }
        free(state->ringBuffers);
    }
    if (state->coef0 != NULL)
        free(state->coef0);
    if (state->coef1 != NULL)
        free(state->coef1);
    if (state->coef2 != NULL)
        free(state->coef2);
    if (state->coef3 != NULL)
        free(state->coef3);
    if (state->maxTruePeaks != NULL)
        free(state->maxTruePeaks);
}