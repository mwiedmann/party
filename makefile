CC=cl65
EMU=../../x16emur47/x16emu

make:
	$(CC) --cpu 65SC02 -Or -Cl -o ./build/PARTY.PRG -t cx16 \
	src/main.c src/utils.c

run:
	cd build && \
	$(EMU) -prg PARTY.PRG -run

emu:
	$(EMU)

gdata:
	rm -f build/VIS*.BIN
	node tools/gamedata.js

img:
	node tools/gimp-img-convert.js gfx/bus-stop.data build/BUSSTOP.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/bus-stop.data.pal build/BUSSTOP.PAL

zip:
	cd build && \
	rm -f dng.zip && \
	zip dng.zip *
