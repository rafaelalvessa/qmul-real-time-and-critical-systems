#include <MKL25Z4.H>
#include "pelican.h"

// -----------------------------------
// Initialisation routines
//   Init_Button: Button GPIO i/p
//   Init_GPIO_Led: LED GPIO o/p
//   Init_ADC: initialise ADC for current measurement
//   Init_SysTick: make SysTick tick every ms
// -----------------------------------

// Initialse Port D BUTTON_POS (pin 6) as an input, with an interrupt.
void Init_Button(void) {
    SIM->SCGC5 |=  SIM_SCGC5_PORTD_MASK; // Enable clock for port D.

    /* Select GPIO and DISable pull-up resistors and interrupts on falling edges
     * for pins connected to switches */
    PORTD->PCR[BUTTON_POS] &= ~PORT_PCR_PS_MASK;
    PORTD->PCR[BUTTON_POS] |= PORT_PCR_MUX(1) | PORT_PCR_IRQC(0x0a);

    /* Set port D switch bit to inputs. */
    PTD->PDDR &= ~MASK(BUTTON_POS);

    /* Enable Interrupts. */
    NVIC_SetPriority(PORTD_IRQn, 128); // 0, 64, 128 or 192
    NVIC_ClearPendingIRQ(PORTD_IRQn);
    NVIC_EnableIRQ(PORTD_IRQn);
}


// Initialise GPIO o/p pins on Port E.
void Init_GPIO_Led(void) {
    // Enable clock to port E
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Make PTE pins GPIO.
    PORTE->PCR[RED_POS] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[RED_POS] |= PORT_PCR_MUX(1);
    PORTE->PCR[AMB_POS] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[AMB_POS] |= PORT_PCR_MUX(1);
    PORTE->PCR[GRE_POS] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[GRE_POS] |= PORT_PCR_MUX(1);
    PORTE->PCR[DWL_POS] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[DWL_POS] |= PORT_PCR_MUX(1);
    PORTE->PCR[WLK_POS] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[WLK_POS] |= PORT_PCR_MUX(1);
    PORTE->PCR[WAI_POS] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[WAI_POS] |= PORT_PCR_MUX(1);

    // Set PTE pins to output
    PTE->PDDR |= MASK(RED_POS)
        |  MASK(AMB_POS)
        |  MASK(GRE_POS)
        |  MASK(DWL_POS)
        |  MASK(WLK_POS)
        |  MASK(WAI_POS);
}

//  Initialise ADC .
void Init_ADC(void) {
    // Enable clock to ports B.
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    // Ensure no pull-up / pull-down selected.
    PORTB->PCR[ADCPOS] &= ~PORT_PCR_PS_MASK;

    // Enable clock to ADC.
    SIM->SCGC6 |= (1UL << SIM_SCGC6_ADC0_SHIFT);

    // Set the ADC0_CFG1 to 0xB5, which is 1011 0101
    //   1 --> low power conversion
    //   01 --> ADIV is divide by 2
    //   1 --> ADLSMP is long sample time
    //   01 --> MODE is 12 bit conversion
    //   01 --> ADIClK is bus clock / 2
    ADC0->CFG1 = 0xB5 ;

    // Set the ADC0_SC2 register to 0
    //   0 --> DATRG - s/w trigger
    //   0 --> ACFE - compare disable
    //   0 --> ACFGT - n/a when compare disabled
    //   0 --> ACREN - n/a when compare disabled
    //   0 --> DMAEN - DMA is disabled
    //   00 -> REFSEL - defaults V_REFH and V_REFL selected
    ADC0->SC2 = 0 ;
}

// Configure SysTick to interrupt every millisecond.
void Init_SysTick(void) {
    uint32_t r = 0;
    r = SysTick_Config(SystemCoreClock / 1000);

    // Check return code for errors.
    if (r != 0) {
        // Error Handling.
        while(1);
    }
}


// Combined initialisation.
void PelicanConfig(void) {
    Init_ADC();
    Init_Button();
    Init_GPIO_Led();
    Init_SysTick();
}

// -----------------------------------
// GPIO outputs
// -----------------------------------
// Assumes all GPIO o/ps are in PortE.
const uint32_t SignalPos[6] = {
    RED_POS,
    AMB_POS,
    GRE_POS,
    DWL_POS,
    WLK_POS,
    WAI_POS
};

// Set a signal.
void SignalSet(enum PelicanSignal ps) {
    // Turn-on PTE pin.
    PTE->PSOR |= MASK(SignalPos[ps]);
}

// Clear a signal.
void SignalReset(enum PelicanSignal ps) {
    // Turn-off PTE pin.
    PTE->PCOR |= MASK(SignalPos[ps]);
}

// -----------------------------------
// Measurement
// -----------------------------------
unsigned Measure(void) {
    unsigned res = 0;

    // Write to ADC0_SC1A
    //   0 --> AIEN Conversion interrupt diabled
    //   0 --> DIFF single end conversion
    //   01000 --> ADCH, selecting AD8
    ADC0->SC1[0] = ADC_CHANNEL; // Writing to this clears the COCO flag.

    // Test the conversion complete flag, which is 1 when completed.
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)); // Empty loop.

    // Read results from ADC0_RA as an unsigned integer.
    res = ADC0->R[0] ; // Reading this clears the COCO flag.
    return res;
}

// -----------------------------------
// Button test
// -----------------------------------
//
// The button is debounced. This is likely not needed, assuming cyclic design
// and only one button access per cycle.

volatile int SysTickCounter = 0;
volatile int ButtonCounter = 0;
volatile int ButtonPressed = 0;

// Tests whether the button is pressed.
//   Test the button. If set, then clear the variable set by the interrupt.
//   Return: Button status
int ButtonTestReset(void) {
    unsigned res = ButtonPressed ;

    if (res) {
        ButtonPressed = 0;
    }

    return res;
}

/*
 * Interrupt Handler:
 *
 *   - Clear the pending request.
 *   - Test the bit for pin BUTTON_POS (6) to see if it generated the interrupt.
 */
void PORTD_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTD_IRQn);

    if ((PORTD->ISFR & MASK(BUTTON_POS))) {
        if (ButtonCounter == 0 && ButtonPressed == 0) {
            ButtonPressed = 1;
            ButtonCounter = BUTTON_DELAY;
            // Otherwise ignore it.
        }
    }

    // Clear status flags .
    PORTD->ISFR = 0xffffffff;
}

// -----------------------------------
// SysTick
// -----------------------------------

// Wait for the SysTick counter to expire, then reset it.
//   Param: number of ticks to set counter
void WaitSysTickCounter(int ticks) {
    while (SysTickCounter > 0);
    SysTickCounter = ticks;
}

// This function handles SysTick Handler.
// Decrement the counters that are greater than zero.
void SysTick_Handler(void) {
    if (SysTickCounter > 0x00) { // Check counter not already zero.
        SysTickCounter--; // Decrement towards zero.
    }

    if (ButtonCounter > 0x00) { // Check counter not already zero.
        ButtonCounter--; // Decrement towards zero.
    }
}
