#include <MKL25Z4.H>
#include "pelican.h"

/* -------------------------------------
 * This project can be used to test the Pelican crossing hardware and as a
 * starting point for the final project.
 *
 * Version 1.1   2014-02-22
 *
 * The program cycles through the 6 lights, one at a time, in the order RED,
 * AMBER, GREEN, DONTWALK, WALK and WAIT, with 1 second on and 1 second off.
 *
 * The current probe point voltage is measured during the first MCYCLES (e.g. 5)
 * mini-cycle for each light.
 *
 * If the button is pressed, all lights go off for 2 seconds and then the cycle
 * starts again.
 * -------------------------------------
 */

//  Function 25Z        Port-Pin     Freedom
//  ------------------------------------------
//  Probe    ADC0_SE8   PTB0         J10, pin 2
//  Button              PTD6         J2, pin 17
//  RED                 PTE3         J9, pin 11
//  AMBER               PTE4         J9, pin 13
//  GREEN               PTE5         J9, pin 15
//  DONTWALK            PTE21        J10, pin 3
//  WALK                PTE22        J10, pin 5
//  WAIT                PTE23        J10, pin 7

// The cycle counter - in units of SysTick.
int cycleCounter;

// Determines if the RED or DONTWALK LED have failed.
int red_failure = 0;

// Determines if the AMBER LED has failed.
int amber_failure = 0;

// The number of cycles of the System Initiation state.
int init_counter = 1;

/*----------------------------------------------------------------------------*
  State Transition System
 *----------------------------------------------------------------------------*/

// --- Names of the states --- //
#define REDINIT 0 // RED init
#define AMBERINIT 1 // AMBER init
#define AMBERFAILUREINIT 2 // AMBER failure init
#define GREENINIT 3 // GREEN init
#define DONTWALKINIT 4 // DONTWALK init
#define WALKINIT 5 // WALK init
#define WAITINIT 6 // WAIT init
#define REDANDDONTWALK 7 // RED and DONTWALK
#define WAITFLASHINGON 8 // WAIT flashing on
#define WAITFLASHINGOFF 9 // WAIT flashing off
#define GREENON 10 // GREEN on
#define WAITON 11 // WAIT on
#define AMBERON 12 // AMBER on
#define AMBERFAILURE 13 // AMBER failure
#define REDON 14 // RED on
#define WALKON 15 // WALK on
#define DONTWALKON 16 // DONTWALK on
#define AMBERANDREDON 17 // AMBER and RED on
#define AMBERFAILUREANDREDON 18 //AMBER failure and RED on

// --- Timing delays in seconds --- //
#define T1 10
#define T2 10
#define T3 25
#define T4 15
#define T5 5
#define T6 30
#define T7 3

#define MCYCLES (5) // Number of ADC measurements.
#define HIGHTHRESHOLD (0.7) // Threshold sensed voltage.
#define LOWTHRESHOLD (0.7)  // Threshold sensed voltage.

// ---- Debugging only ------------
#define RED_LED_POS (18) // On port B.
#define GREEN_LED_POS (19)  // On port B.
// ---- End debugging only ------------

/*----------------------------------------------------------------------------*
  Turns off all lights.
 *----------------------------------------------------------------------------*/
void SignalResetAll() {
    SignalReset(RED_S);
    SignalReset(AMBER_S);
    SignalReset(GREEN_S);
    SignalReset(DONTWALK_S);
    SignalReset(WALK_S);
    SignalReset(WAIT_S);
}

/*----------------------------------------------------------------------------*
  Returns the average of the measured voltage for the current state.
 *----------------------------------------------------------------------------*/
volatile float measured_voltage; // Scaled value.
volatile unsigned res[MCYCLES]; // Raw values.
volatile int ac;
volatile float voltages[6];
volatile float current_volt;
volatile float percentage;

float volt_measurement(int time) {
    // Ignores first incorrect measurement.
    if (cycleCounter == 0) {
        measured_voltage = 0;
        return 0;
    }

    ac = (cycleCounter - 1) % MCYCLES;

    // Prevents last cycle from being measured.
    if (ac <= MCYCLES && cycleCounter < CYCLESPERSEC * time) {
        res[ac] = Measure();
        measured_voltage = (VREF * res[ac]) / ADCRANGE;
    }

    // Calculates average each five cycles.
    if (ac == MCYCLES - 1) {
        unsigned sum = 0;
        int i;

        for (i=0; i < MCYCLES; i++)
            sum = sum + res[i];

        measured_voltage = (VREF * sum) / (MCYCLES * ADCRANGE);

        // Turn on GREEN LED for low voltage and RED for high.
        PTB->PSOR |= MASK(RED_LED_POS) | MASK(GREEN_LED_POS);

        if (measured_voltage >= HIGHTHRESHOLD)
            PTB->PCOR |= MASK(RED_LED_POS);
        else if (measured_voltage < LOWTHRESHOLD)
            PTB->PCOR |= MASK(GREEN_LED_POS);
    }

    return measured_voltage;
}

