/*  keyboard.c */

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

#include "asylum.h"

extern asylum_options options;

static char keyboard[512];
static int keybuf;
static int unibuf;
static int mouse;
static int exposed;

#define ESC_VALUE 27


void init_keyboard()
{
    for (int i = 0; i < 512; i++) keyboard[i] = 0;
}

void swi_removecursors()
{
    ;
}
int swi_readescapestate()
{
    return keyboard[ESC_VALUE];
}
int readmousestate()
{
    return mouse;
}

int osbyte_79(int c)
{
    update_keyboard();
    int key = keybuf;
    keybuf = -1;
    return key;
    //for (int i=0;i<512;i++) if (keyboard[i]) {/*printf("Returning %i\n",i);*/ return i;}
    return -1;
}

int osbyte_79_unicode(int c)
{
    update_keyboard();
    int uni = unibuf;
    unibuf = -1;
    return uni;
}

int osbyte_7a()
{
    update_keyboard(); for (int i = 0; i < 512; i++) if (keyboard[i]) return i;return -1;
}
void osbyte_7c()
{
    keyboard[ESC_VALUE] = 0;
}

int osbyte_81(int c)
{
    if (c >= 0) return osbyte_79(c);
    update_keyboard();
    return keyboard[-c];
}

char swi_oscrc(int w, char* start, char* end, int bytes)
{
    return 0xff;
}


int swi_joystick_read(int a, int* x, int* y)
{
    ;
}

void update_keyboard()
{
    SDL_Event e;
    SDL_KeyboardEvent* ke;
    SDL_MouseButtonEvent* me;

    while (SDL_PollEvent(&e))
    {
        // why not SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_KEYDOWNMASK|SDL_KEYUPMASK)>0)?
        switch (e.type)
        {
        case SDL_KEYDOWN:
            ke = (SDL_KeyboardEvent*)&e;
            keyboard[ke->keysym.sym] = 0xff;
            keybuf = ke->keysym.sym;
            if (ke->keysym.unicode)
                unibuf = ke->keysym.unicode;
            else
                unibuf = -1;
            break;
        case SDL_KEYUP:
            ke = (SDL_KeyboardEvent*)&e;
            keyboard[ke->keysym.sym] = 0;
            break;
        case SDL_MOUSEBUTTONDOWN:
            me = (SDL_MouseButtonEvent*)&e;
            switch (me->button)
            {
            case SDL_BUTTON_LEFT: mouse |= 4; break;
            case SDL_BUTTON_MIDDLE: mouse |= 2; break;
            case SDL_BUTTON_RIGHT: mouse |= 1; break;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            me = (SDL_MouseButtonEvent*)&e;
            switch (me->button)
            {
            case SDL_BUTTON_LEFT: mouse &= ~4; break;
            case SDL_BUTTON_MIDDLE: mouse &= ~2; break;
            case SDL_BUTTON_RIGHT: mouse &= ~1; break;
            }
            break;
        case SDL_VIDEOEXPOSE:
            exposed = 1;
            break;
        case SDL_QUIT:
            exithandler();
            break;
        }
    }
}

void zonecheatread(int* zone)
{
    char r1 = osbyte_79_unicode(0); // was _81(0)

    if ((r1 < 48) || (r1 > 56)) return;
    *zone = r1-48;
}

void cheatread()
{
    if (osbyte_81(-282) == 0xff) getmpmg();
    if (osbyte_81(-283) == 0xff) getrocket();
    if (osbyte_81(-285) == 0xff) screensave();
    if (osbyte_81(-284) == 0xff) prepstrength();
}


void keyread(key_state* ks)
{
    int r4 = -1;

    if (options.joyno)
    {
        int r0, r1;
        int v = swi_joystick_read(options.joyno-1, &r0, &r1);
        if (v)
        {
           nostickerr:
            message(32, 32, 0, 1, "Can't see a joystick!");
            r4 = -1;
            options.joyno = 0;
        }
        else
            r4 = -1;
/*
   MOV R1,R0,ASL #24
   CMN R1,#32<<24
   BICLT R4,R4,#8; down
   CMP R1,#32<<24
   BICGT R4,R4,#4; up
   BIC R0,R0,#&FF
   MOV R1,R0,ASL #16
   CMN R1,#32<<24
   BICLT R4,R4,#1; left
   CMP R1,#32<<24
   BICGT R4,R4,#2; right

   TST R0,#1<<16
   BICNE R4,R4,#16; fire
   TST R0,#1<<17
   BICNE R4,R4,#4; up on fire button 2
 */
    }
   nojoystick:
    if ((osbyte_81(options.leftkey) == 0xff) || !(r4&1)) 
      { if (++ks->leftpress == 0) ks->leftpress = 0xff;}
    else ks->leftpress = 0;
    if ((osbyte_81(options.rightkey) == 0xff) || !(r4&2))
      { if (++ks->rightpress == 0) ks->rightpress = 0xff;}
    else ks->rightpress = 0;
    if ((osbyte_81(options.upkey) == 0xff) || !(r4&4))
      { if (++ks->uppress == 0) ks->uppress = 0xff; }
    else ks->uppress = 0;
    if ((osbyte_81(options.downkey) == 0xff) || !(r4&8))
      { if (++ks->downpress == 0) ks->downpress = 0xff; }
    else ks->downpress = 0;
    if ((osbyte_81(options.firekey) == 0xff) || !(r4&16))
      { if (++ks->fire == 0) ks->fire = 0xff; }
    else ks->fire = 0;
    if (ks->leftpress || ks->rightpress
	  || ks->uppress || ks->downpress || ks->fire)
        ks->keypressed = 1;
}

int need_redraw()
{
    int e = exposed;
    exposed = 0;
    return e;
}
