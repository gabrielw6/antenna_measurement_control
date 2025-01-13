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

int getEndstopPin(int motorID);

// cmdType is 0 for commands in the style
// 	L#  -> command 'L' followed by numeric argument #
// parsedArgs should be a pointer to an integer array of length 2.
void parseInput(String inputStr, int cmdType, int *parsedArgs) {
	int newVal, spaceIndex;
	String valStr;
	valStr = inputStr.substring(1);
	valStr.trim();
	newVal = valStr.toInt();

	parsedArgs[0] = 0;
	parsedArgs[1] = 0;

	if (cmdType == 0) {
		parsedArgs[0] = newVal;
		return;
	}

  // if cmdType == 1 ...  
  spaceIndex = valStr.indexOf(' ');

  if (spaceIndex < 0) {
    Serial.println("Missing arguments.");
    return;
  }

  String argumentA = valStr.substring(0, spaceIndex);
  argumentA.trim();
  String argumentB = valStr.substring(spaceIndex + 1);
  argumentB.trim();
  parsedArgs[0] = argumentA.toInt();
  if ( (parsedArgs[0] < 1) || (parsedArgs[0] > 3)) {
    Serial.println("Motor index incorrect. Should be between 1 and 3.");
    parsedArgs[0] = 0;
    return;
  }
  parsedArgs[1] = argumentB.toInt();

  return;
}

void stopMotorsCMD(int motorID) {
    if (motorID == 0) {
      stopMotor[AZI] = true;
      stopMotor[POL] = true;
      stopMotor[ELE] = true;
      Serial.println("Stop ALL (AZI,POL,ELE).");
    } else if (motorID >= 1 && motorID <= 3) {
      stopMotor[motorID - 1] = true;
      Serial.print("Stop ");
      Serial.print(motorNames[motorID - 1]);
      Serial.println(" motor.");
    }
}

void moveMotorsCMD(int motorID, int steps) {
      desiredSteps[motorID-1] = steps;
      Serial.print("Motor ID: ");
      Serial.println(motorID);

      // Otherwise, proceed with moveMotor
      Serial.print(motorNames[motorID-1]);
      Serial.print(" steps = ");
      Serial.println(steps);

      if ((motorID-1) == 0) {
        moveMotor(0, steps, directionVal[0], stepFrequency[0],
                  STEP_PIN_AZI, DIR_PIN_AZI,
                  MCPWM_UNIT_AZI, MCPWM_TIMER_AZI);
      } else if ((motorID-1) == 1) {
        moveMotor(1, steps, directionVal[1], stepFrequency[1],
                  STEP_PIN_POL, DIR_PIN_POL,
                  MCPWM_UNIT_POL, MCPWM_TIMER_POL);
      } else {
        moveMotor(2, steps, directionVal[2], stepFrequency[2],
                  STEP_PIN_ELE, DIR_PIN_ELE,
                  MCPWM_UNIT_ELE, MCPWM_TIMER_ELE);
      }
}

		
void parseSerialCommand(const String& input)
{
  int argv[3];
  int mID, dir, steps, rpm;
  float rpmF;
  if (input.length() == 0) return;

  char cmd = input.charAt(0);

  switch(cmd) {
    // 1. G => get stepsPerRev
    case 'G':
      Serial.println(stepsPerRev);
      break;

    // 2. R => set stepsPerRev
    case 'R':
      parseInput(input, 0, argv);
      if (argv[0] > 0) {
        stepsPerRev = argv[0];
        Serial.print("StepsPerRev set to ");
        Serial.println(stepsPerRev);
      } else {
        Serial.println("Invalid stepsPerRev.");
      }
      return;

    // 3. X => stop commands
    case 'X':
      if (input.length() < 2) {
        Serial.println("Use X0..X3 (0=all, 1=AZI,2=POL,3=ELE).");
        return;
      }
      parseInput(input, 0, argv);
      stopMotorsCMD(argv[0]);
      return;

    // 4. Z => disableAfterMotion
    case 'Z':
      parseInput(input, 0, argv);
      if (argv[0] == 1) {
        disableAfterMotion = true;
        Serial.println("disableAfterMotion = true");
        digitalWrite(5, HIGH);
      } else {
        disableAfterMotion = false;
        Serial.println("disableAfterMotion = false => motors forced ON");
        digitalWrite(5, LOW); // Keep motors on
      }

      return;

    // Sx <steps> => step command
    case 'S':
      parseInput(input, 1, argv);
      mID = argv[0];
      steps = argv[1];
      if (steps <= 0) {
        Serial.println("Invalid steps. Must be positive integer.");
        break;
      }

      moveMotorsCMD(mID, steps);

      return;

    // Dx <0 or 1> => set direction
    case 'D':
      parseInput(input, 1, argv);
      mID = argv[0]-1;
      dir = argv[1];
      if ( (dir < 0) || (dir > 1)) {
        Serial.println("Invalid dir. Must be 0 or 1.");
      }

      directionVal[mID] = dir;
      switch(mID) {
        case 0: 
          digitalWrite(DIR_PIN_AZI, dir ? HIGH : LOW);
          Serial.print("AZI direction => ");
          Serial.println(dir);
          break;
        case 1:
          digitalWrite(DIR_PIN_POL, dir ? HIGH : LOW);
          Serial.print("POL direction => ");
          Serial.println(dir);
          break;
        case 2:
          digitalWrite(DIR_PIN_ELE, dir ? HIGH : LOW);
          Serial.print("ELE direction => ");
          Serial.println(dir);
          break;
      }
      return;

    // Fx <rpm> => set speed
    case 'F':
      parseInput(input, 1, argv);
      mID = argv[0];
      rpm = argv[1];

      if (rpm < 0) {
        Serial.println("Invalid RPM. Must be positive integer.");
        return;
      }

      rpmF = (float)rpm;
      stepFrequency[mID] = (rpmF * (float)stepsPerRev) / 60.0f;
      switch(mID) {
        case 0:
          Serial.print("AZI speed => ");
          Serial.print(rpmF);
          Serial.print(" rpm => ");
          Serial.println(stepFrequency[0]);
          break;
        case 1:
          Serial.print("POL speed => ");
          Serial.print(rpmF);
          Serial.print(" rpm => ");
          Serial.println(stepFrequency[1]);
          break;
        case 2:
          Serial.print("ELE speed => ");
          Serial.print(rpmF);
          Serial.print(" rpm => ");
          Serial.println(stepFrequency[2]);
          break;
      }

      return;

    default:
      Serial.println("Unknown cmd. Use Sx, Dx, Fx, Xx, G, R, Z, W...");
      break;
  }
}
