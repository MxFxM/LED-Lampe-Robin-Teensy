#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue_7 = 0.0;

void reset_7(void)
{
    hue_7 = 0.0;
}

void run_animation_7(RgbColor ledarray[NUMPIXELS], int stripe_offsets[NUMBER_OF_STRIPES + 1])
{
    // prepare the color
    HsvColor hsvcol;
    hue_7 += 1; // increase hue value for rainbow effect

    // limit to 255
    if (hue_7 >= 256)
    {
        hue_7 -= 256;
    }

    float intermed_hue = hue_7;

    for (int stripenr = 0; stripenr < NUMBER_OF_STRIPES; stripenr++)
    {
        // cycle intermediate hue
        intermed_hue += 10;
        if (intermed_hue >= 256)
        {
            intermed_hue -= 256;
        }

        // create hsv color
        hsvcol.h = int(intermed_hue);
        hsvcol.s = 255;
        hsvcol.v = DEFAULT_BRIGHTNESS;

        // convert to rgb
        RgbColor rgbcol = HsvToRgb(hsvcol);

        for (int i = 0; i < LEDS_PER_STRIPE; i++)
        {
            ledarray[i + stripe_offsets[stripenr]] = rgbcol;
        }
    }

    // built in delay
    delay(1);
}