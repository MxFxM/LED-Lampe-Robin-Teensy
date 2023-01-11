#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue = 0.0;

void reset_1(void)
{
    hue = 0.0;
}

float getmaxpeak(float bins[NUMBER_OF_STRIPES])
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

void adjust_gain_1(float *bin_all, float *gain, float bins[NUMBER_OF_STRIPES], float stripe_maximums[NUMBER_OF_STRIPES], AudioAmplifier *amp)
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

    for (int gn = 0; gn < NUMBER_OF_STRIPES; gn++)
    {
        // if the overall peak is too high, decrease amplification faster
        if (bins[gn] > stripe_maximums[gn] * 0.9)
        {
            stripe_maximums[gn] += 0.001;
        }

        // if the overall peak is too low, increase amplification slowly
        if (bins[gn] < stripe_maximums[gn] * 0.3)
        {
            stripe_maximums[gn] -= 0.0001;
        }

        // limit the maximum to 1, since the signal cannot be greater
        if (stripe_maximums[gn] > 1.0)
        {
            stripe_maximums[gn] = 1.0;
        }

        // limit the minimum maximum (lol)
        if (stripe_maximums[gn] < 0.001)
        {
            stripe_maximums[gn] = 0.001;
        }
    }
}

void update_peaks_1(float *bin_all, AudioAnalyzePeak *peak_all, AudioAnalyzeFFT1024 *fft1024, float bins[NUMBER_OF_STRIPES], float *gain, float stripe_maximums[NUMBER_OF_STRIPES], AudioAmplifier *amp)
{
    // update overall peak measurement
    float peak = *bin_all; // get the old value
    float peak_bins[NUMBER_OF_STRIPES];
    for (int bn = 0; bn < NUMBER_OF_STRIPES; bn++)
    {
        peak_bins[bn] = bins[bn]; // get the old value
    }

    // if there is a new value available, read it
    // this one is required for gain adjustment
    if (peak_all->available())
    {
        peak = peak_all->read();
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

    // same for fft values
    if (fft1024->available())
    {
        // add to bins by hand
        peak_bins[0] = fft1024->read(1, 4);      // was 1
        peak_bins[1] = fft1024->read(5, 8);      // was 2
        peak_bins[2] = fft1024->read(9, 12);     // was 3
        peak_bins[3] = fft1024->read(13, 16);    // was 4
        peak_bins[4] = fft1024->read(17, 20);    // was 5
        peak_bins[5] = fft1024->read(21, 24);    // was 6
        peak_bins[6] = fft1024->read(25, 28);    // was 7
        peak_bins[7] = fft1024->read(29, 32);    // was 8
        peak_bins[8] = fft1024->read(33, 36);    // was 9
        peak_bins[9] = fft1024->read(37, 40);    // was 10
        peak_bins[10] = fft1024->read(41, 48);   // was 11 to 12
        peak_bins[11] = fft1024->read(49, 56);   // was 13 to 14
        peak_bins[12] = fft1024->read(57, 64);   // was 15 to 16
        peak_bins[13] = fft1024->read(65, 80);   // was 17 to 20
        peak_bins[14] = fft1024->read(81, 104);  // was 21 to 26
        peak_bins[15] = fft1024->read(105, 120); // was 27 to 30

        adjust_gain_1(bin_all, gain, bins, stripe_maximums, amp);
    }

    for (int bn = 0; bn < NUMBER_OF_STRIPES; bn++)
    {
        bins[bn] = peak_bins[bn]; // use the higher value
    }
}

void run_animation_1(RgbColor ledarray[NUMPIXELS], float bins[NUMBER_OF_STRIPES], int stripe_offsets[NUMBER_OF_STRIPES + 1], float stripe_maximums[NUMBER_OF_STRIPES])
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
        int turnonnr = map(bins[stripenr], 0.0, stripe_maximums[stripenr], 1, LEDS_PER_STRIPE); // map to number of pixels // modulo to account for 3 strips only at the moment

        // limit in case of unexpected input range
        if (turnonnr > LEDS_PER_STRIPE)
        {
            turnonnr = LEDS_PER_STRIPE;
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
}