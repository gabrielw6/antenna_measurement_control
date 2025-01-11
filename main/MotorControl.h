#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

#include "Globals.h" // so we see extern references, if needed
#include <driver/mcpwm.h>
#include <soc/mcpwm_periph.h>

// Function prototypes
mcpwm_io_signals_t getMCPWMOutputSignal(mcpwm_timer_t timer);
void setupMCPWMChannel(mcpwm_unit_t unit, mcpwm_timer_t timer, int stepPin);

void moveMotor(int motorID, int steps, int dir, float freq, 
               int stepPin, int dirPin,
               mcpwm_unit_t unit, mcpwm_timer_t timer);

#endif // MOTORCONTROL_H
