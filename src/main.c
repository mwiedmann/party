#include <cx16.h>
#include <stdio.h>
#include <string.h>
#include <cx16.h>
#include <cbm.h>
#include <6502.h>
#include <conio.h>

#define CURRENT_ROOM_ID 255 // Last byte of gameState to store current room ID
#define ROOM 0
#define PERSON 1

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
  unsigned short transitionVisualId; // Visual to transition to
  unsigned short resultStringOffset; // Shows after you pick this choice
  unsigned short roomId; // Room to move to if this is a choice on a Person
  Criteria criteria[2];
  StateChange stateChanges[2];
} Choice;

typedef struct Visual {
  unsigned char visualType; // 0 = Room, 1 = Person, 2 = Other
  unsigned short nameStringOffset;
  unsigned short textStringOffset;
  unsigned short imageStringOffset;
  Choice choices[10];
  char stringData[512];
} Visual;

typedef struct TimeEntry {
    unsigned char hour;
    unsigned char minute;
    unsigned short roomId;
} TimeEntry;

typedef struct Person {
    unsigned char id;
    TimeEntry timeEntries[10]; // Up to 10 time entries
} Person;

char gameState[256]; // [0] is 1, so default criteria will fail

Visual currentVisual;
Visual persons[5];
Person timeTable[50];

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

void loadVisual(unsigned short id) {
    char buf[16];

    sprintf(buf, "vis%u.bin", id);

    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam(buf);
	cbm_k_load(0, ((unsigned short)&currentVisual));
}

void loadImage(char* imageName) {
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

char * getString(unsigned short offset) {
  return &currentVisual.stringData[offset];
}

void testLayers() {
    
    printf("Video: %u\n", VERA.display.video); // 0010 0001
    printf("L1 Config: %u\n", VERA.layer1.config); // 0110 0000

    getchar();
}

void main() {
    unsigned short visualId = 1, currentImage = 0; // Start in the foyer
    unsigned char i, c, foundActiveCriteria, criteriaFailed;
    unsigned char choice;
    char resultString[1024];

    gameState[0] = 1; // Default criteria will always skip/fail

    init();
    
    loadVisual(visualId);

    while (1) {
        if (currentVisual.visualType == ROOM) { // Room
            gameState[CURRENT_ROOM_ID] = visualId; // Store current room ID
        }

        if (currentVisual.imageStringOffset != 0 && currentImage != currentVisual.imageStringOffset) {
            currentImage = currentVisual.imageStringOffset;
            loadImage(getString(currentVisual.imageStringOffset));
        }
        
        clearImageArea();
        gotoxy(0, 31);
    
        if (resultString[0]) {
            printf("%s\n\n", resultString);
            resultString[0] = 0; // Reset after showing
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
            // copy the resultStringOff string into resultString
            strcpy(resultString, getString(currentVisual.choices[choice].resultStringOffset));
        }

        // Transition to the next visual if there is a valid choice
        if (currentVisual.choices[choice].transitionVisualId != 0) {
            visualId = currentVisual.choices[choice].transitionVisualId;
            loadVisual(visualId);
        }

        //break;
    }
}