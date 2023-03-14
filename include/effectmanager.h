//+--------------------------------------------------------------------------
//
// File:        EffectManager.h
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
//    Based on my original ESP32LEDStick project this is the class that keeps
//    track of internal efects, which one is active, rotating among them,
//    and fading between them.
//
//
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <memory>
#include <vector>
#include <math.h>

#include "effects/strip/misceffects.h"
#include "effects/strip/fireeffect.h"

#define MAX_EFFECTS 32

extern uint8_t g_Brightness;
extern uint8_t g_Fader;

// References to functions in other C files

void InitEffectsManager();
std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color);
std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color, CRGB color2);
extern DRAM_ATTR std::shared_ptr<GFXBase> g_ptrDevices[NUM_CHANNELS];
// EffectManager
//
// Handles keeping track of the effects, which one is active, asking it to draw, etc.

template <typename GFXTYPE>
class EffectManager
{
    LEDStripEffect **_ppEffects;
    size_t _cEffects;
    size_t _cEnabled;

    size_t _iCurrentEffect;
    uint _effectStartTime;
    uint _effectInterval;
    bool _bPlayAll;
    bool _bShowVU = true;
    CRGB lastManualColor = CRGB::Red;

    std::unique_ptr<bool[]> _abEffectEnabled;
    std::shared_ptr<GFXTYPE> * _gfx;
    std::shared_ptr<LEDStripEffect> _ptrRemoteEffect = nullptr;

public:
    static const uint csFadeButtonSpeed = 15 * 1000;
    static const uint csSmoothButtonSpeed = 60 * 1000;

    EffectManager(LEDStripEffect **pEffects, size_t cEffects, std::shared_ptr<GFXTYPE> *gfx)
        : _ppEffects(pEffects),
          _cEffects(cEffects),
          _cEnabled(0),
          _effectInterval(DEFAULT_EFFECT_INTERVAL),
          _bPlayAll(false),
          _gfx(gfx)
    {
        debugV("EffectManager Constructor");
        _iCurrentEffect = 0;
        _effectStartTime = millis();
        _abEffectEnabled = std::make_unique<bool[]>(cEffects);

        for (int i = 0; i < cEffects; i++)
            EnableEffect(i);
    }

    ~EffectManager()
    {
        ClearRemoteColor();
    }

    std::shared_ptr<GFXTYPE> operator[](size_t index) const
    {
        return _gfx[index];
    }

    // Must provide at least one drawing instance, like the first matrix or strip we are drawing on
    inline std::shared_ptr<GFXTYPE> graphics() const
    {
        return _gfx[0];
    }

    // ShowVU - Control whether VU meter should be draw.  Returns the previous state when set.

    virtual bool ShowVU(bool bShow)
    {
        bool bResult = _bShowVU;
        debugW("Setting ShowVU to %d\n", bShow);
        _bShowVU = bShow;

        // Erase any exising pixels since effects don't all clear each frame
        if (!bShow)
            _gfx[0]->setPixelsF(0, MATRIX_WIDTH, CRGB::Black);

        return bResult;
    }

    virtual bool IsVUVisible() const
    {
        return _bShowVU;
    }

#if ATOMLIGHT
    static const uint FireEffectIndex = 2; // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
    static const uint VUEffectIndex = 6;   // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
#elif FANSET
    static const uint FireEffectIndex = 1; // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
#elif BROOKLYNROOM
    static const uint FireEffectIndex = 2; // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
    static const uint VUEffectIndex = 6;   // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
#else
    static const uint FireEffectIndex = 0; // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
    static const uint VUEffectIndex = 0;   // Index of the fire effect in the g_apEffects table (BUGBUG hardcoded)
#endif

    // SetGlobalColor
    //
    // When a global color is set via the remote, we create a fill effect and assign it as the "remote effect"
    // which takes drawing precedence

