#include <cx16.h>

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