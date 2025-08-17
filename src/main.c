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

void main() {
    unsigned char visualId = 1; // Start in the foyer
    unsigned char i, c, foundActiveCriteria, criteriaFailed;
    unsigned char choice;

    gameState[0] = 1; // Default criteria will always skip/fail

    loadVisual(visualId);

    while (1) {
        // Print the room name
        printf("\n%s\n", getString(currentVisual.nameStringOffset));

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
        choice = getchar() - '0'; // Convert char to index
        printf("\n");

        if (currentVisual.choices[choice].criteria[0].gameStateId == 0) {
            printf("Invalid choice. Try again.\n");
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
            printf("\n%s\n\n", getString(currentVisual.choices[choice].resultStringOffset));
        }

        // Transition to the next visual if there is a valid choice
        if (currentVisual.choices[choice].transitionVisualId != 0) {
            visualId = currentVisual.choices[choice].transitionVisualId;
            loadVisual(visualId);
        }

        //break;
    }
}