#include "SerialCommand.h"
#include "MotorControl.h"

#include <driver/mcpwm.h>
#include <soc/mcpwm_periph.h>
#include <string.h>

void checkForStopCommands()
{
  // If you want to parse commands mid-move
  while (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (!line.isEmpty()) {
      parseSerialCommand(line);
    }
  }
}

int getEndstopPin(int motorID); // if you need the helper from main.ino

void parseSerialCommand(const String& input)
{
  if (input.length() == 0) return;

  char cmd = input.charAt(0);

  // G => get stepsPerRev
  if (cmd == 'G') {
    Serial.println(stepsPerRev);
    return;
  }

  // R => set stepsPerRev
  if (cmd == 'R') {
    String valStr = input.substring(1);
    valStr.trim();
    int newVal = valStr.toInt();
    if (newVal > 0) {
      stepsPerRev = newVal;
      Serial.print("StepsPerRev set to ");
      Serial.println(stepsPerRev);
    } else {
      Serial.println("Invalid stepsPerRev.");
    }
    return;
  }

  // X => stop commands
  if (cmd == 'X') {
    if (input.length() < 2) {
      Serial.println("Use X0..X3 (0=all, 1=AZI,2=POL,3=ELE).");
      return;
    }
    int idx = input.charAt(1) - '0';
    if (idx == 0) {
      stopMotor[0] = true;
      stopMotor[1] = true;
      stopMotor[2] = true;
      Serial.println("Stop ALL (AZI,POL,ELE).");
    } else if (idx >= 1 && idx <= 3) {
      stopMotor[idx - 1] = true;
      switch(idx) {
        case 1: Serial.println("Stop AZI motor."); break;
        case 2: Serial.println("Stop POL motor."); break;
        case 3: Serial.println("Stop ELE motor."); break;
      }
    } else {
      Serial.println("Invalid X command. Use X0..X3.");
    }
    return;
  }

  // Z => disableAfterMotion
  if (cmd == 'Z') {
    String valStr = input.substring(1);
    valStr.trim();
    int val = valStr.toInt();
    if (val == 1) {
      disableAfterMotion = true;
      Serial.println("disableAfterMotion = true");
      digitalWrite(5, HIGH);
    } else {
      disableAfterMotion = false;
      Serial.println("disableAfterMotion = false => motors forced ON");
      digitalWrite(5, LOW); // Keep motors on
    }
    return;
  }

  // W => sweepMode
  if (cmd == 'W') {
    // e.g. "W 1 1" => AZI sweep ON
    String rest = input.substring(1);
    rest.trim();
    int spaceIdx = rest.indexOf(' ');
    if (spaceIdx < 0) {
      Serial.println("Format: W <1..3> <0/1>");
      return;
    }
    String motorStr = rest.substring(0, spaceIdx);
    String modeStr  = rest.substring(spaceIdx + 1);
    motorStr.trim(); modeStr.trim();
    int mID = motorStr.toInt();
    int modeVal = modeStr.toInt();
    if (mID < 1 || mID > 3) {
      Serial.println("W cmd: motor must be 1(AZI),2=POL,3=ELE.");
      return;
    }
    sweepMode[mID - 1] = (modeVal == 1);
    if (sweepMode[mID - 1]) {
      switch(mID) {
        case 1: Serial.println("AZI sweepMode = ON"); break;
        case 2: Serial.println("POL sweepMode = ON"); break;
        case 3: Serial.println("ELE sweepMode = ON"); break;
      }
    } else {
      switch(mID) {
        case 1: Serial.println("AZI sweepMode = OFF"); break;
        case 2: Serial.println("POL sweepMode = OFF"); break;
        case 3: Serial.println("ELE sweepMode = OFF"); break;
      }
    }
    // If any sweep is active => keep motors on
    if (sweepMode[0] || sweepMode[1] || sweepMode[2]) {
      digitalWrite(5, LOW);
      Serial.println("At least one sweep ON => motors forced ON (D5=LOW).");
    } else {
      Serial.println("No sweeps => normal disableAfterMotion logic applies.");
    }
    return;
  }

  // For S, D, F => e.g. "S1 200"
  if (input.length() < 3) {
    Serial.println("Command format error. e.g. 'S1 200'.");
    return;
  }

  // Determine which motor
  char mChar = input.charAt(1);
  if (mChar < '1' || mChar > '3') {
    Serial.println("Motor must be 1(AZI),2=POL,3=ELE.");
    return;
  }
  int mID = (mChar - '1');

  String valStr = input.substring(3);
  valStr.trim();
  int val = valStr.toInt();

  switch (cmd) {
    // Sx <steps> => step command
    case 'S':
      if (val <= 0) {
        Serial.println("Invalid steps. Must be positive integer.");
        break;
      }
      desiredSteps[mID] = val;

      // Check if endstop is active, direction=1 => forward => refuse
      {
        int endstopPin = getEndstopPin(mID);
        bool endstopActiveNow = (digitalRead(endstopPin) == LOW);
        int dir = directionVal[mID];
      }
      // Otherwise, proceed with moveMotor
      if (mID == 0) {
        Serial.print("AZI steps = ");
        Serial.println(val);
        moveMotor(0, val, directionVal[0], stepFrequency[0],
                  STEP_PIN_AZI, DIR_PIN_AZI,
                  MCPWM_UNIT_AZI, MCPWM_TIMER_AZI);
      } else if (mID == 1) {
        Serial.print("POL steps = ");
        Serial.println(val);
        moveMotor(1, val, directionVal[1], stepFrequency[1],
                  STEP_PIN_POL, DIR_PIN_POL,
                  MCPWM_UNIT_POL, MCPWM_TIMER_POL);
      } else {
        Serial.print("ELE steps = ");
        Serial.println(val);
        moveMotor(2, val, directionVal[2], stepFrequency[2],
                  STEP_PIN_ELE, DIR_PIN_ELE,
                  MCPWM_UNIT_ELE, MCPWM_TIMER_ELE);
      }
      break;

    // Dx <0 or 1> => set direction
    case 'D':
      if (val == 0 || val == 1) {
        directionVal[mID] = val;
        switch(mID) {
          case 0: 
            digitalWrite(DIR_PIN_AZI, val ? HIGH : LOW);
            Serial.print("AZI direction => ");
            Serial.println(val);
            break;
          case 1:
            digitalWrite(DIR_PIN_POL, val ? HIGH : LOW);
            Serial.print("POL direction => ");
            Serial.println(val);
            break;
          case 2:
            digitalWrite(DIR_PIN_ELE, val ? HIGH : LOW);
            Serial.print("ELE direction => ");
            Serial.println(val);
            break;
        }
      } else {
        Serial.println("Invalid direction. Use 0 or 1.");
      }
      break;

    // Fx <rpm> => set speed
    case 'F':
      if (val > 0) {
        float rpm = (float)val;
        stepFrequency[mID] = (rpm * (float)stepsPerRev) / 60.0f;
        switch(mID) {
          case 0:
            Serial.print("AZI speed => ");
            Serial.print(rpm);
            Serial.print(" rpm => ");
            Serial.println(stepFrequency[0]);
            break;
          case 1:
            Serial.print("POL speed => ");
            Serial.print(rpm);
            Serial.print(" rpm => ");
            Serial.println(stepFrequency[1]);
            break;
          case 2:
            Serial.print("ELE speed => ");
            Serial.print(rpm);
            Serial.print(" rpm => ");
            Serial.println(stepFrequency[2]);
            break;
        }
      } else {
        Serial.println("Invalid RPM. Must be positive integer.");
      }
      break;

    default:
      Serial.println("Unknown cmd. Use Sx, Dx, Fx, Xx, G, R, Z, W...");
      break;
  }
}
