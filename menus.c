/*  menus.c */

/*  Copyright Hugh Robinson 2006-2008.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <SDL/SDL.h>

#include "asylum.h"

extern fastspr_sprite charsadr[48];

extern char sound_available;
extern asylum_options options;
extern char plscore[8];

char highscorearea[13*5+1];           //=&D000
char hstindex;

int escapehandler()
{
    //frameinc = 1;
    showchatscreen();
    swi_fastspr_clearwindow();
    wipetexttab();
    message(36, 40-256, 0, 4, "¤ Game Interrupted ¤");
    message(32-256, 72, 4, 0, "Please select action");
    message(46+256, 104, -4, 0, "ESC - Abandon game");
    message(40-256, 136, 4, 0, "Q    - lose this life");
    message(40, 168+256, 0, -4, "R    - Return to Game");
    message(40, 200+256, 0, -4, "O    - Alter Options");
    showtext();
    osbyte_7c(); //clear escape
    for (int r9 = 64; swi_readescapestate() == 0;)
    {
       loopb8:
        swi_blitz_wait(0);
        if (r9 != 0)
        {
            r9--;
            switchbank();
            swi_fastspr_clearwindow();
            texthandler(1);
        }
        else if (need_redraw())
        {
            showchatscreen();
            showtext();
        }
        switch (osbyte_79_unicode(1))
        {
            case 'q': case 'Q':
                loselife();
                return 0;
            case 'o': case 'O':
                adjustopt();
                return 0;
            case 'r': case 'R':
                rejoin();
                return 0;
        }
    }
    osbyte_7c();
    return 1;
}

void rejoin()
{
    wipetexttab();
    showgamescreen();
}

void adjustopt()
{
    options_menu(1); // mark as in game
    rejoin();
}

int options_menu(int gameon)
{
    while (1)
    {
        clearkeybuf();
        wipetexttab();
        showchatscreen();
        swi_fastspr_clearwindow();
        message(128, 48, 0, 0, "Options");
        message(32, 96, 0, 0, "1. Define Controls");
        message(32, 128, 0, 0, "2. Tune Game Options");
        message(88, 224, 0, 0, "Fire - Play");
        if (gameon == 0) message(32, 160, 0, 0, "3. Choose Mental Zone");
        else message(32, 160, 0, 0, "3. Save Position");
// if (savedornot==1) message(32,192,0,0,"4. Save Settings");
        showtext();
        swi_blitz_wait(20); //
        switch (readopt((gameon == 0) ? 3 : 3))
        {
        case -1: optionexit: return 1;
        case  1: choosecontrol(); dosaveconf(); break;
        case  2: tunegame(); dosaveconf(); break;
        case  3: if (gameon == 0)
            {
                getzone(); dosaveconf();
            }
            else
                savegame();
            break;
//case  4: if (savedornot==1) dosaveconf(); break;
        default: soundupdate(); return 0;
        }
    }
}

void dosaveconf()
{
    saveconfig();
}

void getzone()
{
    int r0;

    wipetexttab();
    showchatscreen();
    message(88, 48, 0, 0, "Choose which");
    message(96, 68, 0, 0, "mental zone");
    message(64, 96, 0, 0, "1. Ego");
    message(64, 128, 0, 0, "2. Psyche");
    message(64, 160, 0, 0, "3. Id");
    if (checkifextend())
    {
        message(64, 192, 0, 0, "4. Saved Game");
        r0 = 4;
    }
    else r0 = 3;
    showtext();
    r0 = readopt(r0);
    if (r0 == -1) return;
    if ((r0 == 3) && ( /*testid:*/ options.idpermit != 1))
    {
        wipetexttab();
        showchatscreen();
        message(80, 100, 0, 0, "To play the id");
        message(32, 128, 0, 0, "you need to complete");
        message(80, 156, 0, 0, "the ego first.");
        showtext();
        readopt(0);
    }
    else options.mentalzone = r0;
}

void completedzone()
{
   completedzone:
    message(114, 64, 0, 0, "WELL DONE!");
    message(44, 96, 0, 0, "You have completed");
    if (options.mentalzone == 3) message(114, 128, 0, 0, "The game!!!");
    else message(114, 128, 0, 0, "This zone!");
}

void choosecontrol()
{
    wipetexttab();
    showchatscreen();
    message(96, 48, 0, 0, "Controls");
    message(64, 96, 0, 0, "1. Keyboard");
    message(64, 128, 0, 0, "2. Joystick");
    showtext();
    switch (readopt(2))
    {
    case-1: return;
    case  1: choosekeys(); return;
    case  2: choosestick(); return;
    }
}

