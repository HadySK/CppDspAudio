#ifndef _AUDIODSP_H__
#define _AUDIODSP_H__

int audioDelay(int numSamples, int16_t* inputSamples, std::vector<int16_t>* outputSamples);
int audioEcho(int numSamples, int16_t* inputSamples, std::vector<int16_t>* outputSamples);
bool saveWavFile(const char* filename, const std::vector<int16_t>& samples, const SDL_AudioSpec& spec);


#endif /*_AUDIODSP_H__*/