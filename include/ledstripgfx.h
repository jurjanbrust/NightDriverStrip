//+--------------------------------------------------------------------------
//
// File:        LEDStripGFX.h
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
//   Provides a Adafruit_GFX implementation for our RGB LED panel so that
//   we can use primitives such as lines and fills on it.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#pragma once
#include "gfxbase.h"

// LEDStripGFX
// 
// A derivation of GFXBase that adds LED-strip-specific functionality

class LEDStripGFX : public GFXBase 
{
  
public:

    LEDStripGFX(size_t w, size_t h) : GFXBase(w, h)
    {
        leds = static_cast<CRGB *>(calloc(w * h, sizeof(CRGB)));
        if(!leds)
        {
            throw std::runtime_error("Unable to allocate LEDs in LEDStripGFX");
        }
    }

    CRGB * GetLEDBuffer() const
    {
        return leds;
    }

    ~LEDStripGFX()
    {
        free(leds);
        leds = nullptr;
    }

    virtual size_t GetLEDCount() const
    {
        return NUM_LEDS;
    }
    
    inline uint16_t getPixelIndex(int16_t x, int16_t y) const
    {
        if (x & 0x01)
        {
          // Odd rows run backwards
          uint8_t reverseY = (_height - 1) - y;
          return (x * _height) + reverseY;
        }
        else
        {
          // Even rows run forwards
          return (x * _height) + y;
        }
    }

    inline CRGB getPixel(int16_t x) const
    {
        if (x >= 0 && x <= MATRIX_WIDTH * MATRIX_HEIGHT)
            return leds[x];
        else
            throw std::runtime_error(str_sprintf("Invalid index in getPixel: x=%d, NUM_LEDS=%d", x, NUM_LEDS).c_str());
    }

    inline CRGB getPixel(int16_t x, int16_t y) const
    {
        if (x >= 0 && x <= MATRIX_WIDTH && y >= 0 && y <= MATRIX_HEIGHT)
            return leds[xy(x, y)];
        else
            throw std::runtime_error(str_sprintf("Invalid index in getPixel: x=%d, y=%d, NUM_LEDS=%d", x, y, NUM_LEDS).c_str());
    }
};
