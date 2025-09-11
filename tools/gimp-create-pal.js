const fs = require("fs");


const palData=[]

for (let b=0; b<8; b++) {
  for (let g=0; g<8; g++) {
    for (let r=0; r<4; r++) {
      palData.push(r<<4,g<<4,b<<4)
    }
  } 
}

let output = new Uint8Array(palData);
fs.writeFileSync("gimp-dark.pal", output, "binary");
