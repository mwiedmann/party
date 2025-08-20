const fs = require("fs");

const palName = process.argv[2];
const palOutputFilename = process.argv[3];

console.log(
  `Reading Gimp palette file ${palName}`
);

const palData = fs.readFileSync(palName);

const finalPal = []

const adjustColor = (c) => c>>4

// The raw data is just a long array of R,G and B bytes
// Convert them to G+B and R (4 bytes each)
for (let i = 0; i < palData.length; i+=3) {
    finalPal.push((adjustColor(palData[i+1])<<4) + adjustColor(palData[i+2]))
    finalPal.push(adjustColor(palData[i]))
}

let output = new Uint8Array(finalPal);
fs.writeFileSync(palOutputFilename, output, "binary");

console.log(`Generated CX16 palette file ${palOutputFilename}`);
