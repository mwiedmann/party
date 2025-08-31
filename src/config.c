#include <cx16.h>

void init() {
    // Bitmap mode
    // Color Depth 3 - 8 bpp
    VERA.display.video = 0b00110001;

    VERA.layer0.config = 0b00000111;
    //VERA.layer0.config = VERA.layer1.config;

    // Get bytes 16-11 of the new TileBase address
    // Set bit 0 to 0 (for 320 mode), its already 0 in this address
    VERA.layer0.tilebase = 0;
    //VERA.layer0.mapbase = VERA.layer1.mapbase;

    //printf("\nMapbase: %u %u\n", p, p+1);
    // VERA.layer1.mapbase = 76800L>>9;
    // VERA.layer1.tilebase = 0x1F000L>>9;
}