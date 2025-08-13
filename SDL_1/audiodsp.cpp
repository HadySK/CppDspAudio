#include <SDL.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include "audiodsp.h"
#include <fstream>
#include <cmath>

// Function to write a WAV file from a buffer
bool saveWavFile(const char* filename, const std::vector<int16_t>& samples, const SDL_AudioSpec& spec) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return false;
    }

    // Calculate sizes
    uint32_t numSamples = samples.size();
    uint32_t dataSize = numSamples * sizeof(int16_t); // Size of audio data in bytes
    uint32_t fileSize = 36 + dataSize; // Total file size (RIFF header + fmt + data)
    uint32_t byteRate = spec.freq * spec.channels * (spec.format == AUDIO_S16LSB ? 2 : 1);
    uint16_t blockAlign = spec.channels * (spec.format == AUDIO_S16LSB ? 2 : 1);
    uint16_t bitsPerSample = (spec.format == AUDIO_S16LSB ? 16 : 8);

    // Write RIFF header
    outFile.write("RIFF", 4);
    outFile.write(reinterpret_cast<const char*>(&fileSize), 4);
    outFile.write("WAVE", 4);

    // Write fmt chunk
    outFile.write("fmt ", 4);
    uint32_t fmtSize = 16; // PCM format chunk size
    outFile.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t audioFormat = 1; // PCM
    outFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    outFile.write(reinterpret_cast<const char*>(&spec.channels), 2);
    outFile.write(reinterpret_cast<const char*>(&spec.freq), 4);
    outFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    outFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    outFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // Write data chunk
    outFile.write("data", 4);
    outFile.write(reinterpret_cast<const char*>(&dataSize), 4);
    outFile.write(reinterpret_cast<const char*>(samples.data()), dataSize);

    outFile.close();
    return true;
}
int sdlAudioSetup() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Load WAV file
    if (SDL_LoadWAV("testMono.wav", &audioF.wavSpec, &audioF.wavBuffer, &audioF.wavLength) == NULL) {
        std::cerr << "Failed to load WAV file: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Check for 16-bit format and mono or stereo
    if (audioF.wavSpec.format != AUDIO_S16LSB || (audioF.wavSpec.channels != 1 && audioF.wavSpec.channels != 2)) {
        std::cerr << "Unsupported audio format. Please use 16-bit mono or stereo WAV." << std::endl;
        SDL_FreeWAV(audioF.wavBuffer);
        SDL_Quit();
        return 1;
    }
}

