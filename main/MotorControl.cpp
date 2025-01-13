#include "MotorControl.h"

#define MOTOR_ENABLE_PIN 5 // or extern from main.ino if you prefer

mcpwm_io_signals_t getMCPWMOutputSignal(mcpwm_timer_t timer) {
  switch(timer) {
    case MCPWM_TIMER_0: return MCPWM0A;
    case MCPWM_TIMER_1: return MCPWM1A;
    case MCPWM_TIMER_2: return MCPWM2A;
    default:            return MCPWM0A; 
  }
}

void setupMCPWMChannel(mcpwm_unit_t unit, mcpwm_timer_t timer, int stepPin) {
    mcpwm_io_signals_t io_signal = getMCPWMOutputSignal(timer);
    mcpwm_gpio_init(unit, io_signal, stepPin);

    mcpwm_config_t pwm_config;
    memset(&pwm_config, 0, sizeof(mcpwm_config_t));
    pwm_config.frequency    = 1000;  
    pwm_config.cmpr_a       = 50.0;  
    pwm_config.cmpr_b       = 0.0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode    = MCPWM_DUTY_MODE_0;

    mcpwm_init(unit, timer, &pwm_config);
    mcpwm_stop(unit, timer);
}

// void moveHome(int motorID, float freq, int stepPin, int dirPin,
//               mcpwm_unit_t unit, mcpwm_timer_t timer) {
//   // Clear stop flag
//   stopMotor[motorID] = false;

//   // Enable motors
//   digitalWrite(MOTOR_ENABLE_PIN, LOW);

//   // Set direction
//   digitalWrite(dirPin, (badEndstopDir[motorID] == 1) ? HIGH : LOW);

//   // Start MCPWM
//   mcpwm_stop(unit, timer);
//   mcpwm_set_frequency(unit, timer, (int)freq);
//   mcpwm_set_duty(unit, timer, MCPWM_OPR_A, 50.0);
//   mcpwm_start(unit, timer);
// }

void moveMotor(int motorID, int steps, int dir, float freq, 
               mcpwm_unit_t unit, mcpwm_timer_t timer)
{
  int dirPin = dirPins[motorID];
  int stepPin = stepPins[motorID];

  // Clear stop flag
  stopMotor[motorID] = false;

  // Enable motors
  digitalWrite(MOTOR_ENABLE_PIN, LOW);

  // Set direction
  digitalWrite(dirPin, (dir == 1) ? HIGH : LOW);

  // Start MCPWM
  mcpwm_stop(unit, timer);
  mcpwm_set_frequency(unit, timer, (int)freq);
  mcpwm_set_duty(unit, timer, MCPWM_OPR_A, 50.0);
  mcpwm_start(unit, timer);

  float period_s       = 1.0f / freq;
  float total_time_s   = steps * period_s;
  unsigned long total_time_ms = (unsigned long)(total_time_s * 1000.0f);
  unsigned long startTime = millis();



  while ((millis() - startTime) < total_time_ms) {
    if (stopMotor[motorID]) {
      Serial.print(motorNames[motorID]);
      Serial.println(" motor STOP mid-move.");
      break;
    }
    if((dir == badEndstopDir[motorID]) && (endstopTriggered[motorID])) {
      Serial.print("Refuse to move ");
      Serial.print(motorNames[motorID]);
      Serial.println(" motor past endstop.");
      break;
    }
    checkForStopCommands();
    delay(1);
  }

  // Stop PWM
  mcpwm_stop(unit, timer);

  // Clear stop flag
  stopMotor[motorID] = false;

  // End message
  Serial.print(motorNames[motorID]);
  Serial.println(" movement complete.");

  // If "disableAfterMotion" and no sweep => disable
  if (disableAfterMotion && !(sweepMode[0] || sweepMode[1] || sweepMode[2])) {
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    Serial.println("All motors disabled after motion (D5=HIGH).");
  }
}
