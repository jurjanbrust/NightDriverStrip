//+--------------------------------------------------------------------------
//
// File:        misceffects.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.  
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//   
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//   
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//
// Description:
//
//    Draws bouncing balls using a kinematics formula
//
// History:     Apr-17-2019         Davepl      Adapted from NightDriver
//              
//---------------------------------------------------------------------------

#pragma once

// SimpleRainbowTestEffect
//
// Fills the spokes with a rainbow palette, skipping dots as specified

class SimpleRainbowTestEffect : public LEDStripEffect
{
  private:
    uint8_t     _EveryNth;
    uint8_t     _SpeedDivisor;

  public:
  
    SimpleRainbowTestEffect(uint8_t speedDivisor = 8, uint8_t everyNthPixel = 12)
      : LEDStripEffect("Simple Rainbow"),
          _EveryNth(everyNthPixel),
          _SpeedDivisor(speedDivisor)
    {
        debugV("SimpleRainbowTestEffect constructor");
    }

    virtual void Draw() 
    {
        fillRainbowAllChannels(0, _cLEDs, beatsin16(4, 0, 256), 8, _EveryNth);
        delay(10);
    }   
};

// SimpleRainbowTestEffect
//
// Fills the spokes with a rainbow palette, skipping dots as specified

class RainbowTwinkleEffect : public LEDStripEffect
{
  private:
    float _speedDivisor;
    int   _deltaHue;

  public:
  
    RainbowTwinkleEffect(float speedDivisor = 12.0f, int deltaHue = 14)
      : LEDStripEffect("Rainbow Twinkle"),
        _speedDivisor(speedDivisor),
        _deltaHue(deltaHue)
    {
        debugV("RainbowFill constructor");
    }

    virtual void Draw()
    {
        static float hue = 0.0f;
        static unsigned long lastms = millis();

        unsigned long msElapsed = millis() - lastms;
        lastms = millis();

        hue += (float) msElapsed / _speedDivisor;
        hue = fmod(hue, 256.0);
        fillRainbowAllChannels(0, _cLEDs, hue, _deltaHue);

        if (random(0, 1) == 0)
            setPixelOnAllChannels(random(0, _cLEDs), CRGB::White);
        delay(10);
    }
};

// RainbowFillEffect
//
// Fills the spokes with a rainbow palette


class RainbowFillEffect : public LEDStripEffect
{
  private:

protected:

    float _speedDivisor;
    int   _deltaHue;

  public:
    
    RainbowFillEffect(float speedDivisor = 12.0f, int deltaHue = 14)
      : LEDStripEffect("RainobwFill Rainbow"),
        _speedDivisor(speedDivisor),
        _deltaHue(deltaHue)
    {
        debugV("RainbowFill constructor");
    }

    virtual void Draw()
    {
        static float hue = 0.0f;
        static unsigned long lastms = millis();

        unsigned long msElapsed = millis() - lastms;
        lastms = millis();

        hue += (float) msElapsed / _speedDivisor;
        hue = fmod(hue, 256.0);
        fillRainbowAllChannels(0, _cLEDs, hue, _deltaHue);
        delay(10);
    }
};

// RainbowFillEffect
//
// Fills the spokes with a rainbow palette


class ColorFillEffect : public LEDStripEffect
{
  private:

protected:

    int _everyNth;
    CRGB _color;

  public:
    
    ColorFillEffect(CRGB color = CRGB(246,200,160), int everyNth = 10)
      : LEDStripEffect("Color Fill"),
        _everyNth(everyNth),
        _color(color)
    {
        debugV("Color Fill constructor");
    }

    virtual void Draw()
    {
        if (_everyNth != 1)
          fillSolidOnAllChannels(CRGB::Black);                    
        fillSolidOnAllChannels(_color);
    }
};

class StatusEffect : public LEDStripEffect
{
  protected:

    int  _everyNth;
    CRGB _color;

  public:
    
    StatusEffect(CRGB color = CRGB(255,255,255), int everyNth = 10)     // Warmer: CRGB(246,200,160)
      : LEDStripEffect("Status Fill"),
        _everyNth(everyNth),
        _color(color)
    {
        debugV("Status Fill constructor");
    }

    virtual void Draw()
    {
        CRGB color = _color;

        if (g_bUpdateStarted)
          color = CRGB::Purple;
        else if (!WiFi.isConnected())
          color = CRGB::Red;
        else if (!NTPTimeClient::HasClockBeenSet())
          color = CRGB::Green;

        if (_everyNth != 1)
          fillSolidOnAllChannels(CRGB::Black);
        fillSolidOnAllChannels(color, 0, 0, _everyNth);
    }
};

#if CLASSIC_GE_C9
static const CRGB TwinkleColors[] = 
{
    CRGB(238, 51, 39),      // Red
    CRGB(0, 172, 87),       // Green
    CRGB(250, 164, 25),     // Yellow
    CRGB(0, 131, 203)       // Blue
};
#else
static const CRGB TwinkleColors[] = 
{
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::White
};
#endif

class TwinkleEffect : public LEDStripEffect
{
  protected:

    int  _countToDraw;
    int  _fadeFactor;
    int  _updateSpeed;

  public:
    
    TwinkleEffect(int countToDraw = NUM_LEDS / 2, uint8_t fadeFactor = 10, int updateSpeed = 10)
      : LEDStripEffect("Twinkle"),
          _countToDraw(countToDraw),
        _fadeFactor(fadeFactor),
        _updateSpeed(updateSpeed)
    {
    }

    const int Count = 99;
    int buffer[99] = { 0 };

    std::deque<size_t> litPixels;

    virtual void Draw()
    {
        EVERY_N_MILLISECONDS(_updateSpeed)
        {
            if (litPixels.size() > _countToDraw)
            {
                size_t i = litPixels.back();
                litPixels.pop_back();
                _GFX[0]->setPixel(i, CRGB::Black);
            }
        
            // Pick a random pixel and put it in the TOP slot
            int iNew = -1;
            for (int iPass = 0; iPass < NUM_LEDS * 10; iPass++)
            {
                size_t i = random(0, NUM_LEDS);
                if (_GFX[0]->getPixel(i) != CRGB(0,0,0))
                    continue;
                if (litPixels.end() != find(litPixels.begin(), litPixels.end(), i))
                    continue;
                iNew = i;
                break;
            }
            if (iNew == -1)             // No empty slot could be found!
            {   
                litPixels.clear();
                setAllOnAllChannels(0,0,0);
                return;
            }
            
            assert(litPixels.end() == find(litPixels.begin(), litPixels.end(), iNew));
            setPixelOnAllChannels(iNew, TwinkleColors[random(0, ARRAYSIZE(TwinkleColors))]);
            litPixels.push_front(iNew);
        }

        EVERY_N_MILLISECONDS(20)
        {
            fadeToBlackBy(FastLED.leds(), NUM_LEDS, _fadeFactor);
        }
    }
};

class PoliceEffect : public LEDStripEffect
{
    typedef enum { INNERRED, OUTERRED, INNERBLUE, OUTERBLUE, MIXED, STROBE } lampStates;
    const lampStates highestState = STROBE;

    virtual void Draw()
    {
        

    }
};
