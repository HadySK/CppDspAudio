#ifndef _AUDIODSP_H__
#define _AUDIODSP_H__


struct waveFileData{
    SDL_AudioSpec wavSpec;
    Uint8* wavBuffer; //waveStart
    Uint32 wavLength;
};

#define PI  3.14159265358979323846

waveFileData audioF;

#define LOOP_LENGTH 8
#define SQUARE_LENGTH 64

#define NUM_SECTIONS 2

// used for DFT
typedef struct
{
    float real;
    float imag;
} COMPLEX;

//#define N 128
#define N 4096
//#define TESTFREQ 800.0f
//to prevent spectral leakage in imaginary part
#define TESTFREQ 800.78125f
#define SAMPLING_FREQ 8000.0f



int audioDelay(int numSamples, int16_t* inputSamples, std::vector<int16_t>* outputSamples);
int audioEcho(int numSamples, int16_t* inputSamples, std::vector<int16_t>* outputSamples);
bool saveWavFile(const char* filename, const std::vector<int16_t>& samples, const SDL_AudioSpec& spec);


#endif /*_AUDIODSP_H__*/