/*----------------------------------------------------------------------------*
  Execution of the System Initiation states.
 *----------------------------------------------------------------------------*/
int stateLogicInit(int currState, int nextState, enum PelicanSignal ps,
        int numVoltage) {
    // Curent measurement.
    voltages[numVoltage] = volt_measurement(1);

    // RED light or DONTWALK light failure.
    if (red_failure) {
        SignalResetAll();
        cycleCounter = 0;
        return WAITFLASHINGON;
    }

    // Turn on light.
    SignalSet(ps);

    // State exit.
    if (cycleCounter >= CYCLESPERSEC) {
        SignalReset(ps);
        cycleCounter = 0;

        if (currState == WAITINIT)
            init_counter++;

        // CROSSING button pressed.
        if (init_counter > 1) {
            if (ButtonTestReset()) {
                if (currState != REDINIT && currState != AMBERFAILUREINIT &&
                        currState != DONTWALKINIT)
                    SignalReset(ps);

                return REDANDDONTWALK;
            }
        }

        return nextState;
    }

    cycleCounter++;
    return currState;
}

/*----------------------------------------------------------------------------*
  Execution of the Normal Operation states.
 *----------------------------------------------------------------------------*/
int stateLogic(int currState, int nextState, enum PelicanSignal ps, int time) {
    // Turn on 'ps' for 'time' seconds.
    if (cycleCounter >= CYCLESPERSEC * time) {
        cycleCounter = 0;
        return nextState;
    } else {
        SignalSet(ps);
    }

    // RED light or DONTWALK light failure.
    if (red_failure) {
        SignalResetAll();
        cycleCounter = 0;
        return WAITFLASHINGON;
    }

    cycleCounter++;
    return currState;
}

/*----------------------------------------------------------------------------*
  Executes an STM that cycles through the states for the lights.
 *----------------------------------------------------------------------------*/
