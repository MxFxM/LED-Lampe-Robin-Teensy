#include <Arduino.h>

#include <color.h>

#define DEBUG_PRINTS true

#define MAIN_LOOP_DELAY 0

#define MINIMUM_GAIN 0.2
#define MAXIMUM_GAIN 100.0
#define BEAT_DECAY 0.05
#define HUE_CHANGE_SPEED_SLOW 0.1
#define DEFAULT_BRIGHTNESS 100

#define BRIGHTNESS_DECAY 1.3

#define NUMBER_OF_STRIPES 16
int stripe_offsets[NUMBER_OF_STRIPES + 1] = {0, 67, 134, 201, 268, 335, 402, 469, 536, 603, 670, 737, 804, 871, 938, 1005, 1072}; // need one more, since the last strip is inverted
float stripe_maximums[NUMBER_OF_STRIPES] = {0.5, 0.5, 0.4, 0.4, 0.3, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.5, 0.2};
int leds_per_stripe = 67;
float bin_list[] = {0.0, 0.0, 0.0};

#include <WS2812Serial.h>
//  Teensy 4.1:  1, 8, 14, 17, 20, 24, 29, 35, 47, 53
#define LEDPIN 1
// 16 * 67 = 1072
#define NUMPIXELS 1072
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
float gains[NUMBER_OF_STRIPES];
float hue = 0.0;

RgbColor ledarray[NUMPIXELS];

void printFloat(float value);

void update_peaks(void);
void run_animation(void);
void adjust_gain(void);

void setup()
{
    // setup audio nodes
    AudioMemory(1024);         // memory reserved
    amp.gain(gain);            // starting gain for automatic adjustment
    fft256.averageTogether(6); // with roughly 300 values per second, this still updates 60 times per second

    // setup ws2812b leds
    leds.begin(); // begin serial driver

    for (int gn = 0; gn < NUMBER_OF_STRIPES; gn++)
    {
        gains[gn] = 1.0;
    }

#ifdef DEBUG_PRINTS
    Serial.begin(115200);
    Serial.println("Hey");
#endif
}

