#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue_15 = 0.0;
float bins_15[LEDS_PER_STRIPE];

void reset_15(void)
{
    hue_15 = 0.0;
    for (int i = 0; i < LEDS_PER_STRIPE; i++)
    {
        bins_15[i] = 0;
    }
}

void adjust_gain_15(float *bin_all, float *gain, AudioAmplifier *amp, float stripe_maximums[NUMBER_OF_STRIPES], float *peak)
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

void update_peaks_15(float *bin_all, AudioAnalyzePeak *peak_all, float bins[NUMBER_OF_STRIPES], float *gain, AudioAmplifier *amp, AudioAnalyzeFFT1024 *fft1024, float stripe_maximums[NUMBER_OF_STRIPES])
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
        float bass = fft1024->read(0, 4);
        adjust_gain_15(bin_all, gain, amp, stripe_maximums, &bass);

        // 16 stripes
        // 0 to 7 flow towards the smaller index, so index 0 = previous 1
        // with 7 being updated with the latest bass value
        // 8 to 15 flow towards the higher index, so index 15 = previous 14
        // wih 8 being updated with the latest bass value
        // we have to traverse from 15 to 8 in order to properly update them

        // upate list
        for (int i = LEDS_PER_STRIPE - 1; i >= 1; i--)
        {
            // flowing up
            bins_15[i] = bins_15[i - 1];
        }

        // add new value at the start
        bins_15[0] = bass;
    }

    // decay gain value
    if (peak >= *bin_all)
    {
        // if the new peak value is higher than or equal to before
        *bin_all = peak; // use the higher value
    }
    else
    {
        // if the new value is less than the old peak value
        *bin_all = *bin_all - BEAT_DECAY; // decrease the peak slowly
    }
}

void run_animation_15(RgbColor ledarray[NUMPIXELS], float bins[NUMBER_OF_STRIPES], int stripe_offsets[NUMBER_OF_STRIPES + 1], float stripe_maximums[NUMBER_OF_STRIPES])
{
    // prepare the color
    HsvColor hsvcol;
    hue_15 += HUE_CHANGE_SPEED_SLOW; // increase hue value for rainbow effect

    // limit to 255
    if (hue_15 > 255)
    {
        hue_15 = 0;
    }

    // create hsv color
    hsvcol.h = int(hue_15);
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

    int multiple_rows_local = 1;

    for (int lednr = 0; lednr < LEDS_PER_STRIPE - multiple_rows_local; lednr += multiple_rows_local)
    {
        // how many leds to turn on depends on the peak value of the bin
        int turnonnr = map(bins_15[int(lednr / multiple_rows_local)], 0.0, stripe_maximums[0] * 2, 1, NUMBER_OF_STRIPES); // map to number of stripes

        // limit in case of unexpected input range
        if (turnonnr > NUMBER_OF_STRIPES / 2)
        {
            turnonnr = int(NUMBER_OF_STRIPES / 2);
        }

        // limit in case of unexpected input range
        if (turnonnr < 0)
        {
            turnonnr = 0;
        }

        for (int i = 0; i < turnonnr; i++)
        {
            for (int multiple_rows = 0; multiple_rows < multiple_rows_local; multiple_rows++)
            {
                // calculate led index
                int led_number = LEDS_PER_STRIPE * (8 + i) + lednr + multiple_rows;

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

    // mirror to left side
    for (int i = 0; i < NUMPIXELS / 2; i++)
    {
        ledarray[i] = ledarray[NUMPIXELS - 1 - i];
    }
}