    void SetGlobalColor(CRGB color)
    {
        debugW("Setting Global Color");

        CRGB oldColor = lastManualColor;
        lastManualColor = color;

        #if (USE_MATRIX)
                GFXBase *pMatrix = (*this)[0].get();
                pMatrix->setPalette(CRGBPalette16(oldColor, color));
                pMatrix->PausePalette(true);
        #else
            std::shared_ptr<LEDStripEffect> effect;

            if (color == CRGB(CRGB::White))
                effect = std::make_shared<ColorFillEffect>(CRGB::White, 1);
        else

            #if ENABLE_AUDIO
                #if SPECTRUM
                    effect = GetSpectrumAnalyzer(color, oldColor);
                #else
                    effect = std::make_shared<MusicalPaletteFire>("Custom Fire", CRGBPalette256(CRGB::Black, color, CRGB::Yellow, CRGB::White), NUM_LEDS, 1, 8, 50, 1, 24, true, false);
                #endif
            #else
                effect = std::make_shared<PaletteFlameEffect>("Custom Fire", CRGBPalette256(CRGB::Black, color, CRGB::Yellow, CRGB::White), NUM_LEDS, 1, 8, 50, 1, 24, true, false);
            #endif

            if (effect->Init(g_aptrDevices))
            {
                _ptrRemoteEffect = effect;
                StartEffect();
            }
        #endif
    }

    void ClearRemoteColor()
    {
        _ptrRemoteEffect = nullptr;

        #if (USE_MATRIX)
            LEDMatrixGFX *pMatrix = (LEDMatrixGFX *)(*this)[0].get();
            pMatrix->PausePalette(false);
        #endif
    }

    void StartEffect()
    {
        #if USE_MATRIX
            LEDMatrixGFX *pMatrix = (LEDMatrixGFX *)(*this)[0].get();
            pMatrix->SetCaption(_ppEffects[_iCurrentEffect]->FriendlyName(), 3000);
            pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());
        #endif

        // If there's a temporary effect override from the remote control active, we start that, else
        // we start the current regular effect
        
        if (_ptrRemoteEffect)
            _ptrRemoteEffect->Start();
        else
            _ppEffects[_iCurrentEffect]->Start();

