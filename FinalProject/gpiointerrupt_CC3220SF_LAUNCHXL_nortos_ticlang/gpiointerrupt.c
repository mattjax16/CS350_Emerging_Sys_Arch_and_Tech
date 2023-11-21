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



/*
 *  ======== Imports ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART2.h>
#include <ti/drivers/Timer.h>




/*
 *  ======== Global Variables ========
 */

// Driver Handles - Global variables
Timer_Handle timer0;
volatile unsigned char TimerFlag = 0;
unsigned int seconds_since_restart = 0; // keeps track of seconds since restart


// I2C Global Variables
static const struct {
       uint8_t address;
       uint8_t resultReg;
char *id;
} sensors[3] = {
                { 0x48, 0x0000, "11X" },
                { 0x49, 0x0000, "116" },
                { 0x41, 0x0001, "006" }
 };


uint8_t txBuffer[1];
uint8_t rxBuffer[2];
I2C_Transaction i2cTransaction;

// Driver Handles - Global variables
I2C_Handle i2c;


#define DISPLAY(x) UART2_write(uart, &output, x, NULL);

// UART Global Variables
char output[64];
int bytesToSend;

// Driver Handles - Global variables
UART2_Handle uart;

// Temperature Globals
int16_t set_point; //Thermostat user set temperature
int16_t temperature; //Temperature reading from sensor
bool heater_on; //Heater on if true it is off if false

//Task object
typedef struct task{
    int state; // Current state of the task
    unsigned long period; // Rate at which the task should tick
    unsigned long elapsedTime; // Time since task's previous tick
    int (*TickFcn)(int); // Function to call for task's tick
} task;



//Task Timing Variables
const unsigned int number_of_tasks = 3;

//Task Scheduler Global Variables
task tasks[3]; //task objects

const uint32_t taskPeriodGCD = 100000; // IN Microseconds (100ms)
const uint32_t periodCheckButton = 200000; // IN Microseconds (200ms)
const uint32_t periodCheckTemp = 500000; // IN Microseconds (500ms)
const uint32_t periodOutputToServer = 1000000; // IN Microseconds (1000ms) (1s)

// Globals for increasing and decreasing set point
bool increase_set_point = 0;
bool decrease_set_point = 0;


/// Enums for differnt states of the different state machines

enum ButtonState{
    Button_State_Init,
    Button_State_Idle,
    Button_State_Increase,
    Button_State_Decrease
}Button_State;


enum HeaterState{
    Heater_State_Init,
    Heater_State_OFF,
    Heater_State_ON
}Heater_State;

/*
 * ======== Needed Function Prototypes ========
 */

int16_t readTemp(void);

void taskScheduler(void);



/*
 *  ======== Callbacks (interrupts) ========
 */


/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    // Increases the set point
    increase_set_point = 1;
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
    // Decreases the set point
    decrease_set_point = 1;
}



// The timer call back function to set the Timer flag to run the task Scheduler
// was initially going to be in this function put was cousing problems tripping daultISR
void timerCallback(Timer_Handle myHandle, int_fast16_t status) {
    TimerFlag = 1;
}





/*
 * ======== Drivier Inits ========
 */


void initTimer(void) {
    Timer_Params params;
     // Init the driver
     Timer_init();
     // Configure the driver
     Timer_Params_init(&params);
     params.period = taskPeriodGCD; // go off every 100 ms
     params.periodUnits = Timer_PERIOD_US;
     params.timerMode = Timer_CONTINUOUS_CALLBACK;
     params.timerCallback = timerCallback;

     // Open the driver
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


void initUART(void) {
     UART2_Params uart2Params;
     // Configure the driver
     UART2_Params_init(&uart2Params);
     uart2Params.readReturnMode = UART2_ReadReturnMode_FULL;
     uart2Params.baudRate = 115200;
     // Open the driver
     uart = UART2_open(CONFIG_UART2_0, &uart2Params);
     if (uart == NULL) {
         /* UART_open() failed */
         while (1);
     }
}



// call initUART() before this function
void initI2C(void) {
    int8_t i, found;
    I2C_Params i2cParams;
    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Init the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL) {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }
    DISPLAY(snprintf(output, 32, "Passed\n\r"))

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses
    /* Common I2C transaction setup */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 0;
    found = false;

    for (i=0; i<3; ++i) {
        i2cTransaction.targetAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;
        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))

        if (I2C_transfer(i2c, &i2cTransaction)){
            DISPLAY(snprintf(output, 64, "Found\n\r"))
            found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }

    if(found) {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.targetAddress))
    }
    else {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}


