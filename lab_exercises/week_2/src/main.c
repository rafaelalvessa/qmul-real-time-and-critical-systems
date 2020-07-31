#include <MKL25Z4.H>
#include "gpio_defs.h"

// Demonstration of simple digital output
// Use RGB LED on Freedom board

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

void redGreenBlue(void) {
    // Set just red on.
    PTB->PDOR = ~ MASK(RED_LED_POS);
    PTD->PDOR = 0xFFFFFFFF;

    // Wait for 500ms.
    Delay(500);

    // Set just green on.
    PTB->PDOR = ~ MASK(GREEN_LED_POS);
    PTD->PDOR = 0xFFFFFFFF;

    // Wait for 500ms.
    Delay(500);

    // Set just blue on.
    PTB->PDOR = 0xFFFFFFFF;
    PTD->PDOR = ~ MASK(BLUE_LED_POS);

    // Wait for 500ms.
    Delay(500);
}

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
    int ext1 = 5; // External LED 1
    int ext2 = 4; // External LED 2

    // Configuration steps:
    //
    //   1. Enable clock to GPIO ports
    //   2. Enable GPIO ports
    //   3. Set GPIO direction to output
    //   4. Ensure LEDs are off

    // Enable clock to ports B, D and E respectively.
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK |
        SIM_SCGC5_PORTE_MASK;

    // Make 5 pins GPIO.
    PORTB->PCR[RED_LED_POS] &= ~PORT_PCR_MUX_MASK;  // Red LED
    PORTB->PCR[RED_LED_POS] |= PORT_PCR_MUX(1); // Red LED
    PORTB->PCR[GREEN_LED_POS] &= ~PORT_PCR_MUX_MASK; // Green LED
    PORTB->PCR[GREEN_LED_POS] |= PORT_PCR_MUX(1); // Green LED
    PORTD->PCR[BLUE_LED_POS] &= ~PORT_PCR_MUX_MASK; // Blue LED
    PORTD->PCR[BLUE_LED_POS] |= PORT_PCR_MUX(1); // Blue LED

    PORTE->PCR[ext1] &= ~PORT_PCR_MUX_MASK; // External LED 1
    PORTE->PCR[ext1] |= PORT_PCR_MUX(1); // External LED 2

    PORTE->PCR[ext2] &= ~PORT_PCR_MUX_MASK; // External LED 1
    PORTE->PCR[ext2] |= PORT_PCR_MUX(1); // External LED 2

    // Set ports to outputs.
    PTB->PDDR |= MASK(RED_LED_POS) | MASK(GREEN_LED_POS);
    PTD->PDDR |= MASK(BLUE_LED_POS);

    PTE->PDDR |= MASK(ext1); // External LED 1
    PTE->PDDR |= MASK(ext2); // External LED 2

    // Turn off LEDs.
    PTB->PSOR = MASK(RED_LED_POS) | MASK(GREEN_LED_POS);
    PTD->PSOR = MASK(BLUE_LED_POS);

    PTE->PSOR = MASK(ext1); // External LED 1
    PTE->PSOR = MASK(ext2); // External LED 2

    // End of configuration code.

    // Code for flashing the LEDs.
    while (1) {
        PTE->PDOR = ~ MASK(ext1); // External LED 1

        // Wait for 500ms.
        Delay(500);

        PTE->PDOR = ~ MASK(ext2); // External LED 2

        // Wait for 500ms.
        Delay(500);
    }
}
