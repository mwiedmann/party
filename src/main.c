#include <cx16.h>
#include <stdio.h>
#include <string.h>
#include <cx16.h>
#include <cbm.h>
#include <6502.h>
#include <conio.h>

typedef struct Criteria {
  unsigned char gameStateId;
  unsigned char value;
} Criteria;

typedef struct StateChange {
  unsigned char id;
  unsigned char value;
} StateChange;

typedef struct Choice {
  unsigned char canSelect;
  unsigned short textStringOffset;
  unsigned char transitionVisualId; // Visual to transition to
  unsigned short resultStringOffset; // Shows after you pick this choice
  Criteria criteria[2];
  StateChange stateChanges[2];
} Choice;

typedef struct Visual {
  unsigned short nameStringOffset;
  unsigned short textStringOffset;
  Choice choices[10];
  char stringData[2048];
} Visual;

char gameState[256]; // [0] is 1, so default criteria will fail

Visual currentVisual;

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
                VERA.data0 = x>=40 ? 6<<4 : 0;
            } else {
                VERA.data0 = 32;
                VERA.data0 = 6<<4|1;
            }
        }
    }
}

void init() {
    unsigned short i;
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

    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam("party.pal");
	cbm_k_load(3, 0x1FA00L);

    // Add text colors
    // TODO: Need right way to keep these colors
    VERA.address = 0xFA02L;
    VERA.address_hi = 1;
    // Set the Increment Mode, turn on bit 4
    VERA.address_hi |= 0b10000;

    for(i=0; i<1; i++) {
        VERA.data0 = 255; // Color index
        VERA.data0 = 255;
    }

    VERA.address = 0xFA0CL;
    VERA.address_hi = 1;
    // Set the Increment Mode, turn on bit 4
    VERA.address_hi |= 0b10000;

    for(i=0; i<1; i++) {
        VERA.data0 = 0b00001101; // Color index
        VERA.data0 = 0;
    }

    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam("party.bin");
	cbm_k_load(2, 0);
}

void loadVisual(unsigned char id) {
    char buf[16];

    sprintf(buf, "vis%u.bin", id);

    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam(buf);
	cbm_k_load(0, ((unsigned short)&currentVisual));
}

char * getString(unsigned short offset) {
  return &currentVisual.stringData[offset];
}

void testLayers() {
    
    printf("Video: %u\n", VERA.display.video); // 0010 0001
    printf("L1 Config: %u\n", VERA.layer1.config); // 0110 0000

    getchar();
}

void main() {
    unsigned char visualId = 1; // Start in the foyer
    unsigned char i, c, foundActiveCriteria, criteriaFailed;
    unsigned char choice;
    char *resultString = 0;

    gameState[0] = 1; // Default criteria will always skip/fail

    init();
    
    loadVisual(visualId);

    while (1) {
        clearImageArea();
        gotoxy(0, 31);
    
        if (resultString) {
            printf("%s\n\n", resultString);
            resultString = 0; // Reset after showing
        }

        // Print the room name
        printf("%s\n", getString(currentVisual.nameStringOffset));

        // Print the room description
        printf("%s\n\n", getString(currentVisual.textStringOffset));

        // Show choices
        for (i = 0; i < 10; i++) {
            // Check all criteria
            foundActiveCriteria = 0;
            criteriaFailed = 0;
            for (c=0; c<2; c++) {
                if (currentVisual.choices[i].criteria[c].gameStateId != 0) {
                    if (gameState[currentVisual.choices[i].criteria[c].gameStateId] != currentVisual.choices[i].criteria[c].value) {
                        criteriaFailed = 1; // Criteria not met
                        break;
                    }
                    foundActiveCriteria = 1; // At least one criteria is active
                }
            }

            if (!foundActiveCriteria || criteriaFailed) {
                continue; // No active criteria or criteria failed, skip this choice
            }
            
            // Print the choice number if it can be selected
            if (currentVisual.choices[i].canSelect) {
                printf("%d: ", i);
            }

            // Print the choice text
            printf("%s\n", getString(currentVisual.choices[i].textStringOffset));
        }
        
        printf("\nChoose an option: ");
        cursor(1);
        choice = cgetc() - '0'; // Convert char to index
        printf("\n");

        if (choice > 9 || currentVisual.choices[choice].criteria[0].gameStateId == 0) {
            // printf("Invalid choice. Try again.\n");
            continue;
        }

        // apply state changes
        for (i = 0; i < 2; i++) {
            if (currentVisual.choices[choice].stateChanges[i].id != 0) {
                gameState[currentVisual.choices[choice].stateChanges[i].id] = currentVisual.choices[choice].stateChanges[i].value;
            }
        }
        
        // Show result message if any
        if (currentVisual.choices[choice].resultStringOffset != 0) {
            resultString = getString(currentVisual.choices[choice].resultStringOffset);
        }

        // Transition to the next visual if there is a valid choice
        if (currentVisual.choices[choice].transitionVisualId != 0) {
            visualId = currentVisual.choices[choice].transitionVisualId;
            loadVisual(visualId);
        }

        //break;
    }
}