// function to init heater valuer
void initHeaterVars(void){
    // Get the temperature
    temperature = readTemp();

    // If the temperature is at greater than 5 set the
    // default set point to 5 C below else set it to the current temperature
    if(temperature > 5){
        set_point = temperature - 5;
    }
    else{
        set_point = temperature;
    }

}

void setGPIO(void){
    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
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

    return;
}

/*
 * ======== Driver Functions ========
 */

int16_t readTemp(void) {
    int j;
    int16_t temperature = 0;
    i2cTransaction.readCount = 2;
    if (I2C_transfer(i2c, &i2cTransaction)) {
        /*
        * Extract degrees C from the received data;
        * see TMP sensor datasheet
        */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]); temperature *= 0.0078125;
        /*
        * If the MSB is set '1', then we have a 2's complement * negative value which needs to be sign extended
        */
    if (rxBuffer[0] & 0x80)
    {
        temperature |= 0xF000;
    }
    } else {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
            DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }
    return temperature;

}


int outputToServer(){
        // Sending:
        //  1.  ASCII decimal value of room temperature (00 - 99) degrees Celsius
        //  2.  ASCII decimal value of set-point temperature (00-99) degrees Celsius
        //  3.  ‘0’if heat is off,‘1’if heat is on
        //  4.
        DISPLAY( snprintf(output, 64, "<%02d, %02d, %d, %04d>\n\r", temperature, set_point, heater_on, seconds_since_restart));
        seconds_since_restart += 1; // adding plus 1 after because on boot a message is initially sent
                                    // It doesn't wait a second because all tasks initially fire
        return 0;
}


/*
 * ======== State Machines ========
 */


/// Button State Machine
int buttonStateMachine(int Button_State){

    //Transitions
    switch(Button_State){
        case Button_State_Init:
            //Initial Load, Set to Idle
            Button_State = Button_State_Idle;
            break;
        case Button_State_Idle:
            // Check that only one button is pressed
            // if both are pressed stay idle
            if(increase_set_point ^ decrease_set_point){
                // Now see which button was pressed
                if(increase_set_point){
                    Button_State = Button_State_Increase;
                }
                else if(decrease_set_point){
                    Button_State = Button_State_Decrease;
                }
            }
            break;
        case Button_State_Increase:
            // Check that only one button is pressed
            if(increase_set_point ^ decrease_set_point){
                // Now see which button was pressed
                if(increase_set_point){
                    Button_State = Button_State_Increase;
                }
                else if(decrease_set_point){
                    Button_State = Button_State_Decrease;
                }
            }
            // If both buttons or none of the buttons are pressed then go back to idle
            else{
                Button_State = Button_State_Idle;
            }
            break;
        case Button_State_Decrease:
           // Check that only one button is pressed
           if(increase_set_point ^ decrease_set_point){

               // Now see which button was pressed
               if(increase_set_point){
                   Button_State = Button_State_Increase;
               }
               else if(decrease_set_point){
                   Button_State = Button_State_Decrease;
               }
           }
           // If both buttons or none of the buttons are pressed then go back to idle
           else{
               Button_State = Button_State_Idle;
           }
           break;
        default:
            Button_State = Button_State_Idle; // set to idle in case of error
            break;
    }
    //Actions
    switch(Button_State){
        case Button_State_Init:
            increase_set_point = 0; // ensure flags are cleared
            decrease_set_point = 0; // ensure flags are cleared
            break;
        case Button_State_Idle:
            increase_set_point = 0; // ensure flags are cleared
            decrease_set_point = 0; // ensure flags are cleared
            break;
        case Button_State_Increase:
            set_point += 1; // increase the set point
            increase_set_point = 0; // reset the flag
            break;
        case Button_State_Decrease:
            set_point -= 1; // decrease the set point
            decrease_set_point = 0; // reset the flag
            break;
        default:
            break;
    }
    return Button_State;
}


