#include <MKL25Z4.H>
#include "gpio_defs.h"
#include "switches.h"

volatile int counter = 0;
int state = 0;

// Demonstration of digital input using an interrupt.
// Use RGB LED on Freedom board.

void Delay(unsigned int time_del) {
    // Delay is about 1 millisecond * time_del.
    volatile int t;

    while (time_del--) {
        for (t = 4800; t > 0; t--);
    }
}

// Each LED corresponds to a bit on a port:
//
//   - Red LED connected to Port B (PTB), bit 18 (RED_LED_POS).
//   - Green LED connected to Port B (PTB), bit 19 (GREEN_LED_POS).
//   - Blue LED connected to Port D (PTD), bit 1 (BLUE_LED_POS).
//
// Active-Low outputs: Write a 0 to turn on an LED.

// Turning LEDs on and off:
//
//   - Turn on one LED: PTx->PDOR = ~ MASK(yyy_LED_POS);
//   - Turn on two LEDs: PTx->PDOR = ~ (MASK(yyy_LED_POS) | MASK(zzz_LED_POS));
//   - Turn all LEDs off: PTx->PDOR = 0xFFFFFFFF;

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
    // Configuration steps:
    //
    //   1. Enable clock to GPIO ports
    //   2. Enable GPIO ports
    //   3. Set GPIO direction to output
    //   4. Ensure LEDs are off

    // Enable clock to ports B and D.
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK;

    // Make 3 pins GPIO.
    PORTB->PCR[RED_LED_POS] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[RED_LED_POS] |= PORT_PCR_MUX(1);
    PORTB->PCR[GREEN_LED_POS] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[GREEN_LED_POS] |= PORT_PCR_MUX(1);
    PORTD->PCR[BLUE_LED_POS] &= ~PORT_PCR_MUX_MASK;
    PORTD->PCR[BLUE_LED_POS] |= PORT_PCR_MUX(1);

    // Set ports to outputs.
    PTB->PDDR |= MASK(RED_LED_POS) | MASK(GREEN_LED_POS);
    PTD->PDDR |= MASK(BLUE_LED_POS);

    // Turn off LEDs.
    PTB->PSOR = MASK(RED_LED_POS) | MASK(GREEN_LED_POS);
    PTD->PSOR = MASK(BLUE_LED_POS);
    // End of configuration code.

    // Initialise the switch (or button) to generate an interrupt.
    init_switch();

    while(1) {
        switch(state) {
            // LED off
            case 0:
                if(buttonPress) {
                    buttonPress = 0;
                    PTB->PDOR = ~ MASK(RED_LED_POS);
                    counter = 0;
                    state = 1;
                }

                break;

                // Flash on
            case 1:
                if(buttonPress) {
                    buttonPress = 0;
                    state = 3;
                } else {
                    Delay(500);
                    PTB->PDOR = 0xFFFFFFFF;
                    state = 2;
                }

                break;

                // Flash off
            case 2:
                if(counter == 9) {
                    PTB->PDOR = 0xFFFFFFFF;
                    state = 0;
                } else if(buttonPress) {
                    buttonPress = 0;
                    state = 0;
                } else {
                    Delay(500);
                    PTB->PDOR = ~ MASK(RED_LED_POS);
                    counter++;
                    state = 1;
                }

                break;

                // Flash on B (Last flash)
            case 3:
                Delay(500);
                PTB->PDOR = 0xFFFFFFFF;
                break;
        }
    }
}
