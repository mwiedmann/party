#include <cx16.h>
#include <stdio.h>
#include <string.h>
#include <cx16.h>
#include <cbm.h>
#include <6502.h>
#include <conio.h>

#include "utils.h"
#include "globals.h"
#include "config.h"

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