int executeSTM(int curr_state) {
    switch(curr_state) {
        // --- RED init --- //
        case REDINIT:
            return stateLogicInit(REDINIT, AMBERINIT, RED_S, 0);

            // --- AMBER init --- //
        case AMBERINIT:
            return stateLogicInit(AMBERINIT, GREENINIT, AMBER_S, 1);

            // --- AMBER failure init --- //
        case AMBERFAILUREINIT:
            return stateLogicInit(AMBERFAILUREINIT, GREENINIT, RED_S, 0);

            // --- GREEN init --- //
        case GREENINIT:
            return stateLogicInit(GREENINIT, DONTWALKINIT, GREEN_S, 2);

            // --- DONTWALK init --- //
        case DONTWALKINIT:
            return stateLogicInit(DONTWALKINIT, WALKINIT, DONTWALK_S, 3);

            // --- WALK init --- //
        case WALKINIT:
            return stateLogicInit(WALKINIT, WAITINIT, WALK_S, 4);

            // --- WAIT init --- //
        case WAITINIT:
            return stateLogicInit(WAITINIT, REDINIT, WAIT_S, 5);

            // --- RED and DONTWALK --- //
        case REDANDDONTWALK:
            // Turn on RED and DONTWALK for T4 seconds.
            SignalSet(RED_S);
            SignalSet(DONTWALK_S);

            // Current measurement.
            current_volt = volt_measurement(T4);
            percentage = (voltages[0] + voltages[3]) * 0.05;

            if (cycleCounter > 1) {
                if ((current_volt > voltages[0] + voltages[3] + percentage) ||
                        (current_volt < voltages[0] + voltages[3] - percentage))
                    red_failure = 1;
            }

            // RED light or DONTWALK light failure.
            if (red_failure) {
                SignalResetAll();
                cycleCounter = 0;
                return WAITFLASHINGON;
            }

            if (cycleCounter >= CYCLESPERSEC * T4) {
                cycleCounter = 0;
                return (amber_failure) ? AMBERFAILUREANDREDON : AMBERANDREDON;
            }

            cycleCounter++;

            return curr_state;

            // --- WAIT flashing on --- //
        case WAITFLASHINGON:
            // Turn on WAIT on for T7 seconds.
            SignalSet(WAIT_S);

            if (cycleCounter >= CYCLESPERSEC * T7) {
                cycleCounter = 0;
                return WAITFLASHINGOFF;
            }

            cycleCounter++;

            return curr_state;

            // --- WAIT flashing off --- //
        case WAITFLASHINGOFF:
            // Turn off WAIT off for T7 seconds.
            SignalReset(WAIT_S);

            if (cycleCounter >= CYCLESPERSEC * T7) {
                cycleCounter = 0;
                return WAITFLASHINGON;
            }

            cycleCounter++;

            return curr_state;

            // --- GREEN on --- //
        case GREENON:
            // Turn on GREEN.
            SignalSet(GREEN_S);

            // Current measurement.
            current_volt = volt_measurement(cycleCounter + 1);
            percentage = (voltages[2] + voltages[3]) * 0.05;
            if (cycleCounter > 1) {
                if ((current_volt <= voltages[2]) ||
                        (current_volt > voltages[2] + percentage) ||
                        (current_volt < voltages[2] - percentage))
                    red_failure = 1;
            }

            // RED light or DONTWALK light failure.
            if (red_failure) {
                SignalResetAll();
                cycleCounter = 0;
                return WAITFLASHINGON;
            }

            // CROSSING button pressed.
            if (ButtonTestReset()) {
                return WAITON;
            }

            cycleCounter++;

            return curr_state;

            // --- WAIT on --- //
        case WAITON:
            //Turn on WAIT.
            SignalSet(WAIT_S);

            // Current measurement.
            current_volt = volt_measurement(T6);
            percentage = (voltages[2] + voltages[3] + voltages[5]) * 0.05;

            if (cycleCounter > 1) {
                if ((current_volt <= voltages[2]) ||
                        (current_volt <= voltages[5]) ||
                        (current_volt > voltages[2] + voltages[5] - percentage))
                    red_failure = 1;
            }

            // State exit.
            if (cycleCounter >= CYCLESPERSEC * T6) {
                SignalSet(WAIT_S);
                SignalReset(GREEN_S);
            }

            return (amber_failure)
                ? stateLogic(WAITON, AMBERFAILURE, WAIT_S, T6)
                : stateLogic(WAITON, AMBERON, WAIT_S, T6);

            // --- AMBER on --- //
        case AMBERON:
            // Turn on AMBER.
            SignalSet(AMBER_S);

            // Current measurement.
            current_volt = volt_measurement(T1);
            percentage = (voltages[1] + voltages[3] + voltages[5]) * 0.05;

            if (cycleCounter > 1) {
                // RED failure.
                if (current_volt < voltages[1] + voltages[3] + percentage)
                    red_failure = 1;

                // DONTWALK failure.
                if(current_volt < voltages[0] + voltages[1] + percentage)
                    red_failure = 1;
            }

            // AMBER failure.
            if (amber_failure) {
                SignalReset(AMBER_S);
                cycleCounter = 0;
                return AMBERFAILURE;
            }

            // State exit.
            if (cycleCounter >= CYCLESPERSEC * T1) {
                SignalReset(AMBER_S);
                SignalReset(WAIT_S);
            }

            return stateLogic(AMBERON, REDON, AMBER_S, T1);

            // --- AMBER failure --- //
        case AMBERFAILURE:
            // Turn on AMBER.
            SignalSet(RED_S);

            // Current measurement.
            current_volt = volt_measurement(T1);
            percentage = (voltages[1] + voltages[3] + voltages[5]) * 0.05;

            if (cycleCounter > 1) {
                // RED failure.
                if (current_volt < voltages[0] + percentage)
                    red_failure = 1;

                // DONTWALK failure
                if (current_volt < voltages[3] + percentage)
                    red_failure = 1;
            }

            // State exit.
            if (cycleCounter >= CYCLESPERSEC * T1) {
                SignalReset(AMBER_S);
                SignalReset(WAIT_S);
            }

            return stateLogic(AMBERFAILURE, REDON, RED_S, T1);

            // --- RED on --- //
        case REDON:
            // Turn on RED.
            SignalSet(RED_S);

            // Current measurement.
            current_volt = volt_measurement(T2);
            percentage = (voltages[0] + voltages[3]) * 0.05;

            if (cycleCounter > 1) {
                if ((current_volt > voltages[0] + voltages[3] + percentage) ||
                        (current_volt < voltages[0] + voltages[3] - percentage))
                    red_failure = 1;
            }

            // State exit.
            if(cycleCounter >= CYCLESPERSEC * T2) {
                SignalReset(DONTWALK_S);
                SignalReset(WAIT_S);
            }

            return stateLogic(REDON, WALKON, RED_S, T2);

            // --- WALK on --- //
        case WALKON:
            // Turn on WALK.
            SignalSet(WALK_S);

            // Current measurement.
            current_volt = volt_measurement(T2);
            percentage = (voltages[0] + voltages[4]) * 0.05;

            if (cycleCounter > 1) {
                if ((current_volt <= voltages[4]) ||
                        (current_volt > voltages[4] + percentage) ||
                        (current_volt < voltages[4] - percentage))
                    red_failure = 1;
            }

            // State exit.
            if (cycleCounter >= CYCLESPERSEC * T3) {
                SignalReset(WALK_S);
            }

            return stateLogic(WALKON, DONTWALKON, WALK_S, T3);

            // --- DONTWALK on --- //
        case DONTWALKON:
            // Turn on DONTWALK.
            SignalSet(DONTWALK_S);

            // Current measurement.
            current_volt = volt_measurement(T4);
            percentage = (voltages[0] + voltages[3]) * 0.05;

            if (cycleCounter > 1) {
                if ((current_volt > voltages[0] + voltages[3] + percentage) ||
                        (current_volt < voltages[0] + voltages[3] - percentage))
                    red_failure = 1;
            }

            return (amber_failure)
                ? stateLogic(DONTWALKON, AMBERFAILUREANDREDON, DONTWALK_S, T4)
                : stateLogic(DONTWALKON, AMBERANDREDON, DONTWALK_S, T4);

            // --- AMBER and RED on --- //
        case AMBERANDREDON:
            // Turn on AMBER.
            SignalSet(AMBER_S);

            // Current measurement.
            current_volt = volt_measurement(T5);
            percentage = (voltages[0] + voltages[1] + voltages[3]) * 0.05;

            if (cycleCounter > 1) {
                // RED failure.
                if (current_volt < voltages[1] + voltages[3] + percentage)
                    red_failure = 1;

                // DONTWALK failure.
                if (current_volt < voltages[0] + voltages[1] + percentage)
                    red_failure = 1;
            }

            // RED or DONTWALK failure.
            if (red_failure) {
                SignalResetAll();
                cycleCounter = 0;
                return WAITFLASHINGON;
            }

            // AMBER failure.
            if (amber_failure) {
                SignalReset(AMBER_S);
                cycleCounter = 0;
                return AMBERFAILUREANDREDON;
            }

            // State exit.
            if (cycleCounter >= CYCLESPERSEC * T5) {
                SignalReset(RED_S);
                SignalReset(AMBER_S);
                ButtonTestReset();
            }

            return stateLogic(AMBERANDREDON, GREENON, AMBER_S, T5);

            // --- AMBER failure and RED on --- //
        case AMBERFAILUREANDREDON:
            // Current measurement.
            current_volt = volt_measurement(T5);
            percentage = (voltages[0] + voltages[3]) * 0.05;

            if (cycleCounter > 1) {
                if ((current_volt > (voltages[0] + voltages[3]) + percentage) ||
                        (current_volt < (voltages[0] + voltages[3] -
                        percentage)))
                    red_failure = 1;
            }

            // RED light or DONTWALK light failure.
            if (red_failure) {
                SignalResetAll();
                cycleCounter = 0;
                return WAITFLASHINGON;
            }

            // Turn off RED after T5 seconds.
            if (cycleCounter >= CYCLESPERSEC * T5) {
                SignalReset(RED_S);
                ButtonTestReset();
                cycleCounter = 0;
                return GREENON;
            }

            cycleCounter++;

            return curr_state;
    }

    return -1 ; // Error: should never occur.
}

