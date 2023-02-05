#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue_19 = 0.0;
#define XBLOCKS 4
#define YBLOCKS 3
#define NROFBLOCKS (XBLOCKS * YBLOCKS)
float bins_19[NROFBLOCKS];
float peaks_19[NROFBLOCKS];

void reset_19(void)
{
    hue_19 = 0.0;
    for (int i = 0; i < NROFBLOCKS; i++)
    {
        bins_19[i] = 0;
        peaks_19[i] = 0.5;
    }
}

void adjust_gain_19(float *bin_all, float *gain, AudioAmplifier *amp, float stripe_maximums[NUMBER_OF_STRIPES], float *peak)
{
    float maxpeak = *bin_all; // getmaxpeak(bins);
    // if the overall peak is too high, decrease amplification faster
    if (maxpeak > 0.95)
    {
        *gain -= 0.2;
    }

    // if the overall peak is too low, increase amplification slowly
    if (maxpeak < 0.6)
    {
        *gain += 0.1;
    }

    // limit the maximum gain
    if (*gain > MAXIMUM_GAIN)
    {
        *gain = MAXIMUM_GAIN;
    }

    // limit the minimum gain
    if (*gain < MINIMUM_GAIN)
    {
        *gain = MINIMUM_GAIN;
    }

    // set the new gain value for the next loop
    amp->gain(*gain);

    // update stripe maximum
    // if the overall peak is too high, decrease amplification fasterk
    if (*peak > stripe_maximums[0] * 0.95)
    {
        stripe_maximums[0] += 0.002;
    }

    // if the overall peak is too low, increase amplification slowly
    if (*peak < stripe_maximums[0] * 0.3)
    {
        stripe_maximums[0] -= 0.0001;
    }

    // limit the maximum to 1, since the signal cannot be greater
    if (stripe_maximums[0] > 1.0)
    {
        stripe_maximums[0] = 1.0;
    }

    // limit the minimum maximum (lol)
    if (stripe_maximums[0] < 0.001)
    {
        stripe_maximums[0] = 0.001;
    }
}
void update_peaks_19(float *bin_all, AudioAnalyzePeak *peak_all, float bins[NUMBER_OF_STRIPES], float *gain, AudioAmplifier *amp, AudioAnalyzeFFT1024 *fft1024, float stripe_maximums[NUMBER_OF_STRIPES])
{
    // update overall peak measurement
    float peak = *bin_all; // get the old value

    // if there is a new value available, read it
    // this one is required for gain adjustment
    if (peak_all->available())
    {
        peak = peak_all->read();
    }

    if (fft1024->available())
    {
        adjust_gain_19(bin_all, gain, amp, stripe_maximums, bins_19);
        int start = 0;
        for (int x = 0; x < XBLOCKS; x++)
        {
            for (int y = 0; y < YBLOCKS; y++)
            {
                bins_19[x * YBLOCKS + y] = fft1024->read(start, start + 1);
                if (bins_19[x * YBLOCKS + y] > peaks_19[x * YBLOCKS + y])
                {
                    // stripe_maximums[0] += 0.002;
                    peaks_19[x * YBLOCKS + y] += 0.002; // peaks_19[x * YBLOCKS + y] * 2;
                }
                if (bins_19[x * YBLOCKS + y] < peaks_19[x * YBLOCKS + y] / 10)
                {
                    // stripe_maximums[0] -= 0.0001;
                    peaks_19[x * YBLOCKS + y] -= 0.0001; // peaks_19[x * YBLOCKS + y] * 0.75;
                }
                start++;
            }
        }
    }
}

void run_animation_19(RgbColor ledarray[NUMPIXELS], float bins[NUMBER_OF_STRIPES], int stripe_offsets[NUMBER_OF_STRIPES + 1], float stripe_maximums[NUMBER_OF_STRIPES])
{
    // prepare the color
    HsvColor hsvcol;
    hue_19 += HUE_CHANGE_SPEED_SLOW; // increase hue value for rainbow effect

    // limit to 255
    if (hue_19 > 255)
    {
        hue_19 = 0;
    }

    // create hsv color
    hsvcol.h = int(hue_19);
    hsvcol.s = 255;
    hsvcol.v = DEFAULT_BRIGHTNESS;

    // convert to rgb
    RgbColor rgbcol = HsvToRgb(hsvcol);

    // turn off led brightness
    for (int i = 0; i < NUMPIXELS; i++)
    {
        ledarray[i].r = 0;
        ledarray[i].g = 0;
        ledarray[i].b = 0;
    }

    for (int stripenr = 0; stripenr < NUMBER_OF_STRIPES; stripenr++)
    {
        int xblock = floor(stripenr / (NUMBER_OF_STRIPES / XBLOCKS));
        for (int lednr = 0; lednr < LEDS_PER_STRIPE; lednr++)
        {
            int yblock = floor(lednr / (LEDS_PER_STRIPE / YBLOCKS));
            int myvalue = map(bins_19[xblock * YBLOCKS + yblock], 0, peaks_19[xblock * YBLOCKS + yblock], 0, DEFAULT_BRIGHTNESS);
            if (myvalue < 0)
            {
                myvalue = 0;
            }
            if (myvalue > 255)
            {
                myvalue = 255;
            }

            // create hsv color
            hsvcol.h = hue_19 + (xblock * YBLOCKS + yblock) * 50;
            hsvcol.s = 255;
            hsvcol.v = myvalue;
            rgbcol = HsvToRgb(hsvcol);

            // calculate led index
            int led_number = stripenr * LEDS_PER_STRIPE + lednr;

            int index;
            int led_in_segment = led_number % (LEDS_PER_STRIPE * 2);
            if (led_in_segment < LEDS_PER_STRIPE)
            {
                index = (led_number / (LEDS_PER_STRIPE * 2)) * (LEDS_PER_STRIPE * 2) + led_in_segment;
            }
            else
            {
                index = (led_number / (LEDS_PER_STRIPE * 2) + 1) * (LEDS_PER_STRIPE * 2) - 1 - led_in_segment + LEDS_PER_STRIPE;
            }
            ledarray[index] = rgbcol;
        }
    }
}