/*  bullet.c */

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

#include <math.h>
#include "asylum.h"

#define _bulno 64
#define _bullim 0x1000

bulent bulofs[_bullim];

bulent* buladr;
int bulctr;

extern char _targetlowlim, _targethighlim, _translowlim, _transhighlim;

extern int xposmax, yposmax;
extern fastspr_sprite exploadr[];
extern char masterplotal;

extern int xpos, ypos, pllx, plly, plhx, plhy;
extern int framectr;

Mix_Chunk* CHUNK_BULLET_1;
Mix_Chunk* CHUNK_BULLET_2;
Mix_Chunk* CHUNK_BULLET_3;

#define _bulspritebase 16

int splittab[14];

void init_splittab()
{
    for (int i = 0; i < 7; i++)
    {
        splittab[i*2] = (int)(0x200*sin((0.5+i)/3.5*M_PI));
        splittab[i*2+1] = (int)(0x200*cos((0.5+i)/3.5*M_PI));
    }
}

void init_chunk_bullet()
{
    CHUNK_BULLET_1 = make_sound(_Sampsmallzap, fullpitch+0x1000, 0, (fullpitch<<16)|0xfe00, 2);
    CHUNK_BULLET_2 = make_sound(_Sampbigzap, fullpitch+0x1800, 0, (fullpitch<<16)|0xfe00, 2);
    CHUNK_BULLET_3 = make_sound(_Sampbigzap, fullpitch+0x1000, 0, (fullpitch<<16)|0xfe00, 2);
}

void bullets()  // the bullet handler (aliens fire these)
{

    bulent* r11 = buladr;
    int i;

    for (i = _bulno; i > 0; i--, r11++)
    {
       loop71:
        if (r11->type == 0) continue;
       foundbul:

        r11->x += r11->dx;  r11->y += r11->dy;

        if ((r11->x <= xlowlim) || (xposmax <= r11->x) || (r11->y <= ylowlim) || (yposmax <= r11->y))
        {
            // goto projoffscr
            r11->type = 0;
            continue;
        }

        if ((r11->dx <= (1<<12)) && (-r11->dx <= (1<<12)))
            if (r11->flags&BULL_ACCEL)
            {
                r11->dx += (r11->dx>>4);
                r11->dy += (r11->dy>>4);
            }
       bulnoacc:


        if ((r11->flags&BULL_HOME) && (r11->flags <= (3<<6)*BULL_TTL))
        {
            int r6 = 1<<6;
            if (!(r11->flags&BULL_SLOWHOME))
            {
                r6 = 1<<4;
                r11->dx -= (r11->dx>>5);
                r11->dy -= (r11->dy>>5);
            }
            r11->dx += (r11->x < xpos) ? r6 : -r6;
            r11->dy += (r11->y < ypos) ? r6 : -r6;
        }
       bulnohome:

        if (r11->dx > _speedlim) r11->dx = _speedlim;
        if (r11->dx < -_speedlim) r11->dx = -_speedlim;
        if (r11->dy > _speedlim) r11->dy = _speedlim;
        if (r11->dy < -_speedlim) r11->dy = -_speedlim;

        r11->flags -= BULL_TTL; /* decrement the life counter */

        if (r11->flags < 0)
        {
// out of time
// goto buldestroy
            if ((r11->flags)&BULL_SPLIT) goto bulsplit;
            r11->type = 0;
            continue;
        }

        if (!((r11->x <= pllx) || (plhx <= r11->x) || (r11->y <= plly) || (plhy <= r11->y)))
        {
            // goto bulletkill;
           bulletkill:
            r11->type = 0;
            // goto causeexplobullet;
            int DX = r11->dx>>4, DY = r11->dy>>4;
            explogonopyroquiet(r11->x-r11->dx, r11->y-(8<<8)-r11->dy, DX+DY, DY-DX, 0, DX+DY, 0);
            explogonopyro(r11->x-r11->dx, r11->y-(8<<8)-r11->dy, DX-DY, DX+DY, 0, DX-DY, 0);
            continue;
        }
        {
            char r1 = *fntranslate(r11->x, r11->y);
            if ((r1 >= 16) // hit a block
                //     bulhit:
                && !((r1 >= _translowlim) && (r1 <= _transhighlim))
                && !((r1 >= _targetlowlim) && (r1 <= _targethighlim)))
                ;
            else
            {
               bulhitins:

                if (masterplotal == 0) continue;

                int r0 = (r11->type);
                if ((r0 == _bulspritebase+8) || (r0 == _bulspritebase+10))
                    r0 ^= (framectr&4)>>2;
                relplot(exploadr, r0, r11->x, r11->y);
                continue;
            }
        }
       nobultarget:
        r11->type = 0;
       bulnodestroy:
        if ((r11->flags&BULL_EXPLO) != 0)
        {
// causeexplobullet:
            int DX = r11->dx>>4, DY = r11->dy>>4;
            explogonopyroquiet(r11->x-r11->dx, r11->y-(8<<8)-r11->dy, DX+DY, DY-DX, DX+DY, 0, 0);
            explogonopyro(r11->x-r11->dx, r11->y-(8<<8)-r11->dy, DX-DY, DX+DY, DX-DY, 0, 0);
            continue;
        }
       buldestroy:
        if ((r11->flags&BULL_SPLIT) == 0)
        {
            r11->type = 0;
            continue;
        }
       bulsplit:;
        int newtype = (random()&7);
        if (newtype == 7) newtype = 0;
        int r7 = r11->type;
        if (r7 == _bulspritebase+13) newtype = 7;
        if (r7 == _bulspritebase+14) newtype = 10;
        if (r7 == _bulspritebase+15) newtype = 12+(newtype&3);
        if (newtype == 15) newtype = 0;

        for (int i = 0; i < 14; i += 2)
           loop75:
            (void)makebul(r11->x, r11->y, (r11->dx>>2)+splittab[i], (r11->dy>>2)+splittab[i+1], newtype, (1<<6)*BULL_TTL);
        r11->type = 0;
    }
}