void loop()
{
    // run main functions
    update_peaks();  // update peak values for all bins
    run_animation(); // show on the led strips
    adjust_gain();   // automatic gain adjustment

#ifdef DEBUG_PRINTS
    Serial.println();
#endif

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

float getmaxpeak(void)
{
    float maxp = 0;
    for (int bn = 0; bn < NUMBER_OF_STRIPES; bn++)
    {
        if (bins[bn] > maxp)
        {
            maxp = bins[bn];
        }
    }
    return maxp;
}

void adjust_gain(void)
{
    float maxpeak = bin_all; // getmaxpeak();
    // if the overall peak is too high, decrease amplification faster
    if (maxpeak > 0.95)
    {
        gain -= 0.1;
    }

    // if the overall peak is too low, increase amplification slowly
    if (maxpeak < 0.6)
    {
        gain += 0.05;
    }

    // limit the maximum gain
    if (gain > MAXIMUM_GAIN)
    {
        gain = MAXIMUM_GAIN;
    }

    // limit the minimum gain
    if (gain < MINIMUM_GAIN)
    {
        gain = MINIMUM_GAIN;
    }

    printFloat(gain);

    // set the new gain value for the next loop
    amp.gain(gain);

    for (int gn = NUMBER_OF_STRIPES; gn < NUMBER_OF_STRIPES; gn++)
    {
        // if the overall peak is too high, decrease amplification faster
        if (bins[gn] > 0.5)
        {
            gains[gn] -= 0.01;
        }

        // if the overall peak is too low, increase amplification slowly
        if (bins[gn] < 0.01)
        {
            gains[gn] += 0.01;
        }

        // limit the maximum gain
        if (gains[gn] > MAXIMUM_GAIN)
        {
            gains[gn] = MAXIMUM_GAIN;
        }

        // limit the minimum gain
        if (gains[gn] < MINIMUM_GAIN)
        {
            gains[gn] = MINIMUM_GAIN;
        }
    }
}

void update_peaks(void)
{
    // update overall peak measurement
    float peak = bin_all; // get the old value
    float peak_bins[NUMBER_OF_STRIPES];
    for (int bn = 0; bn < NUMBER_OF_STRIPES; bn++)
    {
        peak_bins[bn] = bins[bn]; // get the old value
    }

    // if there is a new value available, read it
    // this one is required for gain adjustment
    if (peak_all.available())
    {
        peak = peak_all.read();
    }

    // same for fft values
    if (fft256.available())
    {
        // add to bins by hand
        peak_bins[0] = fft256.read(1) * gains[0];
        peak_bins[1] = fft256.read(2) * gains[1];
        peak_bins[2] = fft256.read(3) * gains[2];
        peak_bins[3] = fft256.read(4) * gains[3];
        peak_bins[4] = fft256.read(5) * gains[4];
        peak_bins[5] = fft256.read(6) * gains[5];
        peak_bins[6] = fft256.read(7) * gains[6];
        peak_bins[7] = fft256.read(8) * gains[7];
        peak_bins[8] = fft256.read(9) * gains[8];
        peak_bins[9] = fft256.read(10) * gains[9];
        peak_bins[10] = fft256.read(11, 12) * gains[10];
        peak_bins[11] = fft256.read(13, 14) * gains[11];
        peak_bins[12] = fft256.read(15, 16) * gains[12];
        peak_bins[13] = fft256.read(17, 20) * gains[13];
        peak_bins[14] = fft256.read(21, 26) * gains[14];
        peak_bins[15] = fft256.read(27, 30) * gains[15];
    }

    // decay gain value
    if (peak >= bin_all)
    {
        // if the new peak value is higher than or equal to before
        bin_all = peak; // use the higher value
    }
    else
    {
        // if the new value is less than the old peak value
        bin_all = bin_all - BEAT_DECAY; // decrease the peak slowly
    }

    // decay all bins
    for (int bn = 0; bn < NUMBER_OF_STRIPES; bn++)
    {
        if (peak_bins[bn] >= bins[bn])
        {
            // if the new peak value is higher than or equal to before
            bins[bn] = peak_bins[bn]; // use the higher value
        }
        else
        {
            // if the new value is less than the old peak value
            bins[bn] = bins[bn] - BEAT_DECAY; // decrease the peak slowly
        }
    }

    // pack values in list for later accessing
    // bin_list[0] = bin_low;
    // bin_list[1] = bin_med;
    // bin_list[2] = bin_high;

    // printFloat(bin_all);
    // printFloat(bin_low);
    // printFloat(bin_med);
    // printFloat(bin_high);
}

void run_animation(void)
{
    // prepare the color
    HsvColor hsvcol;
    hue += HUE_CHANGE_SPEED_SLOW; // increase hue value for rainbow effect

    // limit to 255
    if (hue > 255)
    {
        hue = 0;
    }

    // create hsv color
    hsvcol.h = int(hue);
    hsvcol.s = 255;
    hsvcol.v = DEFAULT_BRIGHTNESS;

    // convert to rgb
    RgbColor rgbcol = HsvToRgb(hsvcol);

    // decay led brightness
    for (int i = 0; i < NUMPIXELS; i++)
    {
        if (ledarray[i].r > BRIGHTNESS_DECAY)
        {
            ledarray[i].r = int(ledarray[i].r / BRIGHTNESS_DECAY);
        }
        else
        {
            ledarray[i].r = 0;
        }

        if (ledarray[i].g > BRIGHTNESS_DECAY)
        {
            ledarray[i].g = int(ledarray[i].g / BRIGHTNESS_DECAY);
        }
        else
        {
            ledarray[i].g = 0;
        }

        if (ledarray[i].b > BRIGHTNESS_DECAY)
        {
            ledarray[i].b = int(ledarray[i].b / BRIGHTNESS_DECAY);
        }
        else
        {
            ledarray[i].b = 0;
        }
    }

    for (int stripenr = 0; stripenr < NUMBER_OF_STRIPES; stripenr++)
    {
        // how many leds to turn on depends on the peak value of the bin
        int turnonnr = map(bins[stripenr], 0.0, stripe_maximums[stripenr], 1, leds_per_stripe); // map to number of pixels // modulo to account for 3 strips only at the moment

        // limit in case of unexpected input range
        if (turnonnr > leds_per_stripe)
        {
            turnonnr = leds_per_stripe;
        }

        // limit in case of unexpected input range
        if (turnonnr < 0)
        {
            turnonnr = 0;
        }

        if (stripenr % 2 == 0)
        { // every even row
            // turn the new leds on
            for (int i = 0; i < turnonnr; i++)
            {
                ledarray[i + stripe_offsets[stripenr]] = rgbcol;
            }
        }
        else
        { // every odd row
            // turn the new leds on, start inverted, since the zig-za
            for (int i = 0; i < turnonnr; i++)
            {
                ledarray[stripe_offsets[stripenr + 1] - i - 1] = rgbcol;
            }
        }
    }

    // update all leds according to their new value
    for (int i = 0; i < NUMPIXELS; i++)
    {
        leds.setPixel(i, ledarray[i].r, ledarray[i].g, ledarray[i].b);
    }

    // update the strip
    leds.show();
}