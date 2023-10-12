# chugin name
CHUGIN_NAME=ChuGL
# suffice
CHUGIN_SUFFIX=.chug

# ---------------------------------------------------------------------------- #
# you won't generally need to change anything below this line for a new chugin #
# ---------------------------------------------------------------------------- #

CHUG=$(addsuffix $(CHUGIN_SUFFIX),$(CHUGIN_NAME))

.PHONY: mac osx linux linux-oss linux-jack linux-alsa linux-all
mac osx linux linux-oss linux-jack linux-alsa linux-all: all

.PHONY: win32 win
win32 win: build-release

# creates build-release, build, cp .chug to src/
all: build-release
	cmake --build build-release
	cp build-release/ChuGL.chug .

# creates build-release
build-release:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release

clean: 
	rm -rf $(CHUG) build-release
