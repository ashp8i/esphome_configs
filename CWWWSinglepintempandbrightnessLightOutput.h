/*  A CWWW plugin for Single Pin temperature and single pin brightness Globes.

The current inbuilt ESPHome CWWW plugin caters for a globe with 2 flout outputs, one for cool white and one for warm white,
the existing function then changes the values for these to mix colour and perform brightness corrections on theese values.

This ESPHome function is built for CWWW globes from tuya where one pin controlls CWWW with 0 being warm white and 1 being cool white,
the second pin controlls the brightness of the lamp.

Reported lamps that use this single pin method of temperature at point of creation;
SL6 Smart Bulb
Arlec Grid Connect GLD112HA / GLD110HA
BrilliantSmart 20695/05 Downlight CCT Light
Various Tuya rebadged smart globes.

This custom lightoutput changes the separate cwhite and wwhite channels from 1-0 to a combined 1-0, removes the brightness adjustment.
Sends the brightness to the brightness pin.


******* Ensure you set the correct min and max mireds below in this code (Line 79, 80). To work out the mireds you just divide 1000000 by the Kelvin value.
e.g
LED 5700K, 1000000/5700 = 175 mireds
LED 3000K, 1000000/3000 = 333 mireds

TODO:
Add additional function variables to take Kelvin from the yaml call and convert to mireds on the fly, would need to ensure the highest and lowest because mireds is backwards.

Usage:
Place this file into the ESPhome config directory

Include the following line into your ESPHOME yaml config under the board section at the same level.

*******
  includes:
  - CWWWSinglepintempandbrightnessLightOutput.h
*******

Include the following lines into your ESPHOME yaml config, changing pins if required.
*******
light:
  - platform: custom
    lambda: |-
      auto light_out = new CWWWSinglepintempandbrightnessLightOutput(id(output_brightness), id(output_color_temp));
      App.register_component(light_out);
      return {light_out};
    lights:
      - name: $display_name
        restore_mode: ALWAYS_ON

output:
  - platform: esp8266_pwm
    id: output_color_temp
    pin: GPIO13
  - platform: esp8266_pwm
    id: output_brightness
    pin: GPIO5
*******
*/


#pragma once

#include "esphome.h"

class CWWWSinglepintempandbrightnessLightOutput : public Component, public LightOutput {
    public:
    CWWWSinglepintempandbrightnessLightOutput(FloatOutput *pin_brightness, FloatOutput *pin_temp)
    {
      	pin_temp_ = pin_temp;
        pin_brightness_ = pin_brightness;
    }

    LightTraits get_traits() override {
        auto traits = LightTraits();
        traits.set_supports_brightness(true);
        traits.set_supports_rgb(false);
        traits.set_supports_rgb_white_value(false);
        traits.set_supports_color_temperature(true);
// ******* CHANGE BELOW VALUES ******* 
        traits.set_min_mireds(175); //5700K
        traits.set_max_mireds(333); //3000K
// ******* Nothing more the change ******* 
        return traits;
    }
    


    void write_state(LightState *state) override {
        float brightness, cwhite, wwhite, mathtemp;
        //Turn gamma correct to 0 due to these single pin temp bulbs containing inbuilt gamma correction.
        state->set_gamma_correct(0);
        //Get the Cool White and Warm white variables, these are returned for a two pin temperature light so each one is 0-1 float.
        state->current_values_as_cwww(&cwhite, &wwhite);
        //Brightness that we send to the brightness pin
      	state->current_values_as_brightness(&brightness);
        this->pin_brightness_->set_level(brightness);
        
        //The hacks start,
        //Anything left or right of center on the led temperature slider in hass has that temp led on 100% and fade is achieved by fading out the other temperature led.
        
        //Checking for a brightness shift on the cool white.
        //If found it sends the wwhite back to the correct value and sets cool white back on 100%.
        if ((cwhite == brightness) && (brightness < 1)){
           wwhite = wwhite/brightness;
           cwhite = 1;
        }
        //Checking for a brightness shift on the warm white.
        //If found it sends the coolwhite back to the correct value and sets warm white back on 100%.
        if ((wwhite == brightness) && (brightness < 1)){
            wwhite = 1;
            cwhite = cwhite/brightness;
        }
        //This converts the original CWWW single 1-0 warm white variable into a reversed combined warmwhite/coolwhite value. 
        //E.G
        //      WWHITE of 0.1 becomes 0.95
        //      WWHITE of 0.5 becomes 0.75
        //      WWHITE of 1.0 becomes 0.50
        if (cwhite == 1) {
            mathtemp = 1-(wwhite/2);
        }
        //This converts the original CWWW single 1-0 cool white variable into a combined warmwhite/coolwhite value. 
        //E.G
        //      CWHITE of 0.1 becomes 0.05
        //      CWHITE of 0.5 becomes 0.25
        //      CWHITE of 1.0 becomes 0.50
        if (wwhite ==1) {
            mathtemp = (cwhite/2);
        }

        this->pin_temp_->set_level(mathtemp);
        // Debug to see what is going wrong, uncomment if required, fairly intensive debug message.
        //ESP_LOGD("custom", "The value of mathtemp, cwhite, wwhite, brightness is: %f , %f, %f, %f", mathtemp,cwhite,wwhite,brightness);

	  }

    protected:
    FloatOutput *pin_temp_;
    FloatOutput *pin_brightness_;
    float cold_white_temperature_;
    float warm_white_temperature_;
};