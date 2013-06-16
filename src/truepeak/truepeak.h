// truepeak.h
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


#ifndef truepeak_truepeak_h
#define truepeak_truepeak_h

typedef struct {
    int truePeakChannels;
    int truePeakInSamplingRate;
    size_t truePeakFilterSize;
    double* coef0;
    double* coef1;
    double* coef2;
    double* coef3;
    double** ringBuffers;
    size_t ringBufferPos;
    double* maxTruePeaks;
} truePeakState;

int initTruePeakState(truePeakState* state, int channels, int inSamplingRate);
int addSamplesDouble(truePeakState* state, double* samples, size_t numberOfFrames);
int addSamplesFloat(truePeakState* state, float* samples, size_t numberOfFrames);
void destroyTruePeakState(truePeakState* state);
#endif
