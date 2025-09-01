const fs = require("fs")

const rawText = fs.readFileSync("gfx/party.ldtk")
const ldtk = JSON.parse(rawText)

/*
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
  unsigned short personRoomId; // Room to move to if this is a choice on a Person
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
*/

const charConvert = (c) => {
  let charCode = c
  if (charCode >= 97 && charCode <= 122) {
    charCode -= 32 // Convert to PETSCII
  } else if (charCode >= 65 && charCode <= 90) {
    charCode += 32 // Convert to PETSCII
  }
  return charCode
}

const INV_STRING_LEN=30
const inv = require("./inv.json")

const invbindata = new Uint8Array(inv.reduce((prev,curr) => {
  for (let i=0; i<INV_STRING_LEN-1; i++) {
    prev.push(i >= curr.text.length ? 0 : charConvert(curr.text.charCodeAt(i)))
  }
  prev.push(0)
  return prev
}, []))

fs.writeFileSync(`build/INVSTR.BIN`, invbindata, "binary")

const timeEntryCount = 5

const visualTypeValues = {
  "Room": 0,
  "Person": 1,
  "Other": 2
}

const gameState = {}
let gameStateIndex = 2 // 0 is ignored, 1 is always true

const getGameStateIndex = (id) => {
  if (gameState[id] !== undefined) return gameState[id]

  gameState[id] = gameStateIndex++
  return gameState[id]
}

const levelData = {}

ldtk.levels.forEach((level, index) => {
  levelData[level.iid]={
    index: index+1,
    level
  }
})

const visuals = ldtk.levels.map((level) => {
  let stringOffset = 1
  const stringData = []
  let currentString

  const addStringData = (str) => {
    const finalStr=str.replace("{P}",String.fromCharCode(13)+String.fromCharCode(9))
    stringData.push(finalStr)
    return finalStr
  }

  const start = visualTypeValues[level.fieldInstances.find(f => f.__identifier === "start")?.__value]
  
  const visualType = visualTypeValues[level.fieldInstances.find(f => f.__identifier === "visualType")?.__value]
  if (visualType === undefined) {
    throw new Error(`Missing visualType for level ${level.iid}`)
  }

  currentString=level.fieldInstances.find(f => f.__identifier === "name")?.__value
  if (!currentString) {
    throw new Error(`Missing name for level ${level.iid}`)
  }
  currentString=addStringData(currentString)
  const nameStringOffset = stringOffset
  stringOffset += currentString.length + 1 // +1 for null terminator
  
  currentString=level.fieldInstances.find(f => f.__identifier === "text")?.__value
  if (!currentString) {
    throw new Error(`Missing text for level ${level.iid}`)
  }
  currentString=addStringData(currentString)
  const textStringOffset = stringOffset
  stringOffset += currentString.length + 1 // +1 for null terminator

  currentString=level.fieldInstances.find(f => f.__identifier === "image")?.__value
  let imageStringOffset = 0
  if (currentString) {
    currentString=addStringData(currentString)
    imageStringOffset = stringOffset
    stringOffset += currentString.length + 1 // +1 for null terminator
  }

  const choices = level.layerInstances[0].entityInstances.filter(l => l.__identifier === "Choice").map((c) => {
    const transitionBackToRoom = Boolean(c.fieldInstances.find(f => f.__identifier === "transitionBackToRoom")?.__value)

    const transition= c.fieldInstances.find(f => f.__identifier === "transition")?.__value?.levelIid
    const transitionVisualId = transitionBackToRoom ? 32767 :transition ? levelData[transition].index : 0
    
    const criteriaText = c.fieldInstances.find(f => f.__identifier === "criteria").__value

    currentString=c.fieldInstances.find(f => f.__identifier === "text")?.__value
    if (!currentString) {
      throw new Error(`Missing text for choice ${c.iid}`)
    }
    const textStringOffset = stringOffset
    currentString=addStringData(currentString)
    stringOffset += currentString.length + 1 // +1 for null terminator

    currentString=c.fieldInstances.find(f => f.__identifier === "result")?.__value
    let resultStringOffset = 0
    if (currentString) {
      currentString=addStringData(currentString)
      resultStringOffset = stringOffset
      stringOffset += currentString.length + 1 // +1 for null terminator
    }
    
    let personRoomId = c.fieldInstances.find(f => f.__identifier === "personRoom").__value?.levelIid
    personRoomId = personRoomId ? levelData[personRoomId].index : 0
    
    let criteriaRoomId = c.fieldInstances.find(f => f.__identifier === "criteriaRoom").__value?.levelIid
    criteriaRoomId = criteriaRoomId ? levelData[criteriaRoomId].index : 0

    let minutes = c.fieldInstances.find(f => f.__identifier === "minutes").__value

    const choice = {
      canSelect: c.fieldInstances.find(f => f.__identifier === "canSelect")?.__value ? 1 : 0,
      textStringOffset,
      transitionVisualId,
      resultStringOffset,
      personRoomId,
      minutes,
      criteriaRoomId,
      criteria: criteriaText.length>0 ? criteriaText.map((cr) => {
        const parts=cr.split("=")
        return {
          id: getGameStateIndex(parts[0]),
          value: parseInt(parts[1], 10)
        }
      }) : [ { id: 1, value: 0 } ],
      stateChanges: c.fieldInstances.find(f => f.__identifier === "stateChanges").__value.map((sc) => {
        const parts=sc.split("=")
        return {
          id: getGameStateIndex(parts[0]),
          value: parseInt(parts[1], 10)
        }
      })
    }

    return choice
  })

  const timeEntries = level.layerInstances[0].entityInstances.filter(l => l.__identifier === "TimeEntry").map((c) => {
    const hour = c.fieldInstances.find(f => f.__identifier === "hour")?.__value
    const minute = c.fieldInstances.find(f => f.__identifier === "minute")?.__value
    const roomId = levelData[c.fieldInstances.find(f => f.__identifier === "room")?.__value?.levelIid].index

    return {
      hour,
      minute,
      roomId
    }
  })

  return {
    data:{
      start,
      visualType,
      nameStringOffset,
      textStringOffset,
      imageStringOffset,
      choices,
      timeEntries
    },
    stringData
  }
})