        _effectStartTime = millis();
    }

    void EnableEffect(size_t i)
    {
        if (i >= _cEffects)
        {
            debugW("Invalid index for EnableEffect");
            return;
        }

        if (!_abEffectEnabled[i])
        {
            _abEffectEnabled[i] = true;

            if (_cEnabled < 1)
            {
                ClearRemoteColor();
            }
            _cEnabled++;
        }
    }

    void DisableEffect(size_t i)
    {
        if (i >= _cEffects)
        {
            debugW("Invalid index for DisableEffect");
            return;
        }

        if (_abEffectEnabled[i])
        {
            _abEffectEnabled[i] = false;

            _cEnabled--;
            if (_cEnabled < 1)
            {
                SetGlobalColor(CRGB::Black);
            }
        }
    }

    bool IsEffectEnabled(size_t i) const
    {
        if (i >= _cEffects)
        {
            debugW("Invalid index for IsEffectEnabled");
            return false;
        }
        return _abEffectEnabled[i];
    }

    void PlayAll(bool bPlayAll)
    {
        _bPlayAll = bPlayAll;
    }

    uint GetInterval() const
    {
        if (_effectInterval == 0)
            return std::numeric_limits<uint>::max();
        return _effectInterval;
    }

    void SetInterval(uint interval)
    {
        _effectInterval = interval;
    }

    const LEDStripEffect *const *EffectsList() const
    {
        return _ppEffects;
    }

    const size_t EffectCount() const
    {
        return _cEffects;
    }

    const size_t EnabledCount() const
    {
        return _cEnabled;
    }

    const size_t GetCurrentEffectIndex() const
    {
        return _iCurrentEffect;
    }

    LEDStripEffect *GetCurrentEffect() const
    {
        return _ppEffects[_iCurrentEffect];
    }

    const String & GetCurrentEffectName() const
    {
        if (_ptrRemoteEffect)
            return _ptrRemoteEffect->FriendlyName();

        return _ppEffects[_iCurrentEffect]->FriendlyName();
    }

    // Change the current effect; marks the state as needing attention so this get noticed next frame

    void SetCurrentEffectIndex(size_t i)
    {
        if (i >= _cEffects)
        {
            debugW("Invalid index for SetCurrentEffectIndex");
            return;
        }
        _iCurrentEffect = i;
        _effectStartTime = millis();
        StartEffect();
    }

    uint GetTimeRemainingForCurrentEffect() const
    {
        // If the Interval is set to zero, we treat that as an infinite interval and don't even look at the time used so far

        if (GetTimeUsedByCurrentEffect() > GetInterval())
            return 0;

        return GetInterval() - GetTimeUsedByCurrentEffect();
    }

    uint GetTimeUsedByCurrentEffect() const
    {
        return millis() - _effectStartTime;
    }

    void CheckEffectTimerExpired()
    {
        // If interval is zero, the current effect never expires

        if (_effectInterval == 0)
            return;

        if (millis() - _effectStartTime >= GetInterval()) // See if its time for a new effect yet
        {
            debugV("%ldms elapsed: Next Effect", millis() - _effectStartTime);
            NextEffect();
            debugV("Current Effect: %s", GetCurrentEffectName());
        }
    }

    void NextPalette()
    {
        auto g = _gfx[0].get();
        g->CyclePalette(1);
    }

    void PreviousPalette()
    {
        auto g = _gfx[0].get();
        g->CyclePalette(-1);
    }
    // Update to the next effect and abort the current effect.

    void NextEffect()
    {
        do
        {
            _iCurrentEffect++; //   ... if so advance to next effect
            _iCurrentEffect %= EffectCount();
            _effectStartTime = millis();
        } while (0 < _cEnabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));
        StartEffect();
    }

    // Go back to the previous effect and abort the current one.

    void PreviousEffect()
    {
        do
        {
            if (_iCurrentEffect == 0)
                _iCurrentEffect = EffectCount() - 1;

            _iCurrentEffect--;
            _effectStartTime = millis();
        } while (0 < _cEnabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));
        StartEffect();
    }

    bool Init()
    {

        for (int i = 0; i < _cEffects; i++)
        {
            debugV("About to init effect %s", _ppEffects[i]->FriendlyName());
            if (false == _ppEffects[i]->Init(_gfx))
            {
                debugW("Could not initialize effect: %s\n", _ppEffects[i]->FriendlyName());
                return false;
            }
            debugV("Loaded Effect: %s", _ppEffects[i]->FriendlyName());

            // First time only, we ensure the data is cleared

            //_ppEffects[i]->setAll(0,0,0);
        }
        debugV("First Effect: %s", GetCurrentEffectName());
        return true;
    }

    // EffectManager::Update
    //
    // Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

    void Update()
    {
        if ((_gfx[0])->GetLEDCount() == 0)
            return;

        const float msFadeTime = EFFECT_CROSS_FADE_TIME;

        CheckEffectTimerExpired();

        // If a remote control effect is set, we draw that, otherwise we draw the regular effect

        if (_ptrRemoteEffect)
            _ptrRemoteEffect->Draw();
        else
            _ppEffects[_iCurrentEffect]->Draw(); // Draw the currently active effect

        // If we do indeed have multiple effects (BUGBUG what if only a single enabled?) then we
        // fade in and out at the appropriate time based on the time remaining/used by the effect

        if (EffectCount() < 2)
        {
            g_Fader = 255;
            return;
        }

        if (_effectInterval == 0)
        {
            g_Fader = 255;
            return;
        }

        int r = GetTimeRemainingForCurrentEffect();
        int e = GetTimeUsedByCurrentEffect();

        if (e < msFadeTime)
        {
            g_Fader = 255 * (e / msFadeTime); // Fade in
        }
        else if (r < msFadeTime)
        {
            g_Fader = 255 * (r / msFadeTime); // Fade out
        }
        else
        {
            g_Fader = 255; // No fade, not at start or end
        }
    }
};

extern std::unique_ptr<EffectManager<GFXBase>> g_aptrEffectManager;
