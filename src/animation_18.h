#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue_18 = 0.0;
#define RINGLENGTH_18 20
float bins_18[RINGLENGTH_18];

void reset_18(void)
{
    hue_18 = 0.0;
    for (int i = 0; i < RINGLENGTH_18; i++)
    {
        bins_18[i] = 0;
    }
}

void adjust_gain_18(float *bin_all, float *gain, AudioAmplifier *amp, float stripe_maximums[NUMBER_OF_STRIPES], float *peak)
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

void update_peaks_18(float *bin_all, AudioAnalyzePeak *peak_all, float bins[NUMBER_OF_STRIPES], float *gain, AudioAmplifier *amp, AudioAnalyzeFFT1024 *fft1024, float stripe_maximums[NUMBER_OF_STRIPES])
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
        adjust_gain_18(bin_all, gain, amp, stripe_maximums, &bass);

        // 16 stripes
        // 0 to 7 flow towards the smaller index, so index 0 = previous 1
        // with 7 being updated with the latest bass value
        // 8 to 15 flow towards the higher index, so index 15 = previous 14
        // wih 8 being updated with the latest bass value
        // we have to traverse from 15 to 8 in order to properly update them

        // upate list
        for (int i = 20 - 1; i >= 1; i--)
        {
            // flowing up
            bins_18[i] = bins_18[i - 1];
        }

        // add new value at the start
        bins_18[0] = bass;
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

float getDistance_18(float x, float y)
{
    float sum = x * x + y * y;
    return sqrt(sum);
}

void run_animation_18(RgbColor ledarray[NUMPIXELS], float bins[NUMBER_OF_STRIPES], int stripe_offsets[NUMBER_OF_STRIPES + 1], float stripe_maximums[NUMBER_OF_STRIPES])
{
    // prepare the color
    HsvColor hsvcol;
    hue_18 += HUE_CHANGE_SPEED_SLOW; // increase hue value for rainbow effect

    // limit to 255
    if (hue_18 > 255)
    {
        hue_18 = 0;
    }

    // create hsv color
    hsvcol.h = int(hue_18);
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

    float maxdistance = getDistance_18(4 * NUMBER_OF_STRIPES / 2, LEDS_PER_STRIPE / 2);

    for (int stripenr = 0; stripenr < NUMBER_OF_STRIPES; stripenr++)
    {
        for (int lednr = 0; lednr < LEDS_PER_STRIPE; lednr++)
        {
            float x = (stripenr - (NUMBER_OF_STRIPES - 1) / 2) * 4; // subtract 1 to have the center at 7.5 instead of 8
            float y = (lednr - LEDS_PER_STRIPE / 2);

            float thisdistance = getDistance_18(x, y);
            int lowerindex = int(map(thisdistance, 0, maxdistance, 0, RINGLENGTH_18 - 2));
            float thisvalue = (bins_18[lowerindex + 1] - bins_18[lowerindex]) * (lowerindex / RINGLENGTH_18) + bins_18[lowerindex];
            float hueadd = map(thisdistance, 0, maxdistance, 0, 255);
            int thishue = int(hue_18 + hueadd);
            int myvalue = int(map(thisvalue, 0.1, stripe_maximums[0], 0, 255));
            if (myvalue < 0)
            {
                myvalue = 0;
            }
            if (myvalue > 255)
            {
                myvalue = 255;
            }

            // create hsv color
            hsvcol.h = thishue;
            hsvcol.s = 255;
            hsvcol.v = myvalue;
            rgbcol = HsvToRgb(hsvcol);

            // no need to take care of the index and the inverted stripes,
            // since the animation is point symmetrical around the center
            ledarray[stripenr * LEDS_PER_STRIPE + lednr] = rgbcol;
        }
    }
}