//visuals.sort((a,b)=> a.start?-1:1 - b.start?-1:1)

const emptyCriteria = () =>({ id: 0, value: 0 })

const emptyStateChange = () =>({ id: 0, value: 0 })
const emptyChoice = () => ({
  canSelect: 0,
  textStringOffset: 0,
  transitionVisualId: 0,
  resultStringOffset: 0,
  personRoomId: 0,
  minutes: 0,
  criteriaRoomId: 0,
  criteria: [emptyCriteria(), emptyCriteria()],
  stateChanges: [emptyStateChange(), emptyStateChange()],
})

const emptyVisual = () => ({
  visualType: 0,
  nameStringOffset: 0,
  textStringOffset: 0,
  imageStringOffset: 0,
  choices: [emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice(), emptyChoice()],
})

const emptyTimeEntry = () => ({
  hour: 0,
  minute: 0,
  roomId: 0
})

const fullVisual = (v) => {
  const fullV = emptyVisual()
  fullV.visualType = v.visualType || 0
  fullV.nameStringOffset = v.nameStringOffset || 0
  fullV.textStringOffset = v.textStringOffset || 0
  fullV.imageStringOffset = v.imageStringOffset || 0

  v.choices?.forEach((c, i) => {
    fullV.choices[i].canSelect = c.canSelect || 0
    fullV.choices[i].textStringOffset = c.textStringOffset || 0
    fullV.choices[i].transitionVisualId = c.transitionVisualId || 0
    fullV.choices[i].resultStringOffset = c.resultStringOffset || 0
    fullV.choices[i].personRoomId = c.personRoomId || 0
    fullV.choices[i].minutes = c.minutes || 0
    fullV.choices[i].criteriaRoomId = c.criteriaRoomId || 0
    c.criteria?.forEach((cr, ci) => {
      fullV.choices[i].criteria[ci].id = cr.id || 0
      fullV.choices[i].criteria[ci].value = cr.value || 0
    })
    c.stateChanges?.forEach((sc, sci) => {
      fullV.choices[i].stateChanges[sci].id = sc.id || 0
      fullV.choices[i].stateChanges[sci].value = sc.value || 0
    })
  })

  fullV.timeEntries = v.timeEntries

  return fullV
}

const fullVisuals = [...visuals.map(v => ({
    data: fullVisual(v.data),
    stringData: v.stringData
  })
)]

const personsFileData = []

fullVisuals.forEach((vdata,i) => {
  const v= vdata.data
  const stringData = vdata.stringData

  const lowByte = (num) => num & 0xFF
  const highByte = (num) => (num >> 8) & 0xFF

  const filedata = []

  // visualType, nameStringOffset, textStringOffset, imageStringOffset
  filedata.push(v.visualType, lowByte(v.nameStringOffset), highByte(v.nameStringOffset), lowByte(v.textStringOffset), highByte(v.textStringOffset),lowByte(v.imageStringOffset), highByte(v.imageStringOffset))

  v.choices.forEach(c => {
    // canSelect, textStringOffset, transitionVisualId, resultStringOffset, roomId
    filedata.push(c.canSelect, lowByte(c.textStringOffset), highByte(c.textStringOffset), lowByte(c.transitionVisualId), highByte(c.transitionVisualId), lowByte(c.resultStringOffset), highByte(c.resultStringOffset), lowByte(c.personRoomId), highByte(c.personRoomId), c.minutes, lowByte(c.criteriaRoomId), highByte(c.criteriaRoomId))

    c.criteria.forEach(cr => {
      // id, value
      filedata.push(cr.id, cr.value)
    })

    c.stateChanges.forEach(sc => {
      // id, value
      filedata.push(sc.id, sc.value)
    })
  })

  filedata.push(0) // Start with null to make offsets 1-based

  stringData.forEach(s => {
    for (let i = 0; i < s.length; i++) {
      filedata.push(charConvert(s.charCodeAt(i)))
    }
    filedata.push(0) // Null terminator
  })

  const id=i+1

  output = new Uint8Array(filedata)
  fs.writeFileSync(`build/VIS${id}.BIN`, output, "binary")

  if (v.visualType === 1) { // Person
    const currrentRoom = v.timeEntries.find(t => t.hour===0 && t.minute===0).roomId
  
    personsFileData.push(lowByte(id), highByte(id), lowByte(currrentRoom), highByte(currrentRoom))

    for (let t=0; t<timeEntryCount; t++) {
      const timeEntry =  t < v.timeEntries.length ? v.timeEntries[t] : emptyTimeEntry()
      personsFileData.push(timeEntry.hour, timeEntry.minute, lowByte(timeEntry.roomId), highByte(timeEntry.roomId))
    }
  }

  output = new Uint8Array(personsFileData)
  fs.writeFileSync(`build/TIMETBL.BIN`, output, "binary")
});
