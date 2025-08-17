#include <cx16.h>
#include <stdio.h>
#include <string.h>
#include <cx16.h>
#include <cbm.h>
#include <6502.h>

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
  unsigned char textStringId;
  unsigned char transitionVisualId; // Visual to transition to
  unsigned char resultStringId; // Shows after you pick this choice
  Criteria criteria[2];
  StateChange stateChanges[2];
} Choice;

typedef struct Visual {
  unsigned char nameStringId;
  unsigned char textStringId;
  Choice choices[10];
} Visual;

char globalStrings[50][100] = {
  "",
  "The Foyer",
  "You are standing in the foyer.",
  "Go to the Dining Room",
  "Go to the Kitchen",
  "The Dining Room",
  "You are standing in the Dining Room",
  "Go to the Foyer",
  "A very hungry looking dog is staring at you.",
  "Feed the dog your steak.",
  "The dog ate the steak and wagged his tail.",
  "The Kitchen",
  "You are standing in the kitchen. The chef is cooking something..",
  "Talk to the chef",
  "Go to the foyer",
  "The Chef",
  "The chef is awaiting your order.",
  "Leave the chef",
  "Order the steak",
  "The chef makes you a steak.",
  "Order some pasta",
  "The chef recommends the steak instead."
};

char gameState[100]; // [0] is 1, so default criteria will fail

Visual visuals[10];

void makeHouse() {
    // Set visuals memory to all 0 (assuming you want to clear the first N entries)
    memset(visuals, 0, sizeof(Visual) * 10); // Change 10 to however many visuals you expect

    gameState[0] = 1;

    visuals[1].nameStringId = 1;
    strcpy(globalStrings[1], "The Foyer");
    visuals[1].textStringId = 2;
    strcpy(globalStrings[2], "You are in the foyer.");

    // To Dining Room
    visuals[1].choices[0].canSelect = 1;
    visuals[1].choices[0].transitionVisualId = 2;
    visuals[1].choices[0].criteria[0].gameStateId = 1;
    visuals[1].choices[0].textStringId = 3;
    strcpy(globalStrings[3], "Go to the Dining Room");

    // To Kitchen
    visuals[1].choices[1].canSelect = 1;
    visuals[1].choices[1].transitionVisualId = 3;
    visuals[1].choices[1].criteria[0].gameStateId = 1;
    visuals[1].choices[1].textStringId = 4;
    strcpy(globalStrings[4], "Go to the Kitchen");

    // The Dining Room
    visuals[2].nameStringId = 5;
    strcpy(globalStrings[5], "The Dining Room");
    visuals[2].textStringId = 6;
    strcpy(globalStrings[6], "You are in the dining room.");

    // The Kitchen Room
    visuals[3].nameStringId = 7;
    strcpy(globalStrings[7], "The Kitchen");
    visuals[3].textStringId = 8;
    strcpy(globalStrings[8], "You are in the kitchen.");

    // Kitchen - Back to Foyer
    visuals[3].choices[0].canSelect = 1;
    visuals[3].choices[0].transitionVisualId = 1;
    visuals[3].choices[0].criteria[0].gameStateId = 1;
    visuals[3].choices[0].textStringId = 9;
    strcpy(globalStrings[9], "Go to the Foyer");

    // Dining Room - Back to Foyer
    visuals[2].choices[0].canSelect = 1;
    visuals[2].choices[0].transitionVisualId = 1;
    visuals[2].choices[0].criteria[0].gameStateId = 1;
    visuals[2].choices[0].textStringId = 9;

    // Dining Room - Dog looking at you
    visuals[2].choices[1].criteria[0].gameStateId = 1;
    visuals[2].choices[1].textStringId = 10;
    strcpy(globalStrings[10], "There is a hungry looking dog starting at you.");

    // Dining Room - Feed the Dog
    visuals[2].choices[2].canSelect = 1;
    visuals[2].choices[2].criteria[0].gameStateId = 2;
    visuals[2].choices[2].criteria[0].value = 1; // Player has food
    visuals[2].choices[2].textStringId = 11;
    visuals[2].choices[2].stateChanges[0].id = 2; // Player has fed the dog
    visuals[2].choices[2].stateChanges[0].value = 0; // Player no longer has food
    strcpy(globalStrings[11], "Feed the Dog");
    visuals[2].choices[2].resultStringId = 12; // Result message
    strcpy(globalStrings[12], "The dog happily eats the food and wags its tail.");

    // Kitchen - Talk to the Chef
    visuals[3].choices[1].canSelect = 1;
    visuals[3].choices[1].criteria[0].gameStateId = 1;
    visuals[3].choices[1].textStringId = 13;
    visuals[3].choices[1].transitionVisualId = 4; // Transition to the Chef
    strcpy(globalStrings[13], "Talk to the Chef");

    // Chef
    visuals[4].nameStringId = 14;
    strcpy(globalStrings[14], "The Chef");
    visuals[4].textStringId = 15;
    strcpy(globalStrings[15], "The chef can cook you something.");

    // Chef - Cook steak
    visuals[4].choices[0].canSelect = 1;
    visuals[4].choices[0].criteria[0].gameStateId = 1;
    visuals[4].choices[0].textStringId = 16;
    strcpy(globalStrings[16], "Ask the chef to cook you a steak");
    visuals[4].choices[0].resultStringId = 17; // Result message
    strcpy(globalStrings[17], "The chef cooks you a delicious steak.");
    visuals[4].choices[0].transitionVisualId = 3; // Transition back to the kitchen
    visuals[4].choices[0].stateChanges[0].id = 2; // Player has steak
    visuals[4].choices[0].stateChanges[0].value = 1;
}

