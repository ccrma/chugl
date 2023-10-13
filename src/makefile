# chugin name
CHUGIN_NAME=ChuGL
# suffice
CHUGIN_SUFFIX=.chug

# ---------------------------------------------------------------------------- #
# you won't generally need to change anything below this line for a new chugin #
# ---------------------------------------------------------------------------- #

CHUG=$(addsuffix $(CHUGIN_SUFFIX),$(CHUGIN_NAME))

########################## DEFAULT MAKE TARGET ################################
# default target: print usage message and quit
current:
	@echo "[chugl build]: please use one of the following configurations:"
	@echo "   make mac, make linux, make win"

# creates build-release
build-release:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release

.PHONY: mac osx linux linux-oss linux-jack linux-alsa linux-all
mac osx linux linux-oss linux-jack linux-alsa linux-all: build-release
	cmake --build build-release
	cp build-release/ChuGL.chug .

.PHONY: win win32 win64
win win32 win64: build-release
	@echo '------------'
	@echo '(windows) --> build ChuGL.chug in Visual Studio from build-release/chugl.sln'

clean: 
	rm -rf $(CHUG) build-release
