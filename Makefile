#HOST=generic
#HOST=mingw
#HOST=haiku
HOST=emcc

CC=gcc
RM=rm
CFLAGS= -Wall -Wextra -O3

ifeq ($(HOST),mingw)
	CFLAGS+= -IC:/ExtProg/mingw64/x86_64-w64-mingw32/include -LC:/ExtProg/mingw64/x86_64-w64-mingw32/lib
endif

COPTS=  $(CFLAGS) -funsigned-char \
	-DRESOURCEPATH=\"$(INSTALLRESOURCEPATH)\" \
	-DSCOREPATH=\"$(INSTALLHISCORES)\" \
	-DCONFIGPATH=\"$(INSTALLCONFIG)\"
LIBS= -lm -lSDL2 -lSDL2_mixer
SRCS= alien.c asylum.c bullet.c file.c keyboard.c maze.c menus.c player.c projectile.c sound.c vdu.c

RESOURCES=data/Resources data/Ego data/Psyche data/Id data/Voices

INSTALLGROUP=games
CHGRP=chgrp
CHMOD=chmod

# For a non-root install, try something like this:
#
#INSTALLBIN=/home/blotwell/bin/asylum
#INSTALLRESOURCEPATH=/home/blotwell/lib/asylum
#INSTALLHISCORES=/home/blotwell/.asylum-hiscores
#INSTALLCONFIG=/home/blotwell/.asylum-config
#
#INSTALLGROUP=foo
#CHGRP=echo
#CHMOD=echo
ifeq ($(HOST),haiku)
	CC=i586-pc-haiku-gcc
	COPTS+=$(CPPFLAGS) -D_NO_SOUND
	INSTALLBIN=/boot/common/games/asylum/asylum
	INSTALLRESOURCEPATH=/boot/common/games/asylum/data
	INSTALLHISCORES=/boot/common/games/asylum/hiscores
	INSTALLCONFIG=/boot/common/games/asylum/config
	OS_SOURCE=asylum_haiku.c
	LIBS=-lSDL2_mixer -lSDL2 -lbe -lroot -ldevice -lgame -ltextencoding -lmedia
endif
ifeq ($(HOST),mingw)
	INSTALLBIN="c:/program files/asylum/asylum.exe"
	INSTALLRESOURCEPATH="c:/program files/asylum/data"
	INSTALLHISCORES="c:/program files/asylum/hiscores"
	INSTALLCONFIG="c:/program files/asylum/config"
	OS_SOURCE=asylum_win.c
	RM=del
	EXE=.exe
	LIBS=-lmingw32 -lSDL2_mixer -lSDL2main -lSDL2 -mwindows
endif
ifeq ($(HOST),generic)
	INSTALLBIN=/usr/games/asylum
	INSTALLRESOURCEPATH=/usr/share/games/asylum
	INSTALLHISCORES=/var/games/asylum
	INSTALLCONFIG=/var/games/asylum
endif
ifeq ($(HOST),emcc)
	CC=emcc
	INSTALLBIN=/usr/games/asylum
	INSTALLRESOURCEPATH=/usr/share/games/asylum
	INSTALLHISCORES=/var/games/asylum
	INSTALLCONFIG=/var/games/asylum
	LIBS=-sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sASYNCIFY -sINITIAL_MEMORY=32MB -sTOTAL_STACK=1MB -sALLOW_MEMORY_GROWTH \
		 -sEXIT_RUNTIME=1 -o index.html --preload-file=data --shell-file custom_shell.html -lidbfs.js
endif

default: build

ifneq ($(HOST),mingw)
$(INSTALLBIN): asylum$(EXE) Makefile
	cp asylum$(EXE) $(INSTALLBIN)
	$(CHGRP) $(INSTALLGROUP) $(INSTALLBIN)
	$(CHMOD) g+s $(INSTALLBIN)
	$(CHMOD) a+x $(INSTALLBIN)

install-resources: $(RESOURCES) Makefile
	mkdir -p $(INSTALLRESOURCEPATH)
	cp -r $(RESOURCES) $(INSTALLRESOURCEPATH)/
	$(CHGRP) -R $(INSTALLGROUP) $(INSTALLRESOURCEPATH)/
	$(CHMOD) -R a+rX $(INSTALLRESOURCEPATH)/

install-hiscores: Makefile
	mkdir -p $(INSTALLHISCORES)
	touch $(INSTALLHISCORES)/EgoHighScores
	touch $(INSTALLHISCORES)/PsycheHighScores
	touch $(INSTALLHISCORES)/IdHighScores
	touch $(INSTALLHISCORES)/ExtendedHighScores
	$(CHGRP) -R $(INSTALLGROUP) $(INSTALLHISCORES)/*
	$(CHMOD) -R 660 $(INSTALLHISCORES)/*

install-config: Makefile
	mkdir -p $(INSTALLCONFIG)

install-binary: $(INSTALLBIN)

install: install-resources install-hiscores install-config install-binary

uninstall:
	rm -rf $(INSTALLBINARY) $(INSTALLRESOURCEPATH) $(INSTALLHISCORES)
endif

oggs:
	bash -c 'pushd data; for i in */Music?; do pushd ..; ./asylum --dumpmusic $$i `if (echo \$$i|grep Resources.Music2>/dev/null); then echo -n --slower; fi`; \
	popd;\
	tail -c +33 $$i.au| \
	oggenc - --raw --raw-endianness=1 --raw-rate=44100 --artist="Andy Southgate" \
	--album="Background music for Asylum computer game" \
	>$$i.ogg;\
	rm $$i.au;\
	done; popd'

build: asylum$(EXE)

asylum$(EXE): $(SRCS) $(OS_SOURCE) asylum.h Makefile
	$(CC) $(COPTS) $(LDFLAGS) -o asylum$(EXE) $(SRCS) $(OS_SOURCE) $(LIBS)

clean:
	$(RM) asylum$(EXE)

