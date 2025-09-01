#ifndef GLOBALS_H
#define GLOBALS_H

#define CURRENT_ROOM_ID 255 // Last byte of gameState to store current room ID
#define ROOM_TYPE 0
#define PERSON_TYPE 1
#define TRANSITION_CURRENT_ROOM 32767
#define PERSONS_PER_ROOM 5
#define TIME_TABLE_LENGTH 50
#define TIME_ENTRIES_LENGTH 5
#define START_HOUR 8

#define SCREEN_WIDTH 80
#define INV_STRING_COUNT 32
#define INV_STRING_LENGTH 30

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

extern char gameState[256];
extern char invStrings[INV_STRING_COUNT][INV_STRING_LENGTH];

extern Visual currentVisual;
extern PersonInfo persons[5];
extern Person timeTable[TIME_TABLE_LENGTH];

extern unsigned char hour;
extern unsigned char minutes;
extern unsigned char cursorX;
extern unsigned char cursorY;

#endif