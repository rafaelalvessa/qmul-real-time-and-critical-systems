#include <MKL25Z4.H>
#include "switches.h"

// Demonstration of digital input using an interrupt.

volatile unsigned buttonPress=0;

// Initialise Port D pin 6 as an input, with an interrupt.
void init_switch(void) {
    SIM->SCGC5 |=  SIM_SCGC5_PORTD_MASK; // Enable clock for port D.

    /* Select GPIO and enable pull-up resistors and interrupts on falling edges
     * for pins connected to switches
     */
    PORTD->PCR[BUTTON_POS] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK |
        PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0a);

    // Set port D switch bit to inputs.
    PTD->PDDR &= ~MASK(BUTTON_POS);

    // Enable Interrupts.
    NVIC_SetPriority(PORTD_IRQn, 128); // 0, 64, 128 or 192
    NVIC_ClearPendingIRQ(PORTD_IRQn);
    NVIC_EnableIRQ(PORTD_IRQn);
}

/*
 * Interrupt Handler:
 *
 *   - Clear the pending request.
 *   - Test the bit for pin 6 to see if it generated the interrupt.
 */
void PORTD_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTD_IRQn);

    if ((PORTD->ISFR & MASK(BUTTON_POS))) {
        buttonPress = 1;
    }

    // Clear status flags.
    PORTD->ISFR = 0xffffffff;
}