void loadHouse() {
    cbm_k_setlfs(0, 8, 2);
	cbm_k_setnam("gamedata.bin");
	cbm_k_load(0, ((unsigned short)&visuals));

    // Clear game state
    memset(gameState, 0, sizeof(gameState));
    gameState[0] = 1; // Default criteria will always skip/fail
}

void main() {
    unsigned char visualId = 1; // Start in the foyer
    unsigned char i, c, foundActiveCriteria, criteriaFailed;
    unsigned char choice;
    Visual *currentVisual;

    //makeHouse();
    loadHouse();

    while (1) {
        currentVisual = &visuals[visualId];
        
        // Print the room name
        printf("\n%s\n", globalStrings[currentVisual->nameStringId]);

        // Print the room description
        printf("%s\n\n", globalStrings[currentVisual->textStringId]);

        // Show choices
        for (i = 0; i < 10; i++) {
            // Check all criteria
            foundActiveCriteria = 0;
            criteriaFailed = 0;
            for (c=0; c<2; c++) {
                if (currentVisual->choices[i].criteria[c].gameStateId != 0) {
                    if (gameState[currentVisual->choices[i].criteria[c].gameStateId] != currentVisual->choices[i].criteria[c].value) {
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
            if (currentVisual->choices[i].canSelect) {
                printf("%d: ", i);
            }

            // Print the choice text
            printf("%s\n", globalStrings[currentVisual->choices[i].textStringId]);
        }
        
        printf("\nChoose an option: ");
        choice = getchar() - '0'; // Convert char to index
        printf("\n");

        if (currentVisual->choices[choice].criteria[0].gameStateId == 0) {
            printf("Invalid choice. Try again.\n");
            continue;
        }

        // apply state changes
        for (i = 0; i < 2; i++) {
            if (currentVisual->choices[choice].stateChanges[i].id != 0) {
                gameState[currentVisual->choices[choice].stateChanges[i].id] = currentVisual->choices[choice].stateChanges[i].value;
            }
        }
        
        // Show result message if any
        if (currentVisual->choices[choice].resultStringId != 0) {
            printf("\n%s\n\n", globalStrings[currentVisual->choices[choice].resultStringId]);
        }

        // Transition to the next visual if there is a valid choice
        if (currentVisual->choices[choice].transitionVisualId != 0) {
            visualId = currentVisual->choices[choice].transitionVisualId;
        }

        //break;
    }
}