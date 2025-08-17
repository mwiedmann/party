CC=cl65
EMU=../../x16emur47/x16emu

make:
	$(CC) --cpu 65SC02 -Or -Cl -o ./build/PARTY.PRG -t cx16 \
	src/main.c

run:
	cd build && \
	$(EMU) -prg PARTY.PRG -run

emu:
	$(EMU)

gdata:
	node tools/gamedata.js

zip:
	cd build && \
	rm -f dng.zip && \
	zip dng.zip *
