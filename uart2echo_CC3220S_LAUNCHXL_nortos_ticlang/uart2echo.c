/*
 * Copyright (c) 2020, Texas Instruments Incorporated
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
 *  ======== uart2echo.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART2.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    char input; // only one byte of RAM for the serial buffer
    unsigned char state; // only one byte of RAM for the state
    enum States {Start_State, Wait_State, ON_state, N_State, F_State, OFF_State}
    unsigned char numF;// Var to keep track of number of F's
    const char echoPrompt[] = "Echoing characters:\r\n";
    UART2_Handle uart;
    UART2_Params uartParams;
    size_t bytesRead;
    size_t bytesWritten = 0;
    uint32_t status     = UART2_STATUS_SUCCESS;

    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED pin */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    /* Create a UART where the default read and write mode is BLOCKING */
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 115200;

    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL)
    {
        /* UART2_open() failed */
        while (1) {}
    }

    /* Turn on user LED to indicate successful initialization */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    UART2_write(uart, echoPrompt, sizeof(echoPrompt), &bytesWritten);

    // Receive only one character at a time from the UART and it is not buffered.
    // Turn on an LED when a user types ON into the console.
    // Turn off an LED when a user types OFF into the console.



    /* Loop forever echoing */
    while (1)
    {
        bytesRead = 0;
        while (bytesRead == 0)
        {
            status = UART2_read(uart, &input, 1, &bytesRead);

            if (status != UART2_STATUS_SUCCESS)
            {
                /* UART2_read() failed */
                while (1) {}
            }
        }

        // This is where we will create state machine to controll lights
        switch(state) // Transitions
        {
        case Start_State: // Initial Transition
            numF = 0;
            state = Wait_State;
            break;
        case Wait_State:
            if(input == 'O')
            {
                state = O_state;
            }
            else
            {
                state = Wait_State;
            }
            break;
        case O_State:
            if(input == 'N')
            {
                state = ON_state;
            }
            else if (input == 'F')
            {
                state = F_state;
            }
            else
            {
                state = Wait_State;
            }
            break;
        case ON_state:
            state = Wait_State;
            break;
        case F_state:
            else if (input == 'F')
            {
                numF++;
                if(numF == 2)
                {
                    state = OFF_State;
                }
                state = F_state;
            }
            else
            {
                state = Wait_State;
            }
         case OFF_State:
             state = Wait_State;
             break;
         default:
             state = Start_State;
             break;
        }

        switch(state) // Actions
        {
        case Wait_State:
            numF = 0;
            break;
        case O_state:
            break;
        case ON_state:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            break;
        case F_state:
            break;
        case OFF_State:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            break;
        default:
            break;
        }


        bytesWritten = 0;
        while (bytesWritten == 0)
        {
            status = UART2_write(uart, &input, 1, &bytesWritten);

            if (status != UART2_STATUS_SUCCESS)
            {
                /* UART2_write() failed */
                while (1) {}
            }
        }
    }
}
