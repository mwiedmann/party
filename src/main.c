#include <cx16.h>
#include <stdio.h>
#include <string.h>
#include <cx16.h>
#include <cbm.h>
#include <6502.h>
#include <conio.h>

#define CURRENT_ROOM_ID 255 // Last byte of gameState to store current room ID
#define ROOM_TYPE 0
#define PERSON_TYPE 1
#define TRANSITION_CURRENT_ROOM 32767
#define PERSONS_PER_ROOM 5

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
  unsigned short personRoomId; // Room to move Person to if this is a choice on a Person
  unsigned char minutes; // How many minutes this choice takes
  unsigned short criteriaRoomId; // Room Person must be in for this choice to be active
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
    unsigned short id;
    unsigned short currentRoomId;
    TimeEntry timeEntries[1]; // Up to ? time entries
} Person;

typedef struct PersonInfo {
    Visual person;
    unsigned char timeTableIndex;
} PersonInfo;

char gameState[256]; // [0] is 1, so default criteria will fail

Visual currentVisual;
PersonInfo persons[5];

#define TIME_TABLE_LENGTH 50
Person timeTable[TIME_TABLE_LENGTH];

unsigned char hour = 8;
unsigned char minute = 0;

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

char * getString(unsigned short offset, Visual *visual) {
  return &visual->stringData[offset];
}

void testLayers() {
    
    printf("Video: %u\n", VERA.display.video); // 0010 0001
    printf("L1 Config: %u\n", VERA.layer1.config); // 0110 0000

    getchar();
}

void main() {
    unsigned short visualId = 1, currentImage = 0; // Start in the foyer
    unsigned char i, c, foundActiveCriteria, criteriaFailed, personIndex;
    unsigned char choice;
    char resultString[1024];
    PersonInfo currentPerson;

    gameState[0] = 1; // Default criteria will always skip/fail

    init();
    
    loadTimeTable();
    loadVisual(visualId);

    while (1) {
        if (currentVisual.visualType == ROOM_TYPE) { // Room
            gameState[CURRENT_ROOM_ID] = visualId; // Store current room ID
        }

        if (currentVisual.imageStringOffset != 0 && currentImage != currentVisual.imageStringOffset) {
            currentImage = currentVisual.imageStringOffset;
            loadImage(getString(currentVisual.imageStringOffset, &currentVisual));
        }
        
        clearImageArea();
        gotoxy(0, 31);
    
        if (resultString[0]) {
            printf("%s\n\n", resultString);
            resultString[0] = 0; // Reset after showing
        }

        // Print the room name
        printf("%s\n", getString(currentVisual.nameStringOffset, &currentVisual));

        // Print the room description
        printf("%s\n\n", getString(currentVisual.textStringOffset, &currentVisual));

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

            // Check if the current choice is only available in a certain room
            if (currentVisual.choices[i].criteriaRoomId && currentVisual.choices[i].criteriaRoomId != gameState[CURRENT_ROOM_ID]) {
                criteriaFailed = 1;
            }

            if (!foundActiveCriteria || criteriaFailed) {
                continue; // No active criteria or criteria failed, skip this choice
            }
            
            // Print the choice number if it can be selected
            if (currentVisual.choices[i].canSelect) {
                printf("%d: ", i);
            }

            // Print the choice text
            printf("%s\n", getString(currentVisual.choices[i].textStringOffset, &currentVisual));
        }
        
        if (currentVisual.visualType == ROOM_TYPE) {
            // Clear the persons list
            memset(persons, 0, sizeof(persons));

            personIndex=0;
            for (i=0; i<TIME_TABLE_LENGTH; i++) {
                if (timeTable[i].currentRoomId == gameState[CURRENT_ROOM_ID]) {
                    loadPerson(timeTable[i].id, personIndex);
                    persons[personIndex].timeTableIndex = i;
                    // Print an option to talk to this person
                    printf("%c: Talk to %s\n", 'A'+personIndex, getString(persons[personIndex].person.nameStringOffset, &persons[personIndex].person));

                    personIndex++;
                }
            }
        }

        printf("\nChoose an option: ");
        cursor(1);
        choice = cgetc(); // Convert char to index
        printf("\n");

        // See if selecting a person
        if (choice >= 'a' && choice <= 'e') {
            choice -= 'a';

            // Transition to person
            currentPerson = persons[choice];
            visualId = timeTable[persons[choice].timeTableIndex].id;
            loadVisual(visualId);     
        } else {
            // Making a non-person selection
            choice -= '0';
        
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
                strcpy(resultString, getString(currentVisual.choices[choice].resultStringOffset, &currentVisual));
            }

            // If this is a Person, see if they are moving
            if (currentVisual.visualType == PERSON_TYPE && currentVisual.choices[choice].personRoomId) {
                timeTable[currentPerson.timeTableIndex].currentRoomId = currentVisual.choices[choice].personRoomId;
            }

            // Transition to the next visual if there is a valid choice
            if (currentVisual.choices[choice].transitionVisualId != 0) {
                visualId = currentVisual.choices[choice].transitionVisualId;
                if (visualId == TRANSITION_CURRENT_ROOM) {
                    visualId = gameState[CURRENT_ROOM_ID];
                }
                loadVisual(visualId);
            }
        }

        

        //break;
    }
}