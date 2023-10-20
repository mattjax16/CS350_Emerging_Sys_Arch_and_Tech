/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"

// Here is a state machine which will be called every 500000 us.
// it Continuously blink SOS in Morse code on the green and red LEDs.
// If a button is pushed, toggle the message between SOS and OK. Pushing the button in the
// middle of a message should NOT change the message until it is complete.
// For example, if you push the button while the ‘O’ in SOS is being blinked out,
// the message will not change to OK until after the SOS message is completed.
// Dot = red LED on for 500ms
// Dash = green LED on for 1500ms
// 3*500ms between characters (both LEDs off)
// 7*500ms between words (both LEDs off), for example SOS 3500ms SOS
// when the button is pushed it changes the value of the bool display_sos



// Here are 6 states for the state machine: S, O, K, Between_Morse, Between_Characters and Between_Words
enum machine_state {S, O, K, Between_Morse, Between_Characters, Between_Words};

bool display_sos = true;

machine_state current_state = S;
machine_state previous_state;

int morse_counter = 0;
int s_counter = 0;



#include <ti/drivers/Timer.h>
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    // Transitions
    switch(current_state){
        case S:
            // first see if the 3 dots have been displayed
            if(morse_counter == 3){
                // if so then see if it is the last S
                if(s_counter == 2){
                    // if so then go to the next state
                    previous_state = S;
                    current_state = Between_Words;
                    // reset the counters
                    morse_counter = 0;
                    s_counter = 0;
                }
                else{
                    // if not then go to the next state
                    previous_state = S;
                    current_state = Between_Characters;
                    // reset the counter
                    morse_counter = 0;
                    // increment the s counter
                    s_counter++;
                }
                // reset the counter
                morse_counter = 0;
            }
            else{
            // send to between morse
            previous_state = S;
            current_state = Between_Morse;
            }
            break;
        case O:
            // first see if the 3 dashes have been displayed
            if(morse_counter == 3){
                // if so then go to the next state
                previous_state = O;
                current_state = Between_Characters;
                // reset the counter
                morse_counter = 0;
            }
            else{
            // send to between morse
            previous_state = O;
            current_state = Between_Morse;
            }
            break;
        case K:
            previous_state = K;
            current_state = Between_Words;
            break;
        case Between_Morse:
            current_state = previous_state;
            break;
        case Between_Characters:
        // go to the character based on display_sos
            if(display_sos){
                if (previous_state == S){
                    current_state = O;
                }
                else{
                    current_state = S;
                }
            }
            else{
                current_state = K;
            }
            previous_state = Between_Characters;
            break;
        case Between_Words:
            // go to the character based on display_sos
            if(display_sos){
                current_state = S;
            }
            else{
                current_state = O;
            }
            previous_state = Between_Words;
            break;

        case default:
                break;
    }
    // Actions
    switch(current_state){
    case S:
        // turn on the red LED
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
        // turn off the green LED
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
        // increment the morse counter
        morse_counter++;
        // set the timer to 500ms
        Timer_setPeriod(myHandle,Timer_PERIOD_US ,500000);
        break;
    case O:
        // turn on the green LED
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
        // turn off the red LED
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        // increment the morse counter
        morse_counter++;
        // set the timer to 1500ms
        Timer_setPeriod(myHandle,Timer_PERIOD_US ,1500000);
        break;
    case K:
        // turn off the green LED
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
        // turn off the red LED
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        // set the timer to 500ms
                // set the timer to 500ms
        Timer_setPeriod(myHandle,Timer_PERIOD_US ,500000);

        break;
    case Between_Morse:
        // turn off the green LED
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
        // turn off the red LED
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        // set the timer to 500ms
        Timer_setPeriod(myHandle,Timer_PERIOD_US ,500000);
        break;

    case Between_Characters:
        // turn off the green LED
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
        // turn off the red LED
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        // set the timer to 500ms
        Timer_setPeriod(myHandle,Timer_PERIOD_US ,1500000);
        break;

    case Between_Words:
        // turn off the green LED
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
        // turn off the red LED
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        // set the timer to 3000ms so in total it is 3500ms since there was a 500 ms delay between the last character
        // and the word
        Timer_setPeriod(myHandle,Timer_PERIOD_US ,3000000);
        break;
    }
}
void initTimer(void){
    Timer_Handle timer0;
    Timer_Params params;
    Timer_init();
    Timer_Params_init(&params);
    params.period = 500000;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;





    timer0 = Timer_open(CONFIG_TIMER_0, &params);
    if (timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }
    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    /* Toggle an LED */
//    GPIO_toggle(CONFIG_GPIO_LED_0);
    display_sos = !display_sos;
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Toggle an LED */
//    GPIO_toggle(CONFIG_GPIO_LED_1);
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1)
    {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    return (NULL);
}