// Heater State Machine
int heaterStateMachine(Heater_State){
    // Always get the temperature
    temperature = readTemp();

    // Transitions
    switch(Heater_State){
        case Heater_State_Init:
            // Decide whether the heater is on or not based on set point
            // if the temp is less than or equal to set point heater is in on state
            if(temperature <= set_point){
                Heater_State = Heater_State_ON;
            }
            // Else turn heater is in off state
            else{
                Heater_State = Heater_State_OFF;
            }
            break;
        case Heater_State_ON:
            // Decide whether the heater is on or not based on set point
            // if the temp is less than or equal to set point heater is in on state
            if(temperature <= set_point){
                Heater_State = Heater_State_ON;
            }
            // Else turn heater is in off state
            else{
                Heater_State = Heater_State_OFF;
            }
            break;
        case Heater_State_OFF:
            // Decide whether the heater is on or not based on set point
            // if the temp is less than or equal to set point heater is in on state
            if(temperature <= set_point){
                Heater_State = Heater_State_ON;
            }
            // Else turn heater is in off state
            else{
                Heater_State = Heater_State_OFF;
            }
            break;
        default:
            Heater_State = Heater_State_Init;
            break;
    }
    // Actions
    switch(Heater_State){
            case Heater_State_Init:
                break;
            case Heater_State_ON:
                // Turn the heater (set flag) and LED on
                heater_on = 1;
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
                break;
            case Heater_State_OFF:
                // Turn the heater (set flag) and LED off
                heater_on = 0;
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
                break;
            default:
                break;
        }
    return Heater_State;
}

/*
 * ======== Task Scheduler Functions ========
 */


// Task Loop Function
void taskScheduler(void) {
  for (int i = 0; i < number_of_tasks; ++i) {  //For loop to loop through tasks
     if ( tasks[i].elapsedTime >= tasks[i].period ) {  //Check timing intervals
        tasks[i].state = tasks[i].TickFcn(tasks[i].state);  //update tasks
        tasks[i].elapsedTime = 0;                         //reset elapsed time
     }
     tasks[i].elapsedTime += taskPeriodGCD;               //increment with GCD of all tasks
  }
  return;
}

/* Create Tasks Function */
void createTasks(){
    //Every 200ms Check button flags
    tasks[0].state = Button_State_Init;
    tasks[0].period = periodCheckButton;
    tasks[0].elapsedTime = tasks[0].period;
    tasks[0].TickFcn = &buttonStateMachine;


    //Every 500ms read temperature, update heater and LED
    tasks[1].state = Heater_State_Init;
    tasks[1].period = periodCheckTemp;
    tasks[1].elapsedTime = tasks[1].period;
    tasks[1].TickFcn = &heaterStateMachine;

    //Every 1000ms Output to Server (UART)
    tasks[2].state = 1;
    tasks[2].period = periodOutputToServer;
    tasks[2].elapsedTime = tasks[2].period;
    tasks[2].TickFcn = &outputToServer;
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */

    GPIO_init();
    initUART();
    initI2C();
    initTimer();

    // The GPIO configuration moved to clean up main function
    setGPIO();

    // init the heater variables
    initHeaterVars();

    // create the tasks
    createTasks();

    // Main loop
    while(1){
        while(!TimerFlag){}
        TimerFlag = 0;
        taskScheduler();

    }
    return (NULL);
}
