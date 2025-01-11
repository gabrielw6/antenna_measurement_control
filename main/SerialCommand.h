#ifndef SERIALCOMMAND_H
#define SERIALCOMMAND_H

#include "Globals.h" // so we see the extern references

// Function prototypes
void parseSerialCommand(const String& input);
void checkForStopCommands();

#endif // SERIALCOMMAND_H
