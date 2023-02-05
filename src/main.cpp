#include <Arduino.h>
#include <Bounce.h>

#include <project_defs.h>

#define NUMBER_OF_ANIMATIONS 19
#include <animation_1.h>
#include <animation_2.h>
#include <animation_3.h>
#include <animation_4.h>
#include <animation_5.h>
#include <animation_6.h>
#include <animation_7.h>
#include <animation_8.h>
#include <animation_9.h>
#include <animation_10.h>
#include <animation_11.h>
#include <animation_12.h>
#include <animation_13.h>
#include <animation_14.h>
#include <animation_15.h>
#include <animation_16.h>
#include <animation_17.h>
#include <animation_18.h>
#include <animation_19.h>

int stripe_offsets[NUMBER_OF_STRIPES + 1] = {0, 67, 134, 201, 268, 335, 402, 469, 536, 603, 670, 737, 804, 871, 938, 1005, 1072};         // need one more, since the last strip is inverted
int stripe_offsets_default[NUMBER_OF_STRIPES + 1] = {0, 67, 134, 201, 268, 335, 402, 469, 536, 603, 670, 737, 804, 871, 938, 1005, 1072}; // need one more, since the last strip is inverted
float stripe_maximums[NUMBER_OF_STRIPES] = {0.4, 0.4, 0.4, 0.4, 0.3, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.3, 0.6, 0.2};
float stripe_maximums_default[NUMBER_OF_STRIPES] = {0.4, 0.4, 0.4, 0.4, 0.3, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.3, 0.6, 0.2};

//                                                   left, right
float scaling3d[NUMBER_OF_STRIPES] = {1, 0.9, 0.7, 0.6, 0.5, 0.3, 0.2, 0.1, 0.1, 0.2, 0.3, 0.5, 0.6, 0.7, 0.9, 1};

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
AudioInputAnalog adc;        // xy=156,600
AudioInputI2S i2s_timer;     // xy=309,430
AudioAmplifier amp;          // xy=371,628
AudioAnalyzePeak peak_all;   // xy=722,605
AudioAnalyzeFFT1024 fft1024; // xy=734,559
AudioConnection patchCord1(adc, amp);
AudioConnection patchCord2(amp, peak_all);
AudioConnection patchCord3(amp, fft1024);
// GUItool: end automatically generated code

float bin_all = 0.0;
float bins[NUMBER_OF_STRIPES];
float gain = 20.0;

RgbColor ledarray[NUMPIXELS];

#define LEFT_BUTTON_PIN 36
#define RIGHT_BUTTON_PIN 37
Bounce button_left = Bounce(LEFT_BUTTON_PIN, 10);
Bounce button_right = Bounce(RIGHT_BUTTON_PIN, 10);

#define RELAY_PIN 35
bool use_wifi = false;

int animation_number = NUMBER_OF_ANIMATIONS;

void printFloat(float value);
void reset(void);

void setup()
{
    // setup audio nodes
    AudioMemory(1024);          // memory reserved
    amp.gain(gain);             // starting gain for automatic adjustment
    fft1024.averageTogether(3); // with roughly 300 values per second, this still updates 60 times per second

    // setup ws2812b leds
    leds.begin(); // begin serial driver

    pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);

#ifdef DEBUG_PRINTS
    Serial.begin(115200);
    Serial.println("Hey");
#endif
}

