#include <Arduino.h>

#include <color.h>
#include <project_defs.h>

#include <Audio.h>

float hue_6 = 0.0;

void run_animation_6(RgbColor ledarray[NUMPIXELS], int stripe_offsets[NUMBER_OF_STRIPES + 1])
{
    // prepare the color
    HsvColor hsvcol;
    hue_6 += 1; // increase hue value for rainbow effect

    // limit to 255
    if (hue_6 >= 256)
    {
        hue_6 -= 256;
    }

    float intermed_hue = hue_6;

    for (int stripenr = 0; stripenr < NUMBER_OF_STRIPES; stripenr++)
    {
        // cycle intermediate hue
        intermed_hue += 10;
        if (intermed_hue >= 256)
        {
            intermed_hue -= 256;
        }

        float inner_hue = intermed_hue;

        for (int i = 0; i < LEDS_PER_STRIPE; i++)
        {
            // cycle intermediate hue
            inner_hue += 3;
            if (inner_hue >= 256)
            {
                inner_hue -= 256;
            }

            // create hsv color
            hsvcol.h = int(inner_hue);
            hsvcol.s = 255;
            hsvcol.v = DEFAULT_BRIGHTNESS;

            // convert to rgb
            RgbColor rgbcol = HsvToRgb(hsvcol);

            ledarray[i + stripe_offsets[stripenr]] = rgbcol;
        }
    }

    // built in delay
    delay(1);
}