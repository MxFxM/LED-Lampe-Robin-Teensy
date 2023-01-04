#include <Arduino.h>

#include <color.h>

#define DEBUG_PRINTS true

#define MAIN_LOOP_DELAY 0

#define MINIMUM_GAIN 0.2
#define MAXIMUM_GAIN 70.0
#define BEAT_DECAY 0.05
#define HUE_CHANGE_SPEED_SLOW 0.1
#define DEFAULT_BRIGHTNESS 20

#define BRIGHTNESS_DECAY 1.2

#define NUMBER_OF_STRIPES 16
int stripe_offsets[NUMBER_OF_STRIPES] = {0, 67, 134, 201, 268, 335, 402, 469, 536, 603, 670, 737, 804, 871, 938, 1005};
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
float bin_low = 0.0;
float bin_med = 0.0;
float bin_high = 0.0;
float gain = 1.0;
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
    fft256.averageTogether(5); // with roughly 300 values per second, this still updates 60 times per second

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

void adjust_gain(void)
{
    // if the overall peak is too high, decrease amplification faster
    if (bin_all > 0.6)
    {
        gain -= 0.1;
    }

    // if the overall peak is too low, increase amplification slowly
    if (bin_all < 0.3)
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

    // printFloat(gain);

    // set the new gain value for the next loop
    amp.gain(gain);
}

void update_peaks(void)
{
    // update overall peak measurement
    float peak = bin_all;       // get the old value
    float peak_low = bin_low;   // get the old value
    float peak_med = bin_med;   // get the old value
    float peak_high = bin_high; // get the old value

    // if there is a new value available, read it
    // this one is required for gain adjustment
    if (peak_all.available())
    {
        peak = peak_all.read();
    }

    // same for fft values
    if (fft256.available())
    {
        peak_low = fft256.read(0, 1);    // roughly 0 to 340 Hz
        peak_med = fft256.read(2, 10);   // roughly 340 Hz to 1700 Hz
        peak_high = fft256.read(11, 60); // roughly 1700 Hz to 10k2 Hz
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

    // decay low peak
    if (peak_low >= bin_low)
    {
        // if the new peak value is higher than or equal to before
        bin_low = peak_low; // use the higher value
    }
    else
    {
        // if the new value is less than the old peak value
        bin_low = bin_low - BEAT_DECAY; // decrease the peak slowly
    }

    // decay medium peak
    if (peak_med >= bin_med)
    {
        // if the new peak value is higher than or equal to before
        bin_med = peak_med; // use the higher value
    }
    else
    {
        // if the new value is less than the old peak value
        bin_med = bin_med - BEAT_DECAY; // decrease the peak slowly
    }

    // decay high peak
    if (peak_high >= bin_high)
    {
        // if the new peak value is higher than or equal to before
        bin_high = peak_high; // use the higher value
    }
    else
    {
        // if the new value is less than the old peak value
        bin_high = bin_high - BEAT_DECAY; // decrease the peak slowly
    }

    // pack values in list for later accessing
    bin_list[0] = bin_low;
    bin_list[1] = bin_med;
    bin_list[2] = bin_high;

    // printFloat(bin_all);
    printFloat(bin_low);
    printFloat(bin_med);
    printFloat(bin_high);
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
        int turnonnr = map(bin_list[stripenr % 3], 0.0, 0.6, 0, leds_per_stripe); // map to number of pixels // modulo to account for 3 strips only at the moment

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

        // turn the new leds on
        for (int i = 0; i < turnonnr; i++)
        {
            ledarray[i + stripe_offsets[stripenr]] = rgbcol;
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