void choosekeys()
{
    wipetexttab();
    showchatscreen();
    options.leftkey = selectkey(48, 128, 0, 0, "Press Key For Left");
    options.rightkey = selectkey(48, 128, 0, 0, "Press Key For Right");
    options.upkey = selectkey(48, 128, 0, 0, "Press Key For Up");
    options.downkey = selectkey(48, 128, 0, 0, "Press Key For Down");
    options.firekey = selectkey(48, 128, 0, 0, "Press Key to Fire");
}

void choosestick()
{
    wipetexttab();
    showchatscreen();
    message(88, 48, 0, 0, "Joystick");
    message(64, 96, 0, 0, "Joystick Number");
    message(136, 128, 0, 0, "0-3");
    showtext();
    options.joyno = 1+readopt(3);
    if (options.joyno == 0) return;
    wipetexttab();
    showchatscreen();
    message(96, 48, 0, 0, "Reminder");
    message(48, 96, 0, 0, "To jump, you can");
    message(32, 128, 0, 0, "1. Use Joystick Up");
    message(32, 160, 0, 0, "2. Use Fire Button 2");
    message(64, 180, 0, 0, "If you have one.");
    message(32, 212, 0, 0, "3. Use The Up Key");
    showtext();
    (void)readopt(9);
}


void tunegame()
{
    wipetexttab();
    showchatscreen();
    message(96, 48, 0, 0, "Tune Game");
    if (sound_available)
    {
        message(64, 96, 0, 0, "1. Sound System");
        message(64, 128, 0, 0, "2. Sound Volume");
    }
    else
    {
        message(48, 96, 0, 0, "\x11");
        message(48, 128, 0, 0, "\x11");
        message(64, 96, 0, 0, "1.");
        message(64, 128, 0, 0, "2.");
        message(115, 101, 0, 0, "Sound Not");
        message(112, 123, 0, 0, "Available");
    }
    message(64, 160, 0, 0, "3. Video System");
    showtext();
    while (1)
        switch (readopt(3))
        {
        case-1: return;
        case  1: if (sound_available)
            {
                tunesound(); return;
            }
            else break;
        case  2: if (sound_available)
            {
                tunevolume(); return;
            }
            else break;
        case  3: tunespeed(); return;
        }
}

char sound1[] = "-1. No Sound";
char sound2[] = "-2. 4 Channels";
char sound3[] = "-3. 4 Channels";
char sound4[] = "-4. 8 Channels";
char sound5[] = "-5. Normal Quality";
char sound6[] = "-6. High Quality";
char sound7[] = "-7. Overdrive";

void tunesound()
{
    showchatscreen();
    for (;; /*tunesoundloop:*/ soundupdate(), swi_stasis_link(1, 1), swi_sound_control(1, -15, 0x20, 0xfe))
    {
       tunesoundins:
        wipetexttab();
        soundfillin();
        message(96, 32, 0, 0, "Tune Sound");
        message(32, 60, 0, 0, sound1);
        message(32, 80, 0, 0, sound2);
        message(32, 100, 0, 0, sound3);
        message(80, 120, 0, 0, "and music");
        message(32, 140, 0, 0, sound4);
        message(32, 160, 0, 0, sound5);
        message(32, 180, 0, 0, sound6);
        message(32, 200, 0, 0, sound7);
        message(96, 220, 0, 0, "ESC - Exit");
        swi_blitz_wait(0);
        swi_fastspr_clearwindow();
        showtext();

        switch (readopt(7))
        {
        case 1: options.soundtype = 0; break;
        case 2: options.soundtype = 1; break;
        case 3: options.soundtype = 2; break;
        case 4: options.soundtype = 3; break;
        case 5: options.soundquality &= ~1; break;
        case 6: options.soundquality |= 1; break;
        case 7: options.soundquality ^= 2; break;
        default: return;
        }
    }
}

void soundfillin()
{
    sound1[0] = (options.soundtype == 0) ? 16 : 17;
    sound2[0] = (options.soundtype == 1) ? 16 : 17;
    sound3[0] = (options.soundtype == 2) ? 16 : 17;
    sound4[0] = (options.soundtype == 3) ? 16 : 17;
    sound5[0] = (options.soundquality&1) ? 17 : 16;
    sound6[0] = (options.soundquality&1) ? 16 : 17;
    sound7[0] = (options.soundquality&2) ? 16 : 17;
}

