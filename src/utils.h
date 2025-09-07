#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

void printString(char *str, unsigned char *x, unsigned char *y);
char * getString(unsigned short offset, Visual *visual);
void printWordWrapped(char *text);
void clearImageArea();
void loadInvStrings();
unsigned short loadVisual(unsigned short id, unsigned short stringOffset);
unsigned short loadPerson(unsigned short id, unsigned char index, unsigned short stringOffset);
void loadTimeTable();
void loadImage(char * imageName);

#endif