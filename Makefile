HOST=generic
#HOST=mingw
#HOST=haiku

CC=g++
RM=rm
CFLAGS= -O3
COPTS=  $(CFLAGS) -funsigned-char \
	-DRESOURCEPATH=\"$(INSTALLRESOURCEPATH)\" \
	-DSCOREPATH=\"$(INSTALLHISCORES)\"
LIBS= -lm -lSDL -lSDL_mixer -lGL -lGLU
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
	OS_SOURCE=asylum_haiku.c
	LIBS=-lSDL_mixer -lSDL -lbe -lroot -ldevice -lgame -lGL -ltextencoding -lmedia
endif
ifeq ($(HOST),mingw)
	INSTALLBIN="c:/program files/asylum/asylum.exe"
	INSTALLRESOURCEPATH="c:/program files/asylum/data"
	INSTALLHISCORES="c:/program files/asylum/hiscores"
	OS_SOURCE=asylum_win.c
	RM=del
	EXE=.exe
	LIBS=-lm -lmingw32 -lSDL_mixer -lSDLmain -lSDL -mwindows
endif
ifeq ($(HOST),generic)
	INSTALLBIN=/usr/games/asylum
	INSTALLRESOURCEPATH=/usr/share/games/asylum
	INSTALLHISCORES=/var/games/asylum
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

install-binary: $(INSTALLBIN)

install: install-resources install-hiscores install-binary

uninstall:
	rm -rf $(INSTALLBINARY) $(INSTALLRESOURCEPATH) $(INSTALLHISCORES)
endif

oggs:
	bash -c 'pushd data; for i in */Music?; do pushd ..; ./asylum --dumpmusic $$i `if (echo \$$i|grep Resources.Music2>/dev/null); then echo -n --slower; fi`; \
	popd;\
	tail -c +33 $$i.au| \
	oggenc - --raw --raw-endianness=1 --raw-rate=22050 --artist="Andy Southgate" \
	--album="Background music for Asylum computer game" \
	>$$i.ogg;\
	rm $$i.au;\
	done; popd'

build: asylum$(EXE)

asylum$(EXE): $(SRCS) $(OS_SOURCE) asylum.h Makefile
	$(CC) $(COPTS) $(LDFLAGS) -o asylum$(EXE) $(SRCS) $(OS_SOURCE) $(LIBS)

clean:
	$(RM) asylum$(EXE)