char tunevol1[] = "-5. Speaker on";

void tunevolume()
{
    showchatscreen();
    wipetexttab();
    if (sound_available && (options.soundtype == 2)) swi_bodgemusic_start(1, 0);
    swi_bodgemusic_volume(options.musicvol);
    message(80, 32, 0, 0, "Change volume");
    message(48, 96, 0, 0, "1. Louder effects");
    message(48, 116, 0, 0, "2. Quieter effects");
    message(48, 136, 0, 0, "3. Louder music");
    message(48, 156, 0, 0, "4. Quieter music");
    message(48-14, 176, 0, 0, tunevol1);
    message(96, 220, 0, 0, "ESC - Exit");
    do
    {
       tunevolumeloop:
        swi_bodgemusic_volume(options.musicvol);
        if (swi_sound_speaker(0)) *tunevol1 = 17;
        else *tunevol1 = 16;
        swi_blitz_wait(0);
        swi_fastspr_clearwindow();
        showtext();
        int r0 = readopt(5);
        if (r0 == -1) return;

        if (r0 == 1)
        {
            if (options.soundvol < 0x40) options.soundvol = options.soundvol*2+1;
	    maketestsound(options.soundvol);
	    continue;
        }
        if (r0 == 2)
        {
            if (options.soundvol > 0) options.soundvol = (options.soundvol-1)/2;
	    maketestsound(options.soundvol);
	    continue;
        }
        if (r0 == 3)
        {
            if (options.musicvol < 0x40) options.musicvol = options.musicvol*2+1;
	    maketestsound(options.musicvol);
	    continue;
        }
        if (r0 == 4)
        {
            if (options.musicvol > 0) options.musicvol = (options.musicvol-1)/2;
	    maketestsound(options.musicvol);
	    continue;
        }

        if (r0 != 5) return;
        swi_sound_speaker(3-swi_sound_speaker(0));
    }
    while (1);
}


char speed1[] = "-1. Full Screen";
char speed2[] = "-2. Use OpenGL";
char speed3[] = "-4. Half scale";
char sizedesc[][16] = {" 3.  320 x  256",
		       " 3.  640 x  512",
		       " 3.  960 x  768",
		       " 3. 1280 x 1024"};

void tunespeed()
{
    do
    {
        showchatscreen();
        do
        {
           tunespeedloop:
            wipetexttab();
            if (options.fullscreen == 1) speed1[0] = 16;
            else speed1[0] = 17;
            if (options.opengl == 1) speed2[0] = 16;
            else speed2[0] = 17;
            if (options.scale == 2) speed3[0] = 16;
            else speed3[0] = 17;
            message(96, 48, 0, 0, "Tune Video");
            message(32, 96, 0, 0, speed1);
            message(32, 120, 0, 0, speed2);
	    if (options.opengl)
	    {
		message(32, 144, 0, 0, sizedesc[options.size&3]);
		message(32, 168, 0, 0, speed3);
		message(80, 188, 0, 0, "-experimental-");
	    }
            message(96, 220, 0, 0, "ESC - Exit");

            swi_blitz_wait(0);
            swi_fastspr_clearwindow();
            showtext();
            int r0 = readopt(4);
            if (r0 == -1) return;
            else if (r0 == 1)
            {
                options.fullscreen ^= 1;
		vduread(options); break;
            }
            else if (r0 == 2)
            {
                options.opengl ^= 1;
		if (options.opengl == 0)
		{
		    options.size = 0;
		    options.scale = 1;
		}
		vduread(options);
		getvitalfiles();
		getgamefiles();
		getlevelsprites();
		break;
            }
	    else if (options.opengl == 0);
            else if (r0 == 3)
            {
                options.size = (options.size+1) % 4;
		vduread(options);
		getvitalfiles();
		getgamefiles();
		getlevelsprites();
		break;
            }
            else if (r0 == 4)
            {
                options.scale ^= 3;
		vduread(options); break;
            }
        }
        while (1);
    }
    while (1);
}

int selectkey(int x, int y, int xv, int yv, const char* a)
{
    int r1;

    wipetexttab();
    showchatscreen();
    clearkeybuf();
    message(x, y, xv, yv, a);
    showtext();
    do
    {
        if (need_redraw())
        {
            showchatscreen();
            showtext();
        }
        swi_blitz_wait(1);
    }
    while ((r1 = osbyte_79(0)) == -1); // scan keyboard
    if (swi_readescapestate()) return 0;
    return -r1;                        // and r4 (?)
}

