#include <SDL.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include "audiodsp.h"
#include <fstream>
#include <cmath>
struct waveFileData{
    SDL_AudioSpec wavSpec;
    Uint8* wavBuffer; //waveStart
    Uint32 wavLength;
};

#define PI  3.14159265358979323846

waveFileData audioF;

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
    if (SDL_LoadWAV("testStereo.wav", &audioF.wavSpec, &audioF.wavBuffer, &audioF.wavLength) == NULL) {
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

#define LOOP_LENGTH 8
#define SQUARE_LENGTH 64
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

int main(int argc, char* argv[]) {
    int xc = 0;
    sdlAudioSetup();
    // Calculate number of samples (total int16_t samples)
    int numSamples = audioF.wavLength / 2; //16 bit audio, 2 bytes per sample
    int16_t* inputSamples = (int16_t*)audioF.wavBuffer;
    std::vector<int16_t> outputSamples(numSamples);

    while ((xc != 1) && (xc != 2) && (xc != 3)) {

        std::cout << "Enter a number to select an option" << '\n';
        std::cout << "1. Apply Echo to pre-recorded Audio" << '\n';
        std::cout << "2. Generate 1 kHz Sine wave" << '\n';
        std::cout << "3. Generate 125 Hz Square wave" << '\n';
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
        default:
            std::cout << "incorrect option !" << '\n';
        }
        std::cout << "xc = "<< xc << '\n';
    }
    //audioDelay(numSamples, inputSamples, &outputSamples);
   
    char saveF = false;
    std::cout << "do you want to save the output file ? enter y to save / n to cancel and play audio " << '\n';
    std::cin >> saveF;
    if (saveF == 'y' || saveF == 'Y') {
        saveWavFile("output.wav", outputSamples, audioF.wavSpec);
    }
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
    SDL_FreeWAV(audioF.wavBuffer);
    SDL_Quit();
    return 0;
}