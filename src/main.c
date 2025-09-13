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

void showStatus() {
    unsigned char showHour, i;
    char buffer[40];

    cursorX=41;
    cursorY=1;
    showHour = hour+START_HOUR;
    showHour = showHour>12 ? showHour-12 : showHour;
    sprintf(buffer, "%u:%02u %s - %s",
        showHour,
        minutes,
        showHour >= 6 && showHour <= 11 ? "PM" : "AM",
        getString(currentVisual.nameStringOffset, &currentVisual)
    );
    printWordWrapped(buffer);

    cursorX=41;
    cursorY=3;
    printWordWrapped("--INVENTORY--\n");

    for (i=0; i<INV_STRING_COUNT; i++) {
        if (gameState[128+i]) {
            cursorX=41;
            printWordWrapped(invStrings[i]);
            cursorY++;
        }
    }
}

unsigned char pickItem() {
    unsigned char choice, i, gsId, count;

    gsId=0;
    choice = cgetc();

    if (choice >= 'a' && choice <= 'z') {
        choice -= 'a';
    }

    count=0;
    for (i=0; i<INV_STRING_COUNT; i++) {
        if (gameState[128+i]) {
            if (count==choice) {
                // found selected item
                gsId=128+i;
            }
            count++;
        }
    }

    return gsId;
}

unsigned char criteriaCheck(Choice *choice) {
    unsigned char c, foundActiveCriteria, criteriaFailed;

    // Check all criteria
    foundActiveCriteria = 0;
    criteriaFailed = 0;
    for (c=0; c<CRITERIA_COUNT; c++) {
        if (choice->criteria[c].gameStateId != 0) {
            if (gameState[choice->criteria[c].gameStateId] != choice->criteria[c].value) {
                criteriaFailed = 1; // Criteria not met
                break;
            }
            foundActiveCriteria = 1; // At least one criteria is active
        }
    }

    // Check if the current choice is only available in a certain room
    if (choice->criteriaRoomId && choice->criteriaRoomId != gameState[CURRENT_ROOM_ID]) {
        criteriaFailed = 1;
    }

    if (!foundActiveCriteria || criteriaFailed) {
        return 0; // No active criteria or criteria failed, skip this choice
    }
    
    return 1;
}

void main() {
    unsigned short visualId = 1, currentImage = 0, stringOffset, temp; // Start in the foyer
    unsigned char i, personIndex, forcingChoiceId;
    unsigned char choice, showedPerson, selectedGSId;
    char resultString[1024];
    char buffer[80];
    PersonInfo currentPerson;

    // Cursor shows up sometimes. Not sure how to disable.
    // cursor(0) doesn't work so just put it here.
    gotoxy(40,0);

    gameState[0] = 1; // Default criteria will always skip/fail

    init();

    loadInvStrings();
    loadTimeTable();
    stringOffset = loadVisual(visualId, 0);
    
    while (1) {
        if (currentVisual.visualType == ROOM_TYPE) { // Room
            gameState[CURRENT_ROOM_ID] = visualId; // Store current room ID
        }

        // Look for any "force" choices. We take those immediatedly (if criteria ok) and don't draw anything.
        forcingChoiceId=255;
        for (i = 0; i < CHOICE_COUNT; i++) {
            if (currentVisual.choices[i].force) {
                if (!criteriaCheck(&currentVisual.choices[i])) {
                    continue;
                }

                // Found a "force" choice where criteria passes
                forcingChoiceId=i;
                break;
            }
        }

        // If forcing a choice, load the transition and continue the loop
        if (forcingChoiceId!=255) {
            visualId = currentVisual.choices[forcingChoiceId].transitionVisualId;
            if (visualId == TRANSITION_CURRENT_ROOM) {
                visualId = gameState[CURRENT_ROOM_ID];
            }
            stringOffset = loadVisual(visualId, 0);
            continue;
        }

        if (currentVisual.imageStringOffset != 0 && currentImage != currentVisual.imageStringOffset) {
            currentImage = currentVisual.imageStringOffset;
            loadImage(getString(currentVisual.imageStringOffset, &currentVisual));
        }
        
        clearImageArea();

        // Show the Time
        showStatus();
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
            showedPerson=0;
            for (i=0; i<TIME_TABLE_LENGTH; i++) {
                if (timeTable[i].currentRoomId == gameState[CURRENT_ROOM_ID]) {
                    temp = loadPerson(timeTable[i].id, personIndex, stringOffset);
                    persons[personIndex].person.stringDataOffset = stringOffset;
                    stringOffset+= temp;
                    persons[personIndex].timeTableIndex = i;
                    showedPerson=1;

                    printWordWrapped(getString(persons[personIndex].person.textStringOffset, &persons[personIndex].person));
                    cursorY++;
                    cursorX=0;
                    personIndex++;
                }
            }
        }

        // If we have and showed any Persons, add a blank line
        if (showedPerson) {
            cursorY++;
            cursorX=0;
        }

        // Show choices
        for (i = 0; i < CHOICE_COUNT; i++) {
            // No need to show forced choices
            if (currentVisual.choices[i].force) {
                continue;
            }

            // Check all criteria
            if (!criteriaCheck(&currentVisual.choices[i])) {
                continue;
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
            sprintf(buffer, "\nX: Leave %s\n", getString(currentVisual.nameStringOffset, &currentVisual));
            printWordWrapped(buffer);
        }

        // Always add a choice to wait for a minute if in a room
        printWordWrapped("\nI: Use an item\n");

        printWordWrapped("\nChoose an option: ");
        choice = cgetc(); // Convert char to index
        printWordWrapped("\n");

        // Pick inv item
        if (choice == 'i') {
            printWordWrapped("\nPick an item to use: ");
            selectedGSId = pickItem();
        }

        // See if waiting a minute
        if (currentVisual.visualType == ROOM_TYPE && choice == 'w') {
            advanceTime(1);
            continue;
        }

        // See if exiting this interaction
        if (currentVisual.visualType != ROOM_TYPE && choice == 'x') {
            visualId = gameState[CURRENT_ROOM_ID];
            stringOffset = loadVisual(visualId, 0);
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
            stringOffset = loadVisual(visualId, 0);
        } else {
            // Making a non-person selection
            choice -= '0';
        
            if (choice > 9 || currentVisual.choices[choice].criteria[0].gameStateId == 0 || !criteriaCheck(&currentVisual.choices[choice])) {
                // printf("Invalid choice. Try again.\n");
                continue;
            }

            // apply state changes
            for (i = 0; i < STATE_CHANGE_COUNT; i++) {
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
                stringOffset = loadVisual(visualId, 0);
            }
        }
    }
}