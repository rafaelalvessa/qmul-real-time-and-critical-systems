#include <MKL25Z4.H>
#include "gpio_defs.h"
#include "adc_defs.h"

// Demonstration of simple ADC.
// Use ADC0_SE8, PTB0, J10, pin 2.
// Use RGB LED on Freedom board.

/*
 * Simple and imprecise delay function.
 */
void Delay(unsigned int time_del) {
    // Delay is about 1 millisecond * time_del.
    volatile int t;

    while (time_del--) {
        for (t = 4800; t > 0; t--);
    }
}

// Cycle through the colours red, gren and blue.
void redGreenBlue(void) {
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
    //   - Turn on two LEDs: PTx->PDOR = ~ ( MASK(yyy_LED_POS) |
    //           MASK(zzz_LED_POS) );
    //   - Turn all LEDs off: PTx->PDOR = 0xFFFFFFFF;

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

// Initialise on board LEDs.
void Init_LED() {
    // Configuration steps:
    //
    //   1. Enable pins as GPIO ports
    //   2. Set GPIO direction to output
    //   3. Ensure LEDs are off

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
}

// Initialise ADC.
void Init_ADC() {
    // Set the ADC0_CFG1 to 0x9C, which is 1001 1100.
    //   1 --> low power conversion
    //   00 --> ADIV is 1, no divide
    //   1 --> ADLSMP is long sample time
    //   11 --> MODE is 16 bit conversion
    //   01 --> ADIClK is bus clock / 2
    ADC0->CFG1 = 0x9C;

    // Set the ADC0_SC2 register to 0.
    //   0 --> DATRG - s/w trigger
    //   0 --> ACFE - compare disable
    //   0 --> ACFGT - n/a when compare disabled
    //   0 --> ACREN - n/a when compare disabled
    //   0 --> DMAEN - DMA is disabled
    //   00 -> REFSEL - defaults V_REFH and V_REFL selected
    ADC0->SC2 = 0;
}

unsigned Measure(void) {
    unsigned res = 0 ;

    // Write to ADC0_SC1A.
    //   0 --> AIEN Conversion interrupt diabled
    //   0 --> DIFF single end conversion
    //   01000 --> ADCH, selecting AD8
    ADC0->SC1[0] = ADC_CHANNEL; // Writing to this clears the COCO flag.

    // Test the conversion complete flag, which is 1 when completed.
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)); // Empty loop.

    // Read results from ADC0_RA as an unsigned integer.
    res = ADC0->R[0]; // Reading this clears the COCO flag.

    return res;
}

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/

// Declare volatile to ensure changes seen in debugger.
volatile float measured_voltage; // Scaled value.
volatile unsigned res; // Raw value.

int main (void) {
    // Configuration steps:
    //
    //   1. Enable clock to GPIO ports
    //   2. Enable clock to GPIO ports
    //   3. Set GPIO direction to output
    //   4. Ensure LEDs are off

    // Enable clock to ports B and D.
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK |
            SIM_SCGC5_PORTE_MASK;

    // Enable clock to ADC.
    SIM->SCGC6 |= (1UL << SIM_SCGC6_ADC0_SHIFT);

    // Initialise LED.
    Init_LED();

    // Initialise ADC.
    Init_ADC();

    // End of configuration code.

    while (1) {
        // This flashes the lights which is good to show it is working.
        redGreenBlue();

        // Measure the voltage.
        res = Measure();

        // Scale to an actual voltage, assuming VREF accurate.
        measured_voltage = VREF * res / ADCRANGE;
    }
}
