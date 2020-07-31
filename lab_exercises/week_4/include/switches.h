#ifndef SWITCHES_H
#define SWITCHES_H
#include "gpio_defs.h"

// Switches is on port D for interrupt support
#define BUTTON_POS (6)

// Function prototypes
extern void init_switch(void);

// Shared variables
extern volatile unsigned buttonPress;
#endif
// *******************************ARM University Program Copyright © ARM Ltd 2013*************************************   
