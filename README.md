# CppDspAudio
testing dsp algorithms on audio


we are using SDL to read a .wav audio file
we store the file into a buffer, manipulate the buffer and then play it back with SDL.

## Delay
to add a delay we take the input buffer, delay it by a set amount of time, then add it back to the original output as shown below
![alt text](docs/delay.png)
*Taken from Arm DSP course*

## Echo
An echo works similarly to a delay but we take feedback signal from the delayed input multiplied by a set amount of again and add it back to the input before the delay (feedback multiplied by gain add that fading echo sound)
![alt text](docs/echo.png)


## Generating a sine wave
To generate a sine wave, we create an point array and assign each point to a sample then save the wav file. we have to specifiy the frequency, because we are using 8 sine wave points, we have to set the frequency to 8000 instead of 1000 to get 1Khz sine wave.
sine_table[i] = 10000sin(2*pi*i/8) << if we use 1000 instead of 1000 we get the same sine wave but smaller amplitude
![alt text](docs/sine.png)

Run project using Visual Studio 2022

## references

audio samples taken from here https://github.com/voxserv/audio_quality_testing_samples