#include "Globals.h"
#include "MotorControl.h"
#include "SerialCommand.h"

// ------ Define global variables here ------
int   desiredSteps[3]   = {0, 0, 0};
int   directionVal[3]   = {0, 0, 0};
float stepFrequency[3]  = {1000.0, 1000.0, 1000.0};

bool  stopMotor[3]      = {false, false, false};
bool  disableAfterMotion = true;
bool  sweepMode[3]      = {false, false, false};
volatile bool endstopTriggered[3] = {false, false, false};

int stepsPerRev = 200;

// Pin definitions, if desired:
#define STEP_PIN_AZI 18
#define DIR_PIN_AZI  19
#define ENDSTOP_PIN_AZI 15

#define STEP_PIN_POL 21
#define DIR_PIN_POL  22
#define ENDSTOP_PIN_POL 4

#define STEP_PIN_ELE 23
#define DIR_PIN_ELE  25
#define ENDSTOP_PIN_ELE 2

// Motor enable pin (D5). LOW => motors enabled, HIGH => disabled
#define MOTOR_ENABLE_PIN 5

// --- Endstop ISRs ---
void IRAM_ATTR endstopAZI() {
  if (digitalRead(ENDSTOP_PIN_AZI) == LOW) {
    stopMotor[AZI] = true;
    endstopTriggered[AZI] = true;
  }
}
void IRAM_ATTR endstopPOL() {
  if (digitalRead(ENDSTOP_PIN_POL) == LOW) {
    stopMotor[POL] = true;
    endstopTriggered[POL] = true;
  }
}
void IRAM_ATTR endstopELE() {
  if (digitalRead(ENDSTOP_PIN_ELE) == LOW) {
    stopMotor[ELE] = true;
    endstopTriggered[ELE] = true;
  }
}

// Example helper function if you need it 
int getEndstopPin(int motorID) {
  switch(motorID) {
    case AZI: return ENDSTOP_PIN_AZI;
    case POL: return ENDSTOP_PIN_POL;
    case ELE: return ENDSTOP_PIN_ELE;
    default:  return -1; 
  }
}

// Setup
void setup() {
  Serial.begin(115200);

  pinMode(DIR_PIN_AZI, OUTPUT);
  pinMode(DIR_PIN_POL, OUTPUT);
  pinMode(DIR_PIN_ELE, OUTPUT);

  pinMode(ENDSTOP_PIN_AZI, INPUT);
  pinMode(ENDSTOP_PIN_POL, INPUT);
  pinMode(ENDSTOP_PIN_ELE, INPUT);

  attachInterrupt(digitalPinToInterrupt(ENDSTOP_PIN_AZI), endstopAZI, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENDSTOP_PIN_POL), endstopPOL, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENDSTOP_PIN_ELE), endstopELE, FALLING);

  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, HIGH); // OFF by default

  // setup MCPWM for each motor
  setupMCPWMChannel(MCPWM_UNIT_AZI, MCPWM_TIMER_AZI, STEP_PIN_AZI);
  setupMCPWMChannel(MCPWM_UNIT_POL, MCPWM_TIMER_POL, STEP_PIN_POL);
  setupMCPWMChannel(MCPWM_UNIT_ELE, MCPWM_TIMER_ELE, STEP_PIN_ELE);

  Serial.println("main.ino: Setup done, motors OFF at boot.");
}

void loop() {
  // For example, parse serial input for commands
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (!input.isEmpty()) {
      parseSerialCommand(input); // function from SerialCommand.cpp
    }
  }

  // Optional: check for endstop triggered messages
  for (int i = 0; i < 3; i++) {
    if (endstopTriggered[i]) {
      endstopTriggered[i] = false;
      switch(i) {
        case AZI: Serial.println("AZI motor movement stopped: ENDSTOP triggered."); break;
        case POL: Serial.println("POL motor movement stopped: ENDSTOP triggered."); break;
        case ELE: Serial.println("ELE motor movement stopped: ENDSTOP triggered."); break;
      }
    }
  }
}
