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
static int mouse;


void init_keyboard()
{
    for (int i = 0; i < 512; i++) keyboard[i] = 0;
}

/*
void swi_removecursors()
{
    ;
}
*/

int swi_readescapestate()
{
    return keyboard[SDL_SCANCODE_ESCAPE];
}
int readmousestate()
{
    return mouse;
}

int osbyte_79()
{
    update_keyboard();
    int key = keybuf;
    keybuf = -1;
    return key;
    //for (int i=0;i<512;i++) if (keyboard[i]) {/*printf("Returning %i\n",i);*/ return i;}
    return -1;
}

int osbyte_7a()
{
    update_keyboard(); for (int i = 0; i < 512; i++) if (keyboard[i]) return i;return -1;
}
void osbyte_7c()
{
    keyboard[SDL_SCANCODE_ESCAPE] = 0;
}

int osbyte_81(int c)
{
    if (c >= 0) return osbyte_79();
    update_keyboard();
    return keyboard[-c];
}

char swi_oscrc()
{
    return 0xff;
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

            if (ke->keysym.scancode <= 512) {
                keyboard[ke->keysym.scancode] = 0xff;
                keybuf = ke->keysym.scancode;
            }

            break;
        case SDL_KEYUP:
            ke = (SDL_KeyboardEvent*)&e;

            if (ke->keysym.scancode <= 512)
                keyboard[ke->keysym.scancode] = 0;

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
        case SDL_QUIT:
            exithandler();
            break;
        }
    }
}

void cheatread()
{
    int key = osbyte_79();

    if (key == -1)
        return;

    switch (key) {
        case SDL_SCANCODE_F1:
            getmpmg();
            break;
        case SDL_SCANCODE_F2:
            getrocket();
            break;
        case SDL_SCANCODE_F3:
            prepstrength();
            break;
        case SDL_SCANCODE_F4:
            maxlives(); // Originally screensave();
            break;
        default:
            if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_9)
            {
                change_zone(key - SDL_SCANCODE_1);
            }
            break;
    }
}


void keyread(key_state* ks)
{
    int r4 = -1;

   //nojoystick:
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
    return 0;
}
