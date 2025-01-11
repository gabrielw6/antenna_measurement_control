#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// --------------------- Pin Assignments ---------------------
// Azimuth (AZI)
#define STEP_PIN_AZI 18
#define DIR_PIN_AZI  19
#define ENDSTOP_PIN_AZI 15  // Active LOW => goes LOW when endstop triggered

// Polarization (POL)
#define STEP_PIN_POL 21
#define DIR_PIN_POL  22
#define ENDSTOP_PIN_POL 4   // Active LOW => goes LOW when endstop triggered

// Elevation (ELE)
#define STEP_PIN_ELE 23
#define DIR_PIN_ELE  25
#define ENDSTOP_PIN_ELE 2   // Active LOW => goes LOW when endstop triggered

// MCPWM timers
#define MCPWM_UNIT_AZI  MCPWM_UNIT_0
#define MCPWM_TIMER_AZI MCPWM_TIMER_0

#define MCPWM_UNIT_POL  MCPWM_UNIT_0
#define MCPWM_TIMER_POL MCPWM_TIMER_1

#define MCPWM_UNIT_ELE  MCPWM_UNIT_0
#define MCPWM_TIMER_ELE MCPWM_TIMER_2



// ---- Global arrays/variables as extern ----
extern int   desiredSteps[3];
extern int   directionVal[3];
extern float stepFrequency[3];

extern bool  stopMotor[3];
extern bool  disableAfterMotion;
extern bool  sweepMode[3];
extern volatile bool endstopTriggered[3];

// Helper enumerations (if you wish to share them)
static const int AZI = 0;
static const int POL = 1;
static const int ELE = 2;

// Steps per revolution (default)
extern int stepsPerRev;

#endif // GLOBALS_H
