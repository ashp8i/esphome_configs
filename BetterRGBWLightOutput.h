/*  A Better RGBW(W) Output

    Based on/inspired by user *envy* at https://github.com/esphome/feature-requests/issues/212#issuecomment-498036079

    Why?:

        I have RGBWW (RGB + Cool-White + Warm-White) LED bulbs.  After finding the correct PWM GPIOs to control each channel
        and setting them up with esphome and HomeAssistant, I noticed the controls in HA were weird.

        HA presented a brightness slider, colour temperature slider, 'white value' slider and an RGB picker.

        The 'white value' and 'brightness' only worked to brighten/dim the CW/WW and RGB LEDs respectively.  I wanted a single
        'brightness' slider that worked no matter if the bulb was showing white or colour.

    How?:

        This output checks what the desired state is.  If the RGB levels are equal, then the code turns off the RGB
        LEDs and turns on the Cool and Warm LEDs.  These levels set the white temperature, but not the brightness.

        So, we also grab the brightness setting and multiply the CW/WW levels by it to get final levels for CW/WW.

        If the desired RGB levels differ from eachother, then we know we want a coloured light.  So, the code 
        turns off the CW/WW LEDs and sets the RGB levels as desired.  We don't do a brightness multiplication here,
        since that already seems to have been applied.



*/


#pragma once

#include "esphome.h"

class BetterRGBWLightOutput : public Component, public LightOutput {
    public:
    BetterRGBWLightOutput(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *cold_white, FloatOutput *warm_white)
    {
        red_ = red;
        green_ = green;
        blue_ = blue;
	cold_white_ = cold_white;
        warm_white_ = warm_white;
    }

    LightTraits get_traits() override {
        auto traits = LightTraits();
        traits.set_supports_brightness(true);
        traits.set_supports_rgb(true);
        traits.set_supports_color_temperature(true);
        traits.set_supports_color_temperature(true);
        traits.set_max_mireds(370);
        traits.set_min_mireds(166);
        return traits;
    }

    void write_state(LightState *state) override {
        float red, green, blue, cold_white, warm_white, brightness;
        state->current_values_as_rgbww(&red, &green, &blue, &cold_white, &warm_white);

	state->current_values_as_brightness(&brightness);

        if (red == green && red == blue)
        {
            this->red_->set_level(0);
            this->green_->set_level(0);
            this->blue_->set_level(0);

	    this->cold_white_->set_level(cold_white * brightness);
            this->warm_white_->set_level(warm_white * brightness);
	}
        else
        {
            this->red_->set_level(red);
            this->green_->set_level(green);
            this->blue_->set_level(blue);
            this->cold_white_->set_level(0);
	    this->warm_white_->set_level(0);
        }
    }

    protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *cold_white_;
    FloatOutput *warm_white_;
};