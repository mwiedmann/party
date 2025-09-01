#include <cx16.h>
#include <cbm.h>
#include <stdio.h>
#include <conio.h>

#include "globals.h"

void printString(char *str, unsigned char *x, unsigned char *y) {
    unsigned short offset;
    unsigned short i = 0;
    unsigned char c;

    VERA.address_hi = 1;
    // Set the Increment Mode, turn on bit 4
    VERA.address_hi |= 0b10000;

    while(str[i]) {
        offset = ((unsigned short)(*y) * 128 * 2) + ((unsigned short)(*x) * 2);
        // Point to layer0 mapbase
        VERA.address = (VERA.layer1.mapbase<<9) + offset;
        
        c = str[i];
        if (c >= 97 && c <= 122) {
            c-= 32;
        } else if (c >= 65 && c <= 90) {
            c-= 64;
        } else if (c >= 193 && c <= 218) {
            c-= 128;
        }

        VERA.data0 = c;
        VERA.data0 = 6<<4|1;
        i++;

        *x+=1;
        if (*x==80) {
            *x=0;
            *y+=1;
        }
    }
}

char * getString(unsigned short offset, Visual *visual) {
  return &visual->stringData[offset];
}

void printWordWrapped(char *text) {
    int t;
    char *wordStart;
    int wordLen;
    char word[81]; // Max word length

    t=0;
    while (text[t]) {
        // Handle non-printables
        while (text[t] == ' ' || text[t] == '\n' || text[t] == '\t') {
            // Handle newline
            if (text[t] == '\n') {
                cursorX = 0;
                cursorY++;
            } else if (text[t] == '\t') {  // Tabs
                cursorX+=2;
            }

            // Handle spaces   
            if (text[t] == ' ') {            
                if (cursorX == 0) {
                    // Skip these at beginning of line
                } else if (cursorX >= SCREEN_WIDTH - 1) {
                    cursorX = 0;
                    cursorY++;
                } else {
                    cursorX++;
                }
            }
            t++;
        }

        // Find word length
        wordStart = text+t;
        wordLen = 0;
        while (text[t] && text[t] != ' ' && text[t] != '\n' && text[t] != '\t') {
            if (wordLen < 80) {
                word[wordLen++] = text[t];
            }
            t++;
        }
        word[wordLen] = '\0';

        // If word doesn't fit, move to next line
        if (cursorX + wordLen > SCREEN_WIDTH) {
            cursorX = 0;
            cursorY++;
        }

        printString(word, &cursorX, &cursorY);
    }
}

void clearImageArea() {
    unsigned short x,y;

    // Point to layer0 mapbase
    VERA.address = VERA.layer1.mapbase<<9;
    VERA.address_hi = 1;
    // Set the Increment Mode, turn on bit 4
    VERA.address_hi |= 0b10000;

    for (y=0; y<60; y++) {
        for (x=0; x<128; x++) {
            if (y<30) {
                VERA.data0 = x>=40 ? 32 : 0;
                VERA.data0 = x>=40 ? 6<<4|1 : 0;
            } else {
                VERA.data0 = 32;
                VERA.data0 = 6<<4|1;
            }
        }
    }
}

void loadVisual(unsigned short id) {
    char buf[16];

    sprintf(buf, "vis%u.bin", id);

    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam(buf);
	cbm_k_load(0, ((unsigned short)&currentVisual));
}

void loadPerson(unsigned short id, unsigned char index) {
    char buf[16];

    sprintf(buf, "vis%u.bin", id);

    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam(buf);
	cbm_k_load(0, ((unsigned short)&persons[index]));
}

void loadTimeTable() {
    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam("timetbl.bin");
	cbm_k_load(0, ((unsigned short)&timeTable));
}

void loadInvStrings() {
    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam("invstr.bin");
	cbm_k_load(0, ((unsigned short)&invStrings));
}

void loadImage(char * imageName) {
    char buf[16];

    sprintf(buf, "%s.pal", imageName);
    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam(buf);
	cbm_k_load(3, 0x1FA00L);

    sprintf(buf, "%s.bin", imageName);
    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam(buf);
	cbm_k_load(2, 0);
}

