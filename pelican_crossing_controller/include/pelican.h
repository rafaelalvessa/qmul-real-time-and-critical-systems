#ifndef __PELICAN_H
#define __PELICAN_H

// Mask
#define MASK(x) (1UL << (x))

// --------------------------
// Configuration
// --------------------------
extern void PelicanConfig(void);

#define CYCLESYSTICK (50)  // STM cycle in SysTicks (ms).
#define CYCLESPERSEC (20)  // Implies 20 cycles per second.

// --------------------------
// GPIO Outputs
// --------------------------

// Pin positions on Port E.
#define RED_POS (3)
#define AMB_POS (4)
#define GRE_POS (5)
#define DWL_POS (21)
#define WLK_POS (22)
#define WAI_POS (23)

// Names for the 6 signals.
enum PelicanSignal {RED_S, AMBER_S, GREEN_S, DONTWALK_S, WALK_S, WAIT_S};

// Set a signal.
extern void SignalSet(enum PelicanSignal ps);

// Clear a signal.
extern void SignalReset(enum PelicanSignal ps);

// -----------------------------------
// Measurement
// -----------------------------------

// Freedom KL25Z ADC Channel.
#define ADC_CHANNEL (8) // On port B.
#define VREF (3.3) // Reference voltage.
#define ADCRANGE (0x0fff) // Maximum for a 12 bit conversion.

#define ADCPOS (0) // Pin number on port B.

//  Uses ADC to read the voltage on the prob point.
//     Returns raw value from ADC
extern unsigned Measure(void);

// --------------------------
// Button test
// --------------------------

#define BUTTON_DELAY (10) // Delay in SysTick cycle (ms).

// Switch is on port D for interrupt support.
#define BUTTON_POS (6)

// Tests whether the button is pressed.
//   Test the button. If set, then clear the variable set by the interrupt.
//   Return: Button status

extern int ButtonTestReset(void);

// -----------------------------------
// SysTick
// -----------------------------------

// Wait for the SysTick counter to expire and then reset the SysTick counter.
//   Param Number of ticks to set counter for
extern void WaitSysTickCounter(int ticks);

#endif