int readopt(int maxopt)
{
    int r1;

    for (;;)
    {
       keyloop:
        if (options.joyno != 0)
        {
            swi_joystick_read(options.joyno-1, NULL, NULL);
// MOVVS R0,#0
//if (r0&(1<<16)) {/*optfire:*/ return 0;}
        }
       nooptstick:
        r1 = osbyte_79_unicode(1); // read key in time limit
        if (swi_readescapestate())
        {
           optescape:
            osbyte_7c(); // clear escape
            return -1;
        }
        if (r1 >= '0' && r1 <= '0' + maxopt)
            return r1 - '0';
        if (osbyte_81(options.firekey) == 0xff)
            return 0;
        if (need_redraw())
        {
            showchatscreen();
            showtext();
        }
        swi_blitz_wait(1);
    }
}

const int _x = 250;
const int _v = -1;

int prelude()
{
    int cheatpermit = 0;
    //frameinc = 1;
    showchatscreen();
    swi_fastspr_clearwindow();
    wipetexttab();
    message(2048, _x, 0, _v, "Digital Psychosis");
    message(2108, _x+24, 0, _v, "Presents");
    message(2152, _x+(24*5)/2, 0, _v, "#");
    message(2040, _x+4*24, 0, _v, "Young Sigmund has a");
    message(2056, _x+5*24, 0, _v, "Few problems. Can");
    message(2056, _x+6*24, 0, _v, "you help him find");
    message(2028, _x+7*24, 0, _v, "the rogue brain cells");
    message(2036, _x+8*24, 0, _v, "in his mind and shut");
    message(2100, _x+9*24, 0, _v, "Them down?");
    message(2096, _x+10*24, 0, _v, "PRESS SPACE");
    message(2088, _x+12*24, 0, _v, "Cheat Mode!!!");
    message(2028, _x+14*24, 0, _v, "F1, F2 Get Weapons");
    message(2028, _x+15*24, 0, _v, "F3 - Restore Strength");
    message(2096, _x+17*24, 0, _v, "HAVE FUN !!!");


    showtext();
    for (int scroll = 256+8; swi_readescapestate() == 0;)
    {
       loope0:
        swi_blitz_wait(2);
        if (scroll != 0)
        {
            scroll--;
            switchbank();
            swi_fastspr_clearwindow();
            texthandler(1);
        }
        else if (need_redraw())
        {
            showchatscreen();
            showtext();
        }
       preludetextstop:;
        int r1 = osbyte_7a();
        if ((r1 != -1) && (r1 != 307) && (r1 != 308)) // escape
           endprelude:
            return cheatpermit;
        if (readmousestate()&2)
        {
           gocheat:
            if (osbyte_81(-SDLK_LALT) != 0xff) return cheatpermit;
            if (osbyte_81(-SDLK_RALT) != 0xff && osbyte_81(-SDLK_MODE) != 0xff) return cheatpermit;
            cheatpermit = 1;
            scroll = 1024;
        }
    }
    osbyte_7c();
    return 2;
}


void fatalfile()
{
    exit(printf("A file vital to the game cannot be loaded.  Please reset your machine and try   again"));
}

void showloading()
{
    wipetexttab();
    showchatscreen();
    message(88, 64, 0, 0, "Please Wait");
    message(48, 128, 0, 0, "Loading Game files");
    showtext();
}


void filenotthere()
{
    showerror();
    message(56, 64, 0, 0, "A file is missing.");
    message(48, 96, 0, 0, "It cannot be loaded");
    showtext();
    errorwait();
}


void filesyserror()
{
    showerrorok();
    message(40, 64, 0, 0, "Unable to Load File");
    message(56, 96, 0, 0, "Please Check Disc");
    showtext();
    errorwait();
}

void badload()
{
    showerrorok();
    message(32, 32, 0, 0, "A game file could not");
    message(48, 64, 0, 0, "be loaded. Program");
    message(80, 96, 0, 0, "Must exit now.");
    showtext();
    errorwait();
    abort_game();
}

int badlevelload()
{
    showerrorok();
    message(40, 32, 0, 0, "The level cannot be");
    message(32, 64, 0, 0, "Loaded. Please check");
    message(48, 96, 0, 0, "the disc, or press");
    message(48, 128, 0, 0, "escape to end game.");
    showtext();
    return errorwait(); // 'clear carry if carry clear (???)'
}

void nomemory()
{
    showerrorok();
    message(48, 32, 0, 0, "There is not enough");
    message(48, 64, 0, 0, "memory available to");
    message(48, 96, 0, 0, "load the game files.");
    showtext();
    errorwait();
}