/*----------------------------------------------------------------------------*
  MAIN function
 *----------------------------------------------------------------------------*/

volatile int state ;

int main (void) {
    // ---- Debugging only ------------
    // Enable clock to ports B.
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK ;

    // Make 2 PortB on-board LED GPIO pins o/p.
    PORTB->PCR[RED_LED_POS] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[RED_LED_POS] |= PORT_PCR_MUX(1);
    PORTB->PCR[GREEN_LED_POS] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[GREEN_LED_POS] |= PORT_PCR_MUX(1);

    // Set o/p.
    PTB->PDDR |= MASK(RED_LED_POS) | MASK(GREEN_LED_POS);

    // Turn off LEDs.
    PTB->PSOR |= MASK(RED_LED_POS) | MASK(GREEN_LED_POS);
    // ------ End debugging only -----------------

    PelicanConfig();
    // End of configuration.

    // Initialise.
    ButtonTestReset(); // Ignore answer.
    SignalResetAll();

    cycleCounter = 0;
    state = REDINIT;

    // ---- Debugging only ------------
    //PTB->PCOR |= MASK(RED_LED_POS);
    // ---- End debugging only ------------

    while(1) {
        // Execute STM.
        state = executeSTM(state);

        // Wait for start of cycle.
        WaitSysTickCounter(CYCLESYSTICK);

        // ---- Debugging only ------------
        //if (cycleCounter < 20) {
        //  PTB->PSOR |= MASK(GREEN_LED_POS);
        //} else {
        //  PTB->PCOR |= MASK(GREEN_LED_POS);
        //}
        // ---- End debugging only ------------
    }
}
