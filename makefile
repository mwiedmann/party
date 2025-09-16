CC=cl65
EMU=../../x16emur47/x16emu

make:
	$(CC) --cpu 65SC02 -Or -Cl -o ./build/PARTY.PRG -t cx16 \
	src/globals.c src/config.c src/utils.c src/main.c 

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
	node tools/gimp-img-convert.js gfx/bus-girl-mad.data build/BUSGMAD.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/bus-girl-mad.data.pal build/BUSGMAD.PAL
	node tools/gimp-img-convert.js gfx/bus-girl-happy.data build/BUSGHAP.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/bus-girl-happy.data.pal build/BUSGHAP.PAL
	node tools/gimp-img-convert.js gfx/bus-girl-walk.data build/BUSGWALK.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/bus-girl-walk.data.pal build/BUSGWALK.PAL
	node tools/gimp-img-convert.js gfx/apartment.data build/APARTMNT.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/apartment.data.pal build/APARTMNT.PAL
	node tools/gimp-img-convert.js gfx/gate.data build/GATE.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/gate.data.pal build/GATE.PAL
	node tools/gimp-img-convert.js gfx/side-dog.data build/SIDEDOG.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/side-dog.data.pal build/SIDEDOG.PAL
	node tools/gimp-img-convert.js gfx/skyline.data build/SKYLINE.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/skyline.data.pal build/SKYLINE.PAL
	node tools/gimp-img-convert.js gfx/bus-dark.data build/BUSDARK.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/bus-dark.data.pal build/BUSDARK.PAL
	node tools/gimp-img-convert.js gfx/gate-pad.data build/GATEPAD.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/gate-pad.data.pal build/GATEPAD.PAL
	node tools/gimp-img-convert.js gfx/gate-open.data build/GATEOPEN.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/gate-open.data.pal build/GATEOPEN.PAL
	node tools/gimp-img-convert.js gfx/raccoon.data build/RACCOON.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/raccoon.data.pal build/RACCOON.PAL
	node tools/gimp-img-convert.js gfx/cheeseburger.data build/CHEBUR.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/cheeseburger.data.pal build/CHEBUR.PAL
	node tools/gimp-img-convert.js gfx/dog-close.data build/DOGCLS.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/dog-close.data.pal build/DOGCLS.PAL
	node tools/gimp-img-convert.js gfx/front-door.data build/FRTDOOR.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/front-door.data.pal build/FRTDOOR.PAL
	node tools/gimp-img-convert.js gfx/doormat.data build/DOORMAT.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/doormat.data.pal build/DOORMAT.PAL
	node tools/gimp-img-convert.js gfx/doormatu.data build/DOORMATU.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/doormatu.data.pal build/DOORMATU.PAL
	node tools/gimp-img-convert.js gfx/knocker.data build/KNOCKER.BIN 320 240 0 1 1
	node tools/gimp-pal-convert.js gfx/knocker.data.pal build/KNOCKER.PAL

new:
	

zip:
	cd build && \
	rm -f dng.zip && \
	zip dng.zip *

dark:
	node tools/gimp-create-pal.js
