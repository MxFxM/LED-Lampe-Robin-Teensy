#include <Arduino.h>

#include <color.h>

#define MINIMUM_GAIN 0.5
#define MAXIMUM_GAIN 70.0
#define BEAT_DECAY 0.05
#define HUE_CHANGE_SPEED_SLOW 0.1
#define DEFAULT_BRIGHTNESS 20

#define BRIGHTNESS_DECAY 1

#include <WS2812Serial.h>
//  Teensy 4.1:  1, 8, 14, 17, 20, 24, 29, 35, 47, 53
#define LEDPIN 1
#define NUMPIXELS 14
byte drawingMemory[NUMPIXELS * 3];         //  3 bytes per LED
DMAMEM byte displayMemory[NUMPIXELS * 12]; // 12 bytes per LED
WS2812Serial leds(NUMPIXELS, displayMemory, drawingMemory, LEDPIN, WS2812_GRB);

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputAnalog         adc1;           //xy=706,445
AudioInputI2S            i2s1;           //xy=708,340
AudioAmplifier           amp1;           //xy=850,445
AudioFilterFIR           fir3;           //xy=1093,607
AudioFilterFIR           fir4;           //xy=1095,652
AudioFilterFIR           fir2;           //xy=1097,562
AudioFilterFIR           fir1;           //xy=1098,514
AudioAnalyzePeak         peak4;          //xy=1272,608
AudioAnalyzePeak         peak5;          //xy=1273,652
AudioAnalyzePeak         peak3;          //xy=1275,563
AudioAnalyzePeak         peak2;          //xy=1276,514
AudioAnalyzePeak         peak1;          //xy=1278,447
AudioConnection          patchCord1(adc1, amp1);
AudioConnection          patchCord2(amp1, peak1);
AudioConnection          patchCord3(amp1, fir1);
AudioConnection          patchCord4(amp1, fir2);
AudioConnection          patchCord5(amp1, fir3);
AudioConnection          patchCord6(amp1, fir4);
AudioConnection          patchCord7(fir3, peak4);
AudioConnection          patchCord8(fir4, peak5);
AudioConnection          patchCord9(fir2, peak3);
AudioConnection          patchCord10(fir1, peak2);
// GUItool: end automatically generated code

float bin1 = 0.0;
float gain = 1.0;
float hue = 0.0;

RgbColor ledarray[NUMPIXELS];

void run_animation(void);
void adjust_gain(void);

void setup()
{
    AudioMemory(1024);
    amp1.gain(gain);

    leds.begin();

    Serial.begin(115200);
    Serial.println("Hey");
}

void loop()
{
    run_animation();
    adjust_gain();

    Serial.println();
    delay(10);
}

void adjust_gain(void)
{
    if (bin1 > 0.6)
    {
        gain -= 0.1;
    }

    if (bin1 < 0.3)
    {
        gain += 0.05;
    }

    if (gain > MAXIMUM_GAIN)
    {
        gain = MAXIMUM_GAIN;
    }

    if (gain < MINIMUM_GAIN)
    {
        gain = MINIMUM_GAIN;
    }

    Serial.print(gain);
    Serial.print("\t");

    amp1.gain(gain);
}

void run_animation(void)
{
    float peak = bin1;
    if (peak1.available())
    {
        // for (int i = 0; i < 15; i++)
        //{
        //     Serial.print(fft256_1.read(i));
        //     Serial.print("\t");
        // }
        peak = peak1.read(); // 0,1 as range
    }

    Serial.print(peak);
    Serial.print("\t");

    if (peak > bin1)
    {
        bin1 = peak;
    }
    else
    {
        bin1 = bin1 - BEAT_DECAY;
    }

    Serial.print(bin1);
    Serial.print("\t");

    int turnonnr = map(bin1, 0.2, 0.8, 0, NUMPIXELS);
    if (turnonnr > NUMPIXELS)
    {
        turnonnr = NUMPIXELS;
    }
    if (turnonnr < 0)
    {
        turnonnr = 0;
    }

    HsvColor hsvcol;
    hue += HUE_CHANGE_SPEED_SLOW;
    if (hue > 255)
    {
        hue = 0;
    }
    hsvcol.h = int(hue);
    hsvcol.s = 255;
    hsvcol.v = DEFAULT_BRIGHTNESS;

    RgbColor rgbcol = HsvToRgb(hsvcol);

    Serial.print(turnonnr);
    Serial.print("\t");

    for (int i = 0; i < NUMPIXELS; i++)
    {
        if (ledarray[i].r > BRIGHTNESS_DECAY)
        {
            ledarray[i].r -= BRIGHTNESS_DECAY;
        }
        else
        {
            ledarray[i].r = 0;
        }
        if (ledarray[i].g > BRIGHTNESS_DECAY)
        {
            ledarray[i].g -= BRIGHTNESS_DECAY;
        }
        else
        {
            ledarray[i].g = 0;
        }
        if (ledarray[i].b > BRIGHTNESS_DECAY)
        {
            ledarray[i].b -= BRIGHTNESS_DECAY;
        }
        else
        {
            ledarray[i].b = 0;
        }
    }

    for (int i = 0; i < turnonnr; i++)
    {
        // leds.setPixel(i, rgbcol.r, rgbcol.g, rgbcol.b);
        ledarray[i] = rgbcol;
    }
    // for (int i = turnonnr; i < NUMPIXELS; i++)
    //{
    //     leds.setPixel(i, 0x000000); // off
    // }
    for (int i = 0; i < NUMPIXELS; i++)
    {
        leds.setPixel(i, ledarray[i].r, ledarray[i].g, ledarray[i].b);
    }
    leds.show();
}