void loop()
{
    // get button states
    if (button_left.update())
    {
        if (button_left.fallingEdge())
        {
            reset();
            animation_number++;
            if (animation_number > NUMBER_OF_ANIMATIONS)
            {
                animation_number = 1;
            }
        }
    }

    if (button_right.update())
    {
        if (button_right.fallingEdge())
        {
            use_wifi = !use_wifi;
        }
    }

    if (use_wifi)
    {
        digitalWriteFast(RELAY_PIN, LOW);
    }
    else
    {
        digitalWriteFast(RELAY_PIN, HIGH);
    }

    switch (animation_number)
    {
    case 1:
        // fft bins
        update_peaks_1(&bin_all, &peak_all, &fft1024, bins, &gain, stripe_maximums, &amp); // update peak values for all bins
        run_animation_1(ledarray, bins, stripe_offsets, stripe_maximums);                  // show on the led strips
        break;
    case 2:
        // wavefront of peak
        update_peaks_2(&bin_all, &peak_all, bins, &gain, &amp);
        run_animation_2(ledarray, bins, stripe_offsets);
        break;
    case 3:
        // wavefront of bass only
        update_peaks_3(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_3(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 4:
        // wavefront of bass only, but centered
        update_peaks_4(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_4(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 5:
        // wavefront of peak, but centered
        update_peaks_5(&bin_all, &peak_all, bins, &gain, &amp);
        run_animation_5(ledarray, bins, stripe_offsets);
        break;
    case 6:
        // offset per stripe rainbow through all
        run_animation_6(ledarray, stripe_offsets);
        break;
    case 7:
        // striped rainbow through all
        run_animation_7(ledarray, stripe_offsets);
        break;
    case 8:
        // fft bins, but only 43 Hz per bin and only up to 700 Hz at all
        update_peaks_8(&bin_all, &peak_all, &fft1024, bins, &gain, stripe_maximums, &amp); // update peak values for all bins
        run_animation_8(ledarray, bins, stripe_offsets, stripe_maximums);                  // show on the led strips
        break;
    case 9:
        // wavefront of bass only, coming from the center outwards
        update_peaks_9(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_9(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 10:
        // wavefront of bass only, coming from the center outwards, being smaller in the center to make a 3D effect
        update_peaks_10(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_10(ledarray, bins, stripe_offsets, stripe_maximums, scaling3d);
        break;
    case 11:
        // wavefront of bass only, coming from the center outwards, being smaller in the center to make a 3D effect, with lines
        update_peaks_11(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_11(ledarray, bins, stripe_offsets, stripe_maximums, scaling3d);
        break;
    case 12:
        // wavefront of bass only, coming from the center outwards, brightness modulated by current base
        update_peaks_9(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_9(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 13:
        // wavefront of bass only, coming from the center outwards, color modulated by current base
        update_peaks_13(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_13(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 14:
        // wavefront of bass only, coming from the center outwards, color changes towards the outside
        update_peaks_14(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_14(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 15:
        // wavefront of bass only, coming from the bottom up
        update_peaks_15(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_15(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 16:
        // wavefront of bass only, coming from the bottom up, 6 times faster
        update_peaks_16(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_16(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 17:
        // wavefront of bass only, coming from the bottom up, 6 times faster, rainbow colors
        update_peaks_17(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_17(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 18:
        // wavefront of bass only, coming from the center outwards elliptical, rainbow colors
        update_peaks_18(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_18(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    case 19:
        // fft with blocks per frequency
        update_peaks_19(&bin_all, &peak_all, bins, &gain, &amp, &fft1024, stripe_maximums);
        run_animation_19(ledarray, bins, stripe_offsets, stripe_maximums);
        break;
    default:
        reset();
        animation_number = 1;
    }

    // printFloat(gain);
    // for (int i = 0; i < NUMBER_OF_STRIPES; i++)
    //{
    //     printFloat(bins[i]);
    // }

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

void reset(void)
{
    reset_1();
    reset_2();
    reset_3();
    reset_4();
    reset_5();
    reset_6();
    reset_7();
    reset_8();
    reset_9();
    reset_10();
    reset_11();
    reset_12();
    reset_13();
    reset_14();
    reset_15();
    reset_16();
    reset_17();
    reset_18();
    reset_19();

    for (int i = 0; i < NUMBER_OF_STRIPES + 1; i++)
    {
        stripe_offsets[i] = stripe_offsets_default[i];
    }
    for (int i = 0; i < NUMBER_OF_STRIPES; i++)
    {
        stripe_maximums[i] = stripe_maximums_default[i];
    }

    gain = 20.0;
    amp.gain(gain); // starting gain for automatic adjustment
}