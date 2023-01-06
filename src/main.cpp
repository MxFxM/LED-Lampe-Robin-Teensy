#include <Arduino.h>

#include <project_defs.h>

#include <animation_1.h>

int stripe_offsets[NUMBER_OF_STRIPES + 1] = {0, 67, 134, 201, 268, 335, 402, 469, 536, 603, 670, 737, 804, 871, 938, 1005, 1072}; // need one more, since the last strip is inverted
float stripe_maximums[NUMBER_OF_STRIPES] = {0.7, 0.5, 0.4, 0.4, 0.3, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.3, 0.6, 0.2};

#include <WS2812Serial.h>
byte drawingMemory[NUMPIXELS * 3];         //  3 bytes per LED
DMAMEM byte displayMemory[NUMPIXELS * 12]; // 12 bytes per LED
WS2812Serial leds(NUMPIXELS, displayMemory, drawingMemory, LEDPIN, WS2812_GRB);

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputAnalog adc;      // xy=706,445
AudioInputI2S i2s_timer;   // xy=708,340
AudioAmplifier amp;        // xy=858,445
AudioAnalyzeFFT256 fft256; // xy=1061,529
AudioAnalyzePeak peak_all; // xy=1278,447
AudioConnection patchCord1(adc, amp);
AudioConnection patchCord2(amp, peak_all);
AudioConnection patchCord3(amp, fft256);
// GUItool: end automatically generated code

float bin_all = 0.0;
float bins[NUMBER_OF_STRIPES];
float gain = 20.0;

RgbColor ledarray[NUMPIXELS];

void printFloat(float value);

void setup()
{
    // setup audio nodes
    AudioMemory(1024);         // memory reserved
    amp.gain(gain);            // starting gain for automatic adjustment
    fft256.averageTogether(3); // with roughly 300 values per second, this still updates 60 times per second

    // setup ws2812b leds
    leds.begin(); // begin serial driver

#ifdef DEBUG_PRINTS
    Serial.begin(115200);
    Serial.println("Hey");
#endif
}

void loop()
{
    // run main functions
    update_peaks_1(bin_all, peak_all, fft256, bins, gain, stripe_maximums, amp); // update peak values for all bins
    run_animation_1(ledarray, bins, stripe_offsets, stripe_maximums);            // show on the led strips

#ifdef DEBUG_PRINTS
    Serial.println();
#endif

    // update all leds according to their new value
    for (int i = 0; i < NUMPIXELS; i++)
    {
        leds.setPixel(i, ledarray[i].r, ledarray[i].g, ledarray[i].b);
    }

    // update the strip
    leds.show();

    // other functions
    delay(MAIN_LOOP_DELAY); // slow down a little
}

void printFloat(float value)
{
#ifdef DEBUG_PRINTS
    Serial.print(value);
    Serial.print("\t");
#endif
}
