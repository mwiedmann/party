#include "globals.h"

char gameState[256]; // [0] is 1, so default criteria will fail
char invStrings[INV_STRING_COUNT][INV_STRING_LENGTH];

Visual currentVisual;
PersonInfo persons[5];
Person timeTable[TIME_TABLE_LENGTH];

unsigned char hour = 0;
unsigned char minutes = 0;
unsigned char cursorX;
unsigned char cursorY;
