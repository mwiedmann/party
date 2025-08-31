#include <cx16.h>
#include <stdio.h>
#include <string.h>
#include <cx16.h>
#include <cbm.h>
#include <6502.h>
#include <conio.h>

#include "utils.h"

#define CURRENT_ROOM_ID 255 // Last byte of gameState to store current room ID
#define ROOM_TYPE 0
#define PERSON_TYPE 1
#define TRANSITION_CURRENT_ROOM 32767
#define PERSONS_PER_ROOM 5
#define TIME_TABLE_LENGTH 50
#define TIME_ENTRIES_LENGTH 5
#define START_HOUR 8

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
  char stringData[2048];
} Visual;

typedef struct TimeEntry {
    unsigned char hour;
    unsigned char minute;
    unsigned short roomId;
} TimeEntry;

typedef struct Person {
    unsigned short id;
    unsigned short currentRoomId;
    TimeEntry timeEntries[TIME_ENTRIES_LENGTH]; // Up to 5 time entries
} Person;

typedef struct PersonInfo {
    Visual person;
    unsigned char timeTableIndex;
} PersonInfo;

char gameState[256]; // [0] is 1, so default criteria will fail

Visual currentVisual;
PersonInfo persons[5];
Person timeTable[TIME_TABLE_LENGTH];

unsigned char hour = 0;
unsigned char minutes = 0;
unsigned char cursorX;
unsigned char cursorY;

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

char * getString(unsigned short offset, Visual *visual) {
  return &visual->stringData[offset];
}

void advanceTime(unsigned char minutesToAdd) {
    unsigned char i,j;

    minutes+= minutesToAdd;

    if (minutes >= 60) {
        hour++;
        minutes-=60;
    }

    // See if any Persons moved rooms
    for(i=0; i<TIME_TABLE_LENGTH; i++) {
        if (timeTable[i].id) {
            for (j=0; j<TIME_ENTRIES_LENGTH; j++) {
                if (timeTable[i].timeEntries[j].hour == hour && timeTable[i].timeEntries[j].minute == minutes) {
                    timeTable[i].currentRoomId = timeTable[i].timeEntries[j].roomId;
                    break;
                }
            }
        }
    }
}

void showTime() {
    unsigned char showHour;
    
    showHour = hour+START_HOUR;
    showHour = showHour>12 ? showHour-12 : showHour;
    printf("Time: %u:%02u", showHour, minutes);
}

#define SCREEN_WIDTH 80

void printWordWrapped(char *text) {
    int i, t;
    char *wordStart;
    int wordLen;
    char word[81]; // Max word length

    t=0;
    while (text[t]) {
        // Skip spaces and print them
        while (text[t] == ' ') {
            if (cursorX == 0) {
                // Skip these at beginning of line
            } else if (cursorX >= SCREEN_WIDTH - 1) {
                cursorX = 0;
                cursorY++;
            } else {
                cursorX++;
            }
            t++;
        }

        // Find word length
        wordStart = text+t;
        wordLen = 0;
        while (text[t] && text[t] != ' ' && text[t] != '\n') {
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

        printString(word+i, &cursorX, &cursorY);

        // Handle newline in input
        if (text[t] == '\n') {
            cursorX = 0;
            cursorY++;
            t++;
        }
    }
}

void main() {
    unsigned short visualId = 1, currentImage = 0; // Start in the foyer
    unsigned char i, c, foundActiveCriteria, criteriaFailed, personIndex;
    unsigned char choice;
    char resultString[1024];
    char buffer[80];
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

        // Show the Time
        // TODO: Show other status in top right
        //gotoxy(40, 1);
        //showTime();
        //gotoxy(0, 31);
        cursorX=0;
        cursorY=31;
    
        if (*resultString) {
            printWordWrapped(resultString); 
            cursorY+=2;
            cursorX=0;
            resultString[0] = 0; // Reset after showing
        }

        // Print the room name
        printWordWrapped(getString(currentVisual.nameStringOffset, &currentVisual));
        cursorY+=2;
        cursorX=0;

        // Print the room description
        printWordWrapped(getString(currentVisual.textStringOffset, &currentVisual));
        cursorY+=2;
        cursorX=0;

        // Show any Persons
        if (currentVisual.visualType == ROOM_TYPE) {
            // Clear the persons list
            memset(persons, 0, sizeof(persons));

            personIndex=0;
            for (i=0; i<TIME_TABLE_LENGTH; i++) {
                if (timeTable[i].currentRoomId == gameState[CURRENT_ROOM_ID]) {
                    loadPerson(timeTable[i].id, personIndex);
                    persons[personIndex].timeTableIndex = i;
                    // Print an option to talk to this person
                    // printf("%c: Talk to %s\n", 'A'+personIndex, getString(persons[personIndex].person.nameStringOffset, &persons[personIndex].person));
                    printWordWrapped(getString(persons[personIndex].person.textStringOffset, &persons[personIndex].person));
                    personIndex++;
                }
            }

            // If we have and showed any Persons, add a blank line
            if (persons[0].timeTableIndex) {
                cursorY++;
                cursorX=0;
            }
        }

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
                sprintf(buffer, "%u: ", i);
                printWordWrapped(buffer);
            }

            // Print the choice text
            printWordWrapped(getString(currentVisual.choices[i].textStringOffset, &currentVisual));
            cursorY++;
            cursorX=0;
        }
        
        if (currentVisual.visualType == ROOM_TYPE) {
            personIndex=0;
            for (i=0; i<TIME_TABLE_LENGTH; i++) {
                if (timeTable[i].currentRoomId == gameState[CURRENT_ROOM_ID]) {
                    // Print an option to talk to this person
                    sprintf(buffer, "%c: Talk to %s\n", 'A'+personIndex, getString(persons[personIndex].person.nameStringOffset, &persons[personIndex].person));
                    printWordWrapped(buffer);
                    personIndex++;
                }
            }

            // Always add a choice to wait for a minute if in a room
            printWordWrapped("\nW: Wait for a minute\n");
        } else {
            // Always add a choice to leave this person or thing
            printWordWrapped("\nX: Return to the room\n");
        }

        printWordWrapped("\nChoose an option: ");
        choice = cgetc(); // Convert char to index
        printWordWrapped("\n");

        // See if waiting a minute
        if (currentVisual.visualType == ROOM_TYPE && choice == 'w') {
            advanceTime(1);
            continue;
        }

        // See if exiting this interaction
        if (currentVisual.visualType != ROOM_TYPE && choice == 'x') {
            visualId = gameState[CURRENT_ROOM_ID];
            loadVisual(visualId);
            continue;
        }

        // See if selecting a person
        if (choice >= 'a' && choice <= 'e') {
            choice -= 'a';

            // Invalid choice...too high
            if (choice >= personIndex) {
                continue;
            }

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
                strcpy((char *)resultString, (char*)getString(currentVisual.choices[choice].resultStringOffset, &currentVisual));
            }

            // If the current visual is a Person, see if they are moving
            if (currentVisual.visualType == PERSON_TYPE && currentVisual.choices[choice].personRoomId) {
                timeTable[currentPerson.timeTableIndex].currentRoomId = currentVisual.choices[choice].personRoomId;
            }

            // See if any time has passed
            if (currentVisual.choices[choice].minutes) {
                advanceTime(currentVisual.choices[choice].minutes);
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
    }
}