int audioEcho(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {

    // Echo effect parameters
    const float GAIN = 0.5f;
    const float delayTime = 0.5f; // seconds
    int DELAY_BUF_SIZE = static_cast<int>(delayTime * audioF.wavSpec.freq);
    std::cout << "number of channels = " << static_cast<int16_t>(audioF.wavSpec.channels) << '\n';
    if (audioF.wavSpec.channels == 1) {
        // Mono processing
        std::vector<int16_t> delayBuffer(DELAY_BUF_SIZE, 0);
        int bufptr = 0;
        for (int i = 0; i < numSamples; ++i) {
            int16_t delayedSample = delayBuffer[bufptr];
            float delayedF = static_cast<float>(delayedSample);
            float inputF = static_cast<float>(inputSamples[i]);
            float outputF = inputF + delayedF;
            outputF = std::max(std::min(outputF, 32767.0f), -32768.0f);
            (*outputSamples)[i] = static_cast<int16_t>(outputF);
            float feedback = inputF + delayedF * GAIN;
            feedback = std::max(std::min(feedback, 32767.0f), -32768.0f);
            delayBuffer[bufptr] = static_cast<int16_t>(feedback);
            bufptr = (bufptr + 1) % DELAY_BUF_SIZE;
        }
    } else if (audioF.wavSpec.channels == 2) {
        // Stereo processing
        std::vector<int16_t> delayBufferL(DELAY_BUF_SIZE, 0);
        std::vector<int16_t> delayBufferR(DELAY_BUF_SIZE, 0);
        int bufptr = 0;
        for (int i = 0; i < numSamples; i += 2) {
            int16_t delayedL = delayBufferL[bufptr];
            int16_t delayedR = delayBufferR[bufptr];
            float inputL = static_cast<float>(inputSamples[i]);
            float inputR = static_cast<float>(inputSamples[i + 1]);
            float outputL = inputL + delayedL;
            float outputR = inputR ;
            outputL = std::max(std::min(outputL, 32767.0f), -32768.0f);
            outputR = std::max(std::min(outputR, 32767.0f), -32768.0f);
            (*outputSamples)[i] = static_cast<int16_t>(outputL);
            (*outputSamples)[i + 1] = static_cast<int16_t>(outputR);
            float feedbackL = inputL + delayedL * GAIN;
            float feedbackR = inputR ;
            feedbackL = std::max(std::min(feedbackL, 32767.0f), -32768.0f);
            feedbackR = std::max(std::min(feedbackR, 32767.0f), -32768.0f);
            delayBufferL[bufptr] = static_cast<int16_t>(feedbackL);
            delayBufferR[bufptr] = static_cast<int16_t>(feedbackR);
            bufptr = (bufptr + 1) % DELAY_BUF_SIZE;
        }
    }

    return 0;
}

int audioDelay(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {

    int DELAY_BUF_SIZE = 24000; //500ms delay
    const float GAIN = 0.5f;
    std::cout << "number of channels = " << static_cast<int16_t>(audioF.wavSpec.channels) << '\n';
    if (audioF.wavSpec.channels == 1) {
        // Mono processing
        std::vector<int16_t> delayBuffer(DELAY_BUF_SIZE, 0);
        int bufptr = 0;
        for (int i = 0; i < numSamples; ++i) {
            int16_t delayedSample = delayBuffer[bufptr];
            float delayedF = static_cast<float>(delayedSample);
            float inputF = static_cast<float>(inputSamples[i]);
            float outputF = inputF + delayedF;
            outputF = std::max(std::min(outputF, 32767.0f), -32768.0f);
            (*outputSamples)[i] = static_cast<int16_t>(outputF);
            float feedback = inputF + delayedF * GAIN;
            feedback = std::max(std::min(feedback, 32767.0f), -32768.0f);
            delayBuffer[bufptr] = static_cast<int16_t>(feedback);
            bufptr = (bufptr + 1) % DELAY_BUF_SIZE;
        }
    } else if (audioF.wavSpec.channels == 2) {
        // Stereo processing
        std::vector<int16_t> delayBufferL(DELAY_BUF_SIZE, 0);
        int bufptr = 0;
        for (int i = 0; i < numSamples; i += 2) {
            int16_t delayedL = delayBufferL[bufptr];
            float inputL = static_cast<float>(inputSamples[i]);

            float inputR = static_cast<float>(inputSamples[i + 1]);
            float outputL = inputL + delayedL;

            //delayBufferL[bufptr] = inputL;
            //uncomment for echo effect
            delayBufferL[bufptr] = inputL+ delayedL*0.5f;
            float outputR = inputR ;
            outputL = std::max(std::min(outputL, 32767.0f), -32768.0f);
            outputR = std::max(std::min(outputR, 32767.0f), -32768.0f);
            (*outputSamples)[i] = static_cast<int16_t>(outputL);
            (*outputSamples)[i + 1] = static_cast<int16_t>(outputR);
            bufptr = (bufptr + 1) % DELAY_BUF_SIZE;
#ifdef DEBUG_DSP
            if ((*outputSamples)[i] > 2) {

            std::cout << "# " << i << "  delayBufferL " << delayedL << '\n';
            std::cout << "# " << i << "  inputL " << inputL << '\n';
            std::cout << "# " << i << "  outputL " << outputL << '\n';
            std::cout << "# " << i << "  outputR " << outputR << '\n';
            }
#endif
        }
    }

    return 0;
}


//sine_table[i] = 10000sin(2*pi*i/8)
//int16_t sine_table[LOOP_LENGTH] = {0, 7071, 10000, 7071, 0, -7071, -10000, -7071};
//int16_t sine_table[LOOP_LENGTH] = {10000, 10000, 10000, 10000, -10000, -10000, -10000, -10000};
int16_t square_wave[SQUARE_LENGTH] = {
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000,
    -10000, -10000, -10000, -10000};

int16_t sine_ptr = 0;  // pointer into lookup table

int createSinWave(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {

    //audioF.wavSpec.freq = 8000; // 8000  because 8 elements in sine_table
    audioF.wavSpec.freq = 16000; // use with  outputL= 10000*sin(2*PI*(cntrSample*i)/8);

    std::cout << "number of channels = " << static_cast<int16_t>(audioF.wavSpec.channels) << '\n';
  
    // Stereo processing
    int bufptr = 0;
    float outputL = 0.0f;
    float sampleMult = 0.25; // added to increase sampling rate
    for (int i = 0; i < numSamples; i += 2) {

        //outputL = sine_table[sine_ptr];
        outputL = 10000*sin(2*PI*(sampleMult*i)/8);
        

        float outputR = 0;
        outputL = std::max(std::min(outputL, 32767.0f), -32768.0f);
        outputR = std::max(std::min(outputR, 32767.0f), -32768.0f);
        (*outputSamples)[i] = static_cast<int16_t>(outputL);
        (*outputSamples)[i + 1] = static_cast<int16_t>(outputR);
        sine_ptr = (sine_ptr + 1) % LOOP_LENGTH;
    }

    return 0;
}

int createSquareWave(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {

    audioF.wavSpec.freq = 8000; // 8000  because 8 elements in sine_table
    // Stereo processing
    int bufptr = 0;
    float outputL = 0.0f;
    float sampleMult = 0.25; // added to increase sampling rate
    for (int i = 0; i < numSamples; i += 2) {

        outputL = square_wave[sine_ptr];


        float outputR = 0;
        outputL = std::max(std::min(outputL, 32767.0f), -32768.0f);
        outputR = std::max(std::min(outputR, 32767.0f), -32768.0f);
        (*outputSamples)[i] = static_cast<int16_t>(outputL);
        (*outputSamples)[i + 1] = static_cast<int16_t>(outputR);
        //std::cout << "outputL = " << static_cast<int16_t>(outputL) << '\n';

        sine_ptr = (sine_ptr + 1) % SQUARE_LENGTH;
    }
  
    return 0;
}

int movingAvgFilter(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {

    // Echo effect parameters
    
    const float GAIN = 0.5f;
    const float delayTime = 0.5f; // seconds
    int DELAY_BUF_SIZE = static_cast<int>(delayTime * audioF.wavSpec.freq);
    std::cout << "number of channels = " << static_cast<int16_t>(audioF.wavSpec.channels) << '\n';
    if (audioF.wavSpec.channels == 1) {
        // Mono processing
        std::vector<int16_t> delayBuffer(DELAY_BUF_SIZE, 0);
        int bufptr = 0;
        for (int i = 0; i < numSamples; ++i) {
            int16_t delayedSample = delayBuffer[bufptr];
            float delayedF = static_cast<float>(delayedSample);
            float inputF = static_cast<float>(inputSamples[i]);
            float outputF = inputF + delayedF;
            outputF = std::max(std::min(outputF, 32767.0f), -32768.0f);
            (*outputSamples)[i] = static_cast<int16_t>(outputF);

            if (i >= 4) {
                (*outputSamples)[i - 4] = ((*outputSamples)[i] +(*outputSamples)[i-1] +(*outputSamples)[i-2] \
                    + (*outputSamples)[i-3] + (*outputSamples)[i-4])/5;
            }
            /*if (i >= 4) {
                float sum = 0.0f;
                float weights[5] = {0.4f, 0.3f, 0.2f, 0.1f, 0.08f}; // Weighted coefficients for low-pass
                sum += weights[0] * (*outputSamples)[i];
                sum += weights[1] * (*outputSamples)[i-1];
                sum += weights[2] * (*outputSamples)[i-2];
                sum += weights[3] * (*outputSamples)[i-3];
                sum += weights[4] * (*outputSamples)[i-4];
                (*outputSamples)[i - 4] = static_cast<int16_t>(sum); // Normalize sum
            }*/

            float feedback = inputF + delayedF * GAIN;
            feedback = std::max(std::min(feedback, 32767.0f), -32768.0f);
            delayBuffer[bufptr] = static_cast<int16_t>(feedback);
            bufptr = (bufptr + 1) % DELAY_BUF_SIZE;
        }
    } 
    

    return 0;
}

int movingAvgFNoEcho2(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {


    std::cout << "number of channels = " << static_cast<int16_t>(audioF.wavSpec.channels) << '\n';
    
        for (int i = 0; i < numSamples; ++i) {

            float inputF = static_cast<float>(inputSamples[i]);
            //float outputF = inputF + delayedF;
            float outputF = inputF ; //remove delay
            outputF = std::max(std::min(outputF, 32767.0f), -32768.0f);
            (*outputSamples)[i] = static_cast<int16_t>(outputF);

            if (i >= 49) {
                float sum = 0.0f;
                //we can use weighted average to attenuate more of the higher frequencies
                // Adjusted weights with emphasis on middle samples
                //without weighted average, increasing the number of samples will cutoff more lower frequencies
                float weights[50] = {
                    0.01f, 0.01f, 0.01f, 0.01f, 0.02f, 0.02f, 0.03f, 0.03f, 0.04f, 0.05f,
                    0.06f, 0.07f, 0.08f, 0.09f, 0.10f, 0.10f, 0.10f, 0.09f, 0.08f, 0.07f,
                    0.06f, 0.05f, 0.04f, 0.03f, 0.03f, 0.02f, 0.02f, 0.01f, 0.01f, 0.01f,
                    0.01f, 0.01f, 0.01f, 0.01f, 0.02f, 0.02f, 0.03f, 0.03f, 0.04f, 0.05f,
                    0.06f, 0.07f, 0.08f, 0.09f, 0.10f, 0.10f, 0.10f, 0.09f, 0.08f, 0.07f
                }; 
                sum += (*outputSamples)[i];
                sum += (*outputSamples)[i-1];
                sum += (*outputSamples)[i-2];
                sum += (*outputSamples)[i-3];
                sum += (*outputSamples)[i-4];
                sum += (*outputSamples)[i-5];
                sum += (*outputSamples)[i-6];
                sum += (*outputSamples)[i-7];
                sum += (*outputSamples)[i-8];
                sum += (*outputSamples)[i-9];
                sum += (*outputSamples)[i-10];
                sum += (*outputSamples)[i-11];
                sum += (*outputSamples)[i-12];
                sum += (*outputSamples)[i-13];
                sum += (*outputSamples)[i-14];
                sum += (*outputSamples)[i-15];
                sum += (*outputSamples)[i-16];
                sum += (*outputSamples)[i-17];
                sum += (*outputSamples)[i-18];
                sum += (*outputSamples)[i-19];
                sum += (*outputSamples)[i-20];
                sum += (*outputSamples)[i-21];
                sum += (*outputSamples)[i-22];
                sum += (*outputSamples)[i-23];
                sum += (*outputSamples)[i-24];
                sum += (*outputSamples)[i-25];
                sum += (*outputSamples)[i-26];
                sum += (*outputSamples)[i-27];
                sum += (*outputSamples)[i-28];
                sum += (*outputSamples)[i-29];
                sum += (*outputSamples)[i-30];
                sum += (*outputSamples)[i-31];
                sum += (*outputSamples)[i-32];
                sum += (*outputSamples)[i-33];
                sum += (*outputSamples)[i-34];
                sum += (*outputSamples)[i-35];
                sum += (*outputSamples)[i-36];
                sum += (*outputSamples)[i-37];
                sum += (*outputSamples)[i-38];
                sum += (*outputSamples)[i-39];
                sum += (*outputSamples)[i-40];
                sum += (*outputSamples)[i-41];
                sum += (*outputSamples)[i-42];
                sum += (*outputSamples)[i-43];
                sum += (*outputSamples)[i-44];
                sum += (*outputSamples)[i-45];
                sum += (*outputSamples)[i-46];
                sum += (*outputSamples)[i-47];
                sum += (*outputSamples)[i-48];
                sum += (*outputSamples)[i-49];
                (*outputSamples)[i - 49] = static_cast<int16_t>(sum/25 ); // Normalize sum
            }

        }
    return 0;
}

int IirFNoEcho2(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) 
{

    std::cout << "number of channels = " << static_cast<int16_t>(audioF.wavSpec.channels) << '\n';

    // Second-order Butterworth high-pass filter parameters
    float fs = static_cast<float>(audioF.wavSpec.freq); // Sample rate from audio spec
    float fc = 5000.0f; // Cutoff frequency in Hz (5 kHz)
    float pi = acos(-1.0f); // Pi constant
    float w0 = 2.0f * pi * fc / fs; // Angular frequency
    float alpha = sin(w0) / 2.0f; // For Butterworth, adjusted for digital domain
    float cos_w0 = cos(w0);
    float denom = 1.0f + alpha;
    float b0 = (1.0f + cos_w0) / (2.0f * denom);
    float b1 = -(1.0f + cos_w0) / denom;
    float b2 = b0;
    float a1 = -2.0f * cos_w0 / denom;
    float a2 = (1.0f - alpha) / denom;

    // Initialize previous inputs and outputs for second-order IIR filter
    float x_prev1 = 0.0f; // Previous input sample 1
    float x_prev2 = 0.0f; // Previous input sample 2
    float y_prev1 = 0.0f; // Previous output sample 1
    float y_prev2 = 0.0f; // Previous output sample 2

    for (int i = 0; i < numSamples; ++i) {
        // Convert input sample to float
        float x = static_cast<float>(inputSamples[i]);

        // Apply second-order Butterworth high-pass filter: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
        float y = b0 * x + b1 * x_prev1 + b2 * x_prev2 - a1 * y_prev1 - a2 * y_prev2;

        // Clip output to 16-bit range
        y = std::max(std::min(y, 32767.0f), -32768.0f);
        (*outputSamples)[i] = static_cast<int16_t>(y);

        // Update previous samples
        x_prev2 = x_prev1;
        x_prev1 = x;
        y_prev2 = y_prev1;
        y_prev1 = y;
    }

    return 0;

}
int IirFNoEcho4(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {
    // Fourth-order Butterworth high-pass filter parameters
    float fs = static_cast<float>(audioF.wavSpec.freq); // Sample rate from audio spec
    float fc = 5000.0f; // Cutoff frequency in Hz (5 kHz)
    float pi = acos(-1.0f); // Pi constant
    float w0 = 2.0f * pi * fc / fs; // Angular frequency
    float alpha = sin(w0) / 2.0f; // For Butterworth, adjusted for digital domain
    float cos_w0 = cos(w0);

    // First second-order section
    float denom1 = 1.0f + alpha;
    float b0_1 = (1.0f + cos_w0) / (2.0f * denom1);
    float b1_1 = -(1.0f + cos_w0) / denom1;
    float b2_1 = b0_1;
    float a1_1 = -2.0f * cos_w0 / denom1;
    float a2_1 = (1.0f - alpha) / denom1;

    // Second second-order section (same coefficients for Butterworth symmetry)
    float b0_2 = b0_1;
    float b1_2 = b1_1;
    float b2_2 = b2_1;
    float a1_2 = a1_1;
    float a2_2 = a2_1;

    // Initialize previous inputs and outputs for fourth-order IIR filter
    float x_prev1_1 = 0.0f, x_prev2_1 = 0.0f; // First section previous inputs
    float y_prev1_1 = 0.0f, y_prev2_1 = 0.0f; // First section previous outputs
    float x_prev1_2 = 0.0f, x_prev2_2 = 0.0f; // Second section previous inputs
    float y_prev1_2 = 0.0f, y_prev2_2 = 0.0f; // Second section previous outputs

    for (int i = 0; i < numSamples; ++i) {
        // Convert input sample to float
        float x = static_cast<float>(inputSamples[i]);

        // First second-order section
        float y1 = b0_1 * x + b1_1 * x_prev1_1 + b2_1 * x_prev2_1 - a1_1 * y_prev1_1 - a2_1 * y_prev2_1;

        // Second second-order section
        float y = b0_2 * y1 + b1_2 * x_prev1_2 + b2_2 * x_prev2_2 - a1_2 * y_prev1_2 - a2_2 * y_prev2_2;

        // Clip output to 16-bit range
        y = std::max(std::min(y, 32767.0f), -32768.0f);
        (*outputSamples)[i] = static_cast<int16_t>(y)*10;

        // Update previous samples for first section
        x_prev2_1 = x_prev1_1;
        x_prev1_1 = x;
        y_prev2_1 = y_prev1_1;
        y_prev1_1 = y1;

        // Update previous samples for second section
        x_prev2_2 = x_prev1_2;
        x_prev1_2 = y1;
        y_prev2_2 = y_prev1_2;
        y_prev1_2 = y;
    }
return 0;
}


//Second-order, type 1 Chebyshev, low pass filter with 2 dB of passband ripple
// and a cutoff frequency of 1500 Hz (9425 rad/s). 
int IirFCheby(int numSamples, int16_t* inputSamples, std::vector<int16_t>* outputSamples)
{

    float xn1 = 0.0f;
    float yn1 = 0.0f;
    float yn2 = 0.0f;
    for (int i = 2; i < numSamples; ++i) {
        // Convert input sample to float
        float xn = static_cast<float>(inputSamples[i]);
        float yn = 0.48255 * xn1 
            + 0.71624315 * yn1 
            - 0.38791310 * yn2;
        // Clip output to 16-bit range
        yn = std::max(std::min(yn, 32767.0f), -32768.0f);
        (*outputSamples)[i] = static_cast<int16_t>(yn);
        yn2 = yn1;
        yn1 = yn;
        xn1 = xn;

    }

    return 0;

}


float b[NUM_SECTIONS][3] = { 
    {5.54030145E-01, 4.35886813E-01, 5.54030145E-01},
    {1.81411681E-02, -1.40939730E-02, 1.81411681E-02} };

float a[NUM_SECTIONS][3] = { 
    {1.00000000E+00, -1.52873063E+00, 6.37029970E-01},
    {1.00000000E+00, -1.51375766E+00, 8.68678806E-01} };

float w[NUM_SECTIONS][2] = { 0 };

int IirFElliptic(int numSamples, int16_t* inputSamples, std::vector<int16_t>* outputSamples)
{

    float xn1 = 0.0f;
    float yn1 = 0.0f;
    float yn2 = 0.0f;

    int16_t section;    // second order section number
    float input;    // input to each section
    float wn, yn;   // intermediate and output values

   
    for (int i = 2; i < numSamples; ++i) {
        // Convert input sample to float
        float xn = static_cast<float>(inputSamples[i]);
        for (section=0 ; section< NUM_SECTIONS ; section++)
        {
            wn = xn - a[section][1]*w[section][0]
                - a[section][2]*w[section][1];
            yn = b[section][0]*wn + b[section][1]*w[section][0]
                + b[section][2]*w[section][1];
            w[section][1] = w[section][0];
            w[section][0] = wn;
            input = yn; 
        }

        // Clip output to 16-bit range
        yn = std::max(std::min(yn, 32767.0f), -32768.0f);
        (*outputSamples)[i] = static_cast<int16_t>(yn) * 20; // multiple by 20 to restore audio level

    }

    return 0;

}

COMPLEX samples[N];
int DFT(int numSamples, int16_t* inputSamples, std::vector<int16_t> * outputSamples) {
    audioF.wavSpec.channels = 2;
    audioF.wavSpec.freq = 8000;

    float outputL;
    float outputR;
    for(int n=0 ; n<N ; n++)
    {
        samples[n].real = cos(2*PI*TESTFREQ*n/SAMPLING_FREQ);
        samples[n].imag = 0.0f;	
    }

   COMPLEX result[N];
        for (int k = 0; k < N; k++) {
            //initialize all real and imaginary values with 0
            result[k].real = 0.0;
            result[k].imag = 0.0;

            for (int n = 0; n < N; n++)
            {
                result[k].real += samples[n].real * cos(2 * PI * k * n / N)
                                + samples[n].imag * sin(2 * PI * k * n / N);
                result[k].imag += samples[n].imag * cos(2 * PI * k * n / N)
                                - samples[n].real * sin(2 * PI * k * n / N);
            }
        }
        // copy the result to the output buffer
        for (int k = 0; k < N; k++) {
          // std::cout << k << " real part " << result[k].real << '\n';
           //std::cout << k <<" imag part "<< result[k].imag*10000000000.0f << '\n';
            outputL = std::max(std::min(result[k].real, 32767.0f), -32768.0f);
            outputR = std::max(std::min(result[k].imag, 32767.0f), -32768.0f);
            (*outputSamples)[k] = static_cast<int16_t>(outputL*1000.0f); // multiplied for scaling
            (*outputSamples)[k+1] = static_cast<int16_t>(result[k].imag);// multiplied for scaling
        }  

    return 0;
}

int main(int argc, char* argv[]) {
    int xc = 0;
    sdlAudioSetup();
    // Calculate number of samples (total int16_t samples)
    int numSamples = audioF.wavLength / 2; //16 bit audio, 2 bytes per sample
    int16_t* inputSamples = (int16_t*)audioF.wavBuffer;
    //std::vector<int16_t> outputSamples(numSamples);
    //only for DFT
    std::vector<int16_t> outputSamples(N+1);
    while ((xc != 1) && (xc != 2) && (xc != 3) && (xc != 4) && (xc != 5) && (xc != 10)) {

        std::cout << "Enter a number to select an option" << '\n';
        std::cout << "1. Apply Echo to the pre-recorded Audio" << '\n';
        std::cout << "2. Generate 1 kHz Sine wave" << '\n';
        std::cout << "3. Generate 125 Hz Square wave" << '\n';
        std::cout << "4. Apply Moving Average Filter to the pre-recorded audio" << '\n';
        std::cout << "5. Apply 2nd order Chebyshev low pass filter to the pre-recorded audio" << '\n';
        std::cout << "6. Apply 4th order Elliptic low pass filter to the pre-recorded audio" << '\n';
        std::cin >> xc;
        switch(xc) {
        case 1:
            audioEcho(numSamples, inputSamples, &outputSamples);
            break;
        case 2:

            createSinWave(numSamples, inputSamples, &outputSamples);
            break;
        case 3:

            createSquareWave(numSamples, inputSamples, &outputSamples);
            break;
        case 4:

            movingAvgFNoEcho2(numSamples, inputSamples, &outputSamples);
        case 5:

            IirFCheby(numSamples, inputSamples, &outputSamples);
        case 6:

            IirFElliptic(numSamples, inputSamples, &outputSamples);
        case 10:

            DFT(numSamples, inputSamples, &outputSamples);
            break;
        default:
            std::cout << "incorrect option !" << '\n';
        }
        std::cout << "xc = "<< xc << '\n';
    }

    char saveF = false;
    std::cout << "do you want to save the output file ? enter y to save / n to cancel " << '\n';
    std::cin >> saveF;
    if (saveF == 'y' || saveF == 'Y') {
        saveWavFile("output.wav", outputSamples, audioF.wavSpec);
    }
    saveF = false;
    std::cout << "do you want to play the audio ? enter y to play audio / n to cancel  " << '\n';
    std::cin >> saveF;
    if (saveF == 'y' || saveF == 'Y') {
        
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &audioF.wavSpec, NULL, 0);
    if (dev == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(audioF.wavBuffer);
        SDL_Quit();
        return 1;
    }

    // Start playback and queue audio
    SDL_PauseAudioDevice(dev, 0);
    SDL_QueueAudio(dev, outputSamples.data(), numSamples * sizeof(int16_t));

    // Wait for playback to complete
    while (SDL_GetQueuedAudioSize(dev) > 0) {
        SDL_Delay(100);
    }

    // Clean up
    SDL_CloseAudioDevice(dev);
    }
    SDL_FreeWAV(audioF.wavBuffer);
    SDL_Quit();
    return 0;
}