int makebul(int x, int y, int dx, int dy, int type, int flags)
{

    int i;
    bulent* r10 = buladr;

    type += _bulspritebase;
    r10 += bulctr;
    for (i = bulctr; i < _bulno; i++, r10++)
       loop72:
        if (r10->type == 0)
        {
            bulctr = i; break;
        }
    if (i == _bulno)
    {
        r10 = buladr;
        for (i = 0; i < bulctr; i++, r10++)
           loop69:
            if (r10->type == 0)
            {
                bulctr = i; break;
            }
        if (i == bulctr) return 1; // failed
    }
   foundmakebul:

    if (flags < BULL_TTL) flags |= (1<<6)*BULL_TTL;
    if (type >= _bulspritebase+8) flags |= BULL_SLOWHOME|BULL_EXPLO;
    if (type >= _bulspritebase+12) flags = ((1<<5)*BULL_TTL)|BULL_SPLIT;
    if ((type == _bulspritebase+7) || (type == _bulspritebase+8)) flags |= BULL_HOME;
    if (type == _bulspritebase+10) flags |= BULL_ACCEL;

    r10->type = type;
    r10->x = x;
    r10->y = y;
    r10->dx = dx;
    r10->dy = dy;
    r10->flags = flags;

    bulctr = (bulctr+1)%_bulno; // loop68:

// if (bulctr==0)  return; // XXX why???

    int r7 = (r10->x-xpos)>>9;
    x = abs(r10->x-xpos);
    y = abs(r10->y-ypos);
    int vol = (0x7f-((x+y)>>12));
    if (vol >= 80)
    {
	bidforsound(_Explochannel,(type>=_bulspritebase+8)?_Sampbigzap:_Sampsmallzap,
                    (((vol&0x7f)>0x7c)?0x7c:(vol&0x7f))-((type==_bulspritebase+8)?0x10:0),
                    (type==_bulspritebase+8)?(fullpitch+0x1800):(fullpitch+0x1000),
                    0,(fullpitch<<16)|(0xfe<<8),2,r7,   // lifetime (frames)
                    (type>_bulspritebase+8)?CHUNK_BULLET_3:(type==_bulspritebase+8)?CHUNK_BULLET_2:CHUNK_BULLET_1);

    }
   nozapsound:;
    return 0;
}

void initbultab()
{
    buladr = bulofs;
    bulent* r10 = buladr;
    bulctr = 0;
    for (int r9 = _bulno; r9 > 0; r9--)
    {
       loop74:
        *(int*)r10 = 0;
        r10++;
    }
}
