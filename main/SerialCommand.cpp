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
      desiredSteps[motorID] = steps;
      Serial.print("Motor ID: ");
      Serial.println(motorID);

      // Otherwise, proceed with moveMotor
      Serial.print(motorNames[motorID]);
      Serial.print(" steps = ");
      Serial.println(steps);

       moveMotor(motorID, steps, directionVal[motorID], stepFrequency[motorID],
                 stepPins[motorID], dirPins[motorID],
                 MCPWMUnits[motorID], MCPWMTimers[motorID]);
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
      mID = argv[0] - 1;
      steps = argv[1];
      // Step the motors in the specified amount
      if (steps <= 0) {
        Serial.println("Invalid steps. Must be positive integer.");
        break;
      }

      moveMotorsCMD(mID, steps);
      return;

    // Dx <0 or 1> => set direction
    case 'D':
      parseInput(input, 1, argv);
      mID = argv[0] - 1;
      dir = argv[1];
      // Set direction
      if ( (dir < 0) || (dir > 1)) {
        Serial.println("Invalid dir. Must be 0 or 1.");
      }

      directionVal[mID] = dir;
      digitalWrite(dirPins[mID], dir ? HIGH : LOW);

      Serial.print(motorNames[mID]);
      Serial.print(" direction => ");
      Serial.println(dir);
      return;

    // Fx <rpm> => set speed
    case 'F':
      parseInput(input, 1, argv);
      mID = argv[0]-1;
      rpm = argv[1];
      // Set speed
      if (rpm < 0) {
        Serial.println("Invalid RPM. Must be positive integer.");
        return;
      }

      rpmF = (float)rpm;
      stepFrequency[mID] = (rpmF * (float)stepsPerRev) / 60.0f;

      Serial.print(motorNames[mID]);
      Serial.print(" speed => ");
      Serial.print(rpmF);
      Serial.print(" rpm => ");
      Serial.println(stepFrequency[mID]);
      return;

    default:
      Serial.println("Unknown cmd. Use Sx, Dx, Fx, Xx, G, R, Z, W...");
      break;
  }
}
