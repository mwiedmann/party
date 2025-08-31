#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

void printString(char *str, unsigned char *x, unsigned char *y);
char * getString(unsigned short offset, Visual *visual);
void printWordWrapped(char *text);
void clearImageArea();
void loadVisual(unsigned short id);
void loadPerson(unsigned short id, unsigned char index);
void loadTimeTable();
void loadImage(char * imageName);

#endif