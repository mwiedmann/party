const fs = require("fs");

const inputFile = process.argv[2];
const outputFile = process.argv[3];

const frameWidth = process.argv[4];
const frameHeight = process.argv[5];
const startingTile = process.argv[6];
const xTiles = process.argv[7];
const yTiles = process.argv[8];

const imageData = [...fs.readFileSync(inputFile)].slice(startingTile * frameWidth * frameHeight);

const flattenedTiles = [];

let ty, tx, y, x, start, pixelIdx;

for (ty = 0; ty < yTiles; ty++) {
  for (tx = 0; tx < xTiles; tx++) {
    for (y = 0; y < frameHeight; y++) {
      start =
        ty * xTiles * frameWidth * frameHeight +
        tx * frameWidth +
        y * xTiles * frameWidth;
      for (x = 0; x < frameWidth; x++) {
        pixelIdx = start + x;
        flattenedTiles.push(imageData[pixelIdx]);
      }
    }
  }
}

output = new Uint8Array([...flattenedTiles]);
fs.writeFileSync(outputFile, output, "binary");

console.log(`Generated file ${outputFile}`);
