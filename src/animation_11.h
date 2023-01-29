#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue_11 = 0.0;

void reset_11(void)
{
    hue_11 = 0.0;
}

void adjust_gain_11(float *bin_all, float *gain, AudioAmplifier *amp, float stripe_maximums[NUMBER_OF_STRIPES], float *peak)
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

void update_peaks_11(float *bin_all, AudioAnalyzePeak *peak_all, float bins[NUMBER_OF_STRIPES], float *gain, AudioAmplifier *amp, AudioAnalyzeFFT1024 *fft1024, float stripe_maximums[NUMBER_OF_STRIPES])
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
        adjust_gain_11(bin_all, gain, amp, stripe_maximums, &bass);

        // 16 stripes
        // 0 to 7 flow towards the smaller index, so index 0 = previous 1
        // with 7 being updated with the latest bass value
        // 8 to 15 flow towards the higher index, so index 15 = previous 14
        // wih 8 being updated with the latest bass value
        // we have to traverse from 15 to 8 in order to properly update them

        // upate list
        for (int i = 0; i < floor(NUMBER_OF_STRIPES / 2) - 1; i++)
        {
            // left side flowing to the left
            bins[i] = bins[i + 1];
        }

        for (int i = NUMBER_OF_STRIPES - 1; i > ceil(NUMBER_OF_STRIPES / 2); i--)
        {
            // right side flowing to the right
            bins[i] = bins[i - 1];
        }

        // add new value at the end
        bins[int(floor(NUMBER_OF_STRIPES / 2)) - 1] = bass;
        bins[int(ceil(NUMBER_OF_STRIPES / 2))] = bass;
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

void run_animation_11(RgbColor ledarray[NUMPIXELS], float bins[NUMBER_OF_STRIPES], int stripe_offsets[NUMBER_OF_STRIPES + 1], float stripe_maximums[NUMBER_OF_STRIPES], float scaling3d[NUMBER_OF_STRIPES])
{
    // prepare the color
    HsvColor hsvcol;
    HsvColor hsvcol_mod;
    hue_11 += HUE_CHANGE_SPEED_SLOW; // increase hue value for rainbow effect

    // limit to 255
    if (hue_11 > 255)
    {
        hue_11 = 0;
    }

    // create hsv color
    hsvcol.h = int(hue_11);
    hsvcol.s = 255;
    hsvcol.v = DEFAULT_BRIGHTNESS;

    // create modulated hsv color
    hsvcol_mod.h = int(hue_11);
    hsvcol_mod.s = 255;
    hsvcol_mod.v = int(DEFAULT_BRIGHTNESS * bins[int(NUMBER_OF_STRIPES / 2)]);

    // convert to rgb
    RgbColor rgbcol = HsvToRgb(hsvcol);
    RgbColor rgbcol_mod = HsvToRgb(hsvcol_mod);

    // turn off led brightness
    for (int i = 0; i < NUMPIXELS; i++)
    {
        ledarray[i].r = 0;
        ledarray[i].g = 0;
        ledarray[i].b = 0;
    }

    for (int stripenr = 0; stripenr < NUMBER_OF_STRIPES; stripenr++)
    {
        // how many leds to turn on depends on the peak value of the bin
        int turnonnr = map(bins[stripenr], 0.0, stripe_maximums[0] * 2, 1, LEDS_PER_STRIPE * scaling3d[stripenr]); // map to number of pixels // modulo to account for 3 strips only at the moment

        // limit in case of unexpected input range
        if (turnonnr > LEDS_PER_STRIPE / 2)
        {
            turnonnr = int(LEDS_PER_STRIPE / 2);
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
                ledarray[i + stripe_offsets[stripenr] + int(LEDS_PER_STRIPE / 2)] = rgbcol;
                ledarray[-i + stripe_offsets[stripenr] + int(LEDS_PER_STRIPE / 2)] = rgbcol;
            }
        }
        else
        { // every odd row
            // turn the new leds on, start inverted, since the zig-zag
            for (int i = 0; i < turnonnr; i++)
            {
                ledarray[stripe_offsets[stripenr + 1] - i - 1 - int(LEDS_PER_STRIPE / 2)] = rgbcol;
                ledarray[stripe_offsets[stripenr + 1] + i - 1 - int(LEDS_PER_STRIPE / 2)] = rgbcol;
            }
        }

        // static lines
        int limit = int((LEDS_PER_STRIPE - LEDS_PER_STRIPE * scaling3d[stripenr]) / 2) - 5;

        if (limit <= 0)
        {
            limit = 0;
        }
        for (int i = 0; i < limit; i++)
        {
            ledarray[stripe_offsets[stripenr] + i] = rgbcol_mod;
            ledarray[stripe_offsets[stripenr + 1] - i - 1] = rgbcol_mod;
        }
    }
}