void showerror()
{
    //frameinc = 1;
    showchatscreen();
    swi_fastspr_clearwindow();
    wipetexttab();
    message(72, 200, 0, 0, "RET - Try Again");
    message(72, 224, 0, 0, "ESC - Abandon");
}

void showerrorok()
{
    //frameinc = 1;
    showchatscreen();
    swi_fastspr_clearwindow();
    wipetexttab();
    message(72, 200, 0, 0, "RET - OK");
    message(72, 224, 0, 0, "ESC - Abandon");
}

int errorwait()
{
    if (osbyte_81(-74) != 0xff)
       loopb9:
        while (osbyte_81(-61) != 0xff)
            if (swi_readescapestate())
            {
                wipetexttab(); return 0;
            }
   waitover:
    wipetexttab();
    return 1;
}

const char defscore[] = "00000000 PSY\n";

void setdefaultscores()
{
    char* r10 = highscorearea;

    for (int r3 = 5; r3 > 0; r3--)
    {
       loopd4:;
        const char* r11 = defscore;
        for (int r2 = 13; r2 > 0; r2--)
           loopd5: *(r10++) = *(r11++);
    }
}

int showhighscore()
{
    loadscores(highscorearea, options.mentalzone);
    updatehst();
    showhst();
    message(96, 224, 0, 0, "press fire");
    releaseclip();
    showtext();
    readopt(0);
    return swi_readescapestate();
}

void updatehst()
{
    hstindex = 5;
    char* r10 = highscorearea;
    r10 += 4*13; //4*entry length
    while (comparescore(r10))
    {
       loopd1:
        r10 -= 13; //entry length
        if (--hstindex == 0) break;
    }
   lessthan:
    r10 += 13; //entry length
    if (hstindex != 5)
    {
        if (4-hstindex > 0)
        {
            char* r9;
            int r3;
            for (r9 = highscorearea+4*13-1, r3 = 13*(4-hstindex); r3 > 0; r9--, r3--)
               loopd7:
                r9[13] = *r9;
        }
       noshiftscore:;

        char* r11 = plscore;
        for (int r3 = 8; r3 > 0; r3--)
           loopda:
            *(r10++) = *(r11++)+'0';

        r10++;
//int r9=1024;
	key_state ks;
        swi_blitz_wait(20); //
        for (int i = 0; i < 3; i++) r10[i] = options.initials[i];
        for (int r8 = 3; r8 > 0; r8--, r10++)
        {
            while (osbyte_81(0) != -options.firekey)
            {
               scorekeyloop:
//if (--r9<0) goto scoreexit;
                keyread(&ks);
                if (ks.leftpress == 0)
                {
                    (*r10)++;
                }
                if (ks.rightpress == 0)
                {
                    (*r10)--;
                }
                if (*r10 < '0') *r10 = '0';
                if (*r10 > 'Z'+4) *r10 = 'Z'+4;
                showhst();
                swi_blitz_wait(4);
            }
            options.initials[3-r8] = *r10;
            swi_stasis_link(1, 18);
            swi_stasis_volslide(1, 0, 0);
            swi_sound_control(1, 0x17c, 140, 0);
            /*if (r8>1)*/ swi_blitz_wait(20);
        }
       scoreexit:
        savescores(highscorearea, options.mentalzone);
        dosaveconf();
    }
   notontable:;
}


int comparescore(char* r10)
{
    char* r11 = plscore;

    for (int r3 = 8; r3 > 0; r3--)
    {
       loopd0:
        if (*r10-'0' < *r11) scoregreater: return 1;
        else if (*r10-'0' > *r11) scoreless: return 0;
        r10++; r11++;
       compnext:;
    }
    return 1;
}

void showhst()
{
    swi_blitz_wait(0);
    switchbank();
    showchatscores();
    wipetexttab();
    message(64, 32, 0, 0, "Zone High Scores");

    int x = 32, y = 64, i, j;
    char * ptr = highscorearea;
    for (i = 0; i < 5; ++i)
    {
        char line[13];
        for (j = 0; j < 12 && *ptr > 0xa; ++j, ++ptr)
            line[j] = *ptr;
        line[j] = 0;
        message(x, y, 0, 0, line);
        y += 32;
        ++ptr; /*skip over newline*/
    }
    if (hstindex < 5)
        message(280, (hstindex+2)<<5, 0, 0, "=");
    texthandler(0);
}
