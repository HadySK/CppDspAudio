# CppDspAudio
testing dsp algorithms on audio


We are using SDL to read a .wav audio file, store it into a buffer, manipulate the buffer then save it or play it back.  
The audio file is used as an analog signal source, after running each function we can save the signal as .wav file then analyze it using audio software like Audacity to act as our oscilloscope.


## Delay
To add a delay we take the input buffer, delay it by a set amount of time, then add it back to the original output as shown below
![alt text](docs/delay.png)
*Taken from Arm DSP course*

## Echo
An echo works similar to a delay but we take feedback signal from the delayed input multiplied by a set amount of gain and add it back to the input before the delay (feedback multiplied by gain create that fading echo sound)
We apply the echo only to the left Audio channel so if you are wearing headphones you can here the original audio on the right audio channel and modified audio on the left audio channel
![alt text](docs/echo.png)

https://github.com/user-attachments/assets/4fffd921-b9a2-47a6-a436-ec02117e1b39  

[Audio Backup link](https://github.com/HadySK/CppDspAudio/raw/refs/heads/main/docs/echoAudio_.wav)

## Generating a sine wave
To generate a sine wave, we create a look up table for the sin wave values and assign each point to a sample then save the wav file, alternatively we can just use the equation below(both option are present in the code). we have to specifiy the frequency, because we are using 8 sine wave points, we have to set the frequency to 8000 instead of 1000 to get 1Khz sine wave.
sine_table[i] = 10000sin(2*pi*i/8) << if we use 1000 instead of 10000 we get the same sine wave but smaller amplitude
![alt text](docs/sine.png)

## Generating a square wave
We adjust the sine wave code by changing the lookup table to switch between two opposite fixed values (10000,-10000) to generate a square wave of 125Hz

![alt text](docs/sqwave.png)

![alt text](docs/sqf.png)  


The ideal square wave contains only components of odd-integer harmonic frequencies (of the form 2π(2k − 1)f).
 3rd harmonic frequency = 375 Hz
 5th harmonic frequency = 625 Hz
 7th harmonic frequency = 875 Hz

Using Audacity, we can see the harmonics when we analyze the frequency of the wav file for the 125hz square wave
![alt text](docs/sqwaveHarmonics.png)

## Finite Impulse Response (FIR) Filter (Moving Average Filter)
We will use a moving average filter as an example of an FIR filter.
Moving average filter is a low pass so in theory if we take our echo audio file and applying a movingaverage filter we should see some of the higher frequencies cut off.
Lets take a look at our echo audio file frequencies

![alt text](docs/echoMono.png)
*We have frequencies up to 21kHz*

now lets look at the same file frequencies after applying the moving average filter
![alt text](docs/movingAvgEchoMono.png)
*Frequencies after 15kHz are cut off*

Only frequencies below 15kHz make it through and the rest are cut off

![alt text](docs/ogandFIR.png)
## Infinite Impulse Response (IIR) Filter (2nd Order Elliptic Filter)
We will use Matlab filter design tool to generate the coefficients for a fourth order elliptical low pass IIR filter with a cutoff frequency of 8000Hz, 1dB ripple in the pass band and 50dB of stop band attenuation.

![alt text](docs/2ndOrderIIRelliptic.png)

We will use the same mono audio file,below is the frequency analysis after applying the elliptic IIR filter
![alt text](docs/IIRAfter.png)
*Frequencies after 15kHz are cut off*

## Discrete Fourier Transform (DFT)
Discrete fourier transform converts a signal from time domain to frequency domain.
![alt text](docs/dfteq.png)

we divide the equation into Real(cosine) and imaginary (sine) parts
![alt text](docs/dftreImg.png)

our DFT function creates an 800hz sinusoidal wave then it goes through DFT where the real part will be assigned to left audio channel and imaginary part assigned to right audio channel. 
![alt text](docs/reImgDft.png)


Looking at the generated wave, the imaginary part magnitude is zero and the real part all values are zero except for two values. these are the representation of the 8khz input signal in
frequency domain.
The first spike is a representation of the positive 800hz input signal, the second spike
Based on Euler's formula, a real valued sinusoidal signal may be represented by a pair of complex exponentials in the frequency domain corresponding to two contra-rotating phasors. the second spike is the representation of a signal at negative 800hz. In the frequency the value takes both positive and negative frequencies

cool animation of the two phasors can be found in here 👉 [Rotating Phasors](https://dspfirst.gatech.edu/chapters/03spect/demos/phasors/graphics/phasorsn.mp4)

if we can the number of samples to 4096.we get spectral leakage and see the following spikes on both real and imaginary parts
![alt text](docs/spectralLeakageDFT.png)

That happens because our 800hz is not integer multiple of the sample frequency divied by N. and there is no single value of k that correspond exactly to that frequency.
we can work around that and set our test frequency to a float that matches exactly (800.78125f)

## Fast Fourier Transform (FFT)
One way to make DFT faster is to precalculate and store the twiddle factors, then it will be used a lookup table of twiddle  factors

```cpp
Execution time DFT : 1861 ms
Execution time DFT with precalculated twiddle : 129 ms
```
We can that with the precalculated twiddle factors, the DFT function is about 14 times faster than regular DFT function.

For FFT we will use Radix-2 Algorithm,
Radix-2 algorithm is one of the Fast Fourier transform (FFT) algorithms. It computes separately the DFTs of the even-indexed inputs (x<sub>0</sub>, x<sub>2</sub>,..., x<sub>N-2</sub>) and of the odd-indexed inputs (x<sub>1</sub>, x<sub>3</sub>,..., x<sub>N-1</sub>, and then combines those two results to produce the DFT of the whole sequence. This idea can then be performed recursively to reduce the overall runtime from O(N<sup>2</sup>) to O(N logN).
Radix-2 algorithm requires that N(sample points) is a power of two.

```cpp
Execution time DFT : 1861 ms
Execution time DFT with precalculated twiddle : 129 ms
Execution time Radix-2 FFT : 813 us
```
Radix-2 FFT  is 155 times faster than DFT with precalculated twiddle factor, and about 2000 times faster than regular DFT !

## Build & Run

Project uses C++14 & SDL2
The project in the repo is made using Visual Studio 2022.

## references

[1] audio samples taken from here https://github.com/voxserv/audio_quality_testing_samples
[2] Square Wave analysis https://en.wikipedia.org/wiki/Square_wave_(waveform)#Fourier_analysis
[3] DFT https://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch31.pdf
[4] Rotating Phasors https://dspfirst.gatech.edu/chapters/03spect/demos/phasors/index.html
[5] FFT https://www.phys.uconn.edu/~rozman/Courses/m3511_19s/downloads/radix2fft.pdf
