/*  alien.c */

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
#include <SDL/SDL_mixer.h>

#define _firerange (128<<8)
#define _sleeprange (512<<8)

#define _explocontloss (1<<7)
#define _alno 0x200

#define _bulspritebase 16

#define _expwidth (24<<8)
#define _expheight (24<<8)
#define _alheight (32<<8)
#define _alwidth (16<<8)

extern fastspr_sprite blokeadr[77], blockadr[256], alspradr[256], exploadr[32];
extern asylum_options options;

const char _platblim = 64;
const int _xofs = 159, _yofs = 96;
const char _platno = 176;
extern int boardwidth;
extern int framectr;
extern int xpos, ypos, xposmax, yposmax;
extern char masterplotal;
extern const char _blim;
extern const char _extendno;
extern const char _bonuslow, _bonushigh;
extern const char _platlowlim = 176, _plathighlim = 190, _alplathighlim = 177;
const int xblocks_disturb = 18, yblocks_disturb = 12;
char plotal;
//char lefthit, righthit, uphit, downhit;         // word-aligned
char alonobj, alonplat, colchplace;
int sprlx; int sprly; int sprhx; int sprhy;
int xtemp, ytemp;
colchent* colchtab;
colchent* colchptr;
colchent* oldcolchtab;
colchent* oldcolchptr;
colchent* colchtabuse;
colchent* colchptruse;
alent* dodgypointer;
bulcolchent* bulcolchptr;
//colchofs=&5000 // two tables
#define _colchlim 0x400
//bulcolchofs=&7000
#define _bulcolchlim 0x400
colchent colchofs[2*_colchlim];
bulcolchent bulcolchtab[_bulcolchlim];

//alofs=&F00

alent aladr[_alno];
int alctr;

int* platsandstr;
#define _savearealen (0x9c0/32)

Mix_Chunk* CHUNK_EXPLO;
Mix_Chunk* CHUNK_ATOM;
Mix_Chunk* CHUNK_SPINFIRE;
Mix_Chunk* CHUNK_SPINPOWERFIRE;
Mix_Chunk* CHUNK_SHOOT;
Mix_Chunk* CHUNK_SHOOTNUTTER;

void moval()
{
//BL writeclip - active for release version
    sprlx = xpos-(((xblocks_disturb+2)*8)<<8);
    sprhx = xpos+(((xblocks_disturb+2)*8)<<8);
    sprly = ypos-(((yblocks_disturb+2)*8)<<8);
    sprhy = ypos+(((yblocks_disturb+2)*8)<<8);

    alent* r11 = aladr;
    for (int r9 = _alno; r9 > 0; r9 -= 8)
    {
       l8:
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
        if (r11->type) procal(r11);r11++;
    }
    return;
}

void procal(alent* r11)
{
    if ((r11->x <= xlowlim) || (xposmax <= r11->x) || (r11->y <= ylowlim) || (yposmax <= r11->y))
    {
       aloffscr:
        if (r11->type != _Decoration)
        {
            r11->type = 0;
            return;
        }
    }
   noaloffscr:
    if ((r11->x >= sprlx) && (sprhx >= r11->x) && (r11->y >= sprly) && (sprhy >= r11->y))
        plotal = masterplotal;
    else plotal = 0;
    switch ((r11->type)&0x1f)
    {
       aljumptab:
       t:
    case 1: explo(r11); return;
    case 2: ember(r11); return;
    case 3: plat1(r11); return;
    case 4: plat2(r11); return;
    case 5:
    case 6: plat3(r11); return;
    case 7:
    case 8:
    case 9:
    case 10: plat4(r11); return;
    case 11: plat5(r11); return;
    case 12: scoreobj(r11); return;
    case 13: dyingbonus(r11); return;
    case 14: flyingbonus(r11); return;
    case 15: booby(r11); return;
    case 16: decoration(r11); return;
    case 17: extender(r11); return;
    case 18: alien1(r11); return;
    case 19: alien2(r11); return;
    case 20: alien3(r11); return;
    case 21: alien4(r11); return;
    case 22: alien5(r11); return;
    case 23: alien6(r11); return;
    case 24: alien7(r11); return;
    case 25: alien8(r11); return;
    case 26: alien9(r11); return;
    case 27: alien10(r11); return;
    case 28: alien11(r11); return;
    case 29: alien12(r11); return;
    case 30: alien13(r11); return;
    case 31: alien14(r11); return;
    case 0:;
    }
   aljerr:
    return;
}

void alienwander(alent* r11, char* r5)
{
    if ((r11->righthit == 1) || (r11->lefthit == 1)) almightjump(r11);
    else if (*r5 == 0) alpossjump(r11);
    else if (*(r5+1) == 0) alpossjump(r11);
    else if ((random()&0x3f) == 0) almightjump(r11);
    else almightjumpins(r11);
}

void almightjumpins(alent* r11)
{
    if (((r11->dx) > (1<<5)) || ((r11->dx) < -(1<<5))
        || ((r11->dy) > (1<<7)) || ((r11->dy) < -(1<<7))) return;
    alienstopped(r11);
}

void jumpyalwander(alent* r11)
{
    if ((r11->righthit == 1) || (r11->lefthit == 1)) almightjump(r11);
    else if (((r11->dx) > (1<<5)) || ((r11->dx) < -(1<<5))
             || ((r11->dy) > (1<<7)) || ((r11->dy) < -(1<<7))) return;
    else alienstopped(r11);
}

void alienwanderfly(alent* r11)
{
    if ((random()&0xff) == 0) alienstoppedfly(r11);
    if (((r11->dx) > (1<<5)) || ((r11->dx) < -(1<<5))
        || ((r11->dy) > (1<<5)) || ((r11->dy) < -(1<<5))) return;
    alienstoppedfly(r11);
}

void alienwandernojump(alent* r11)
{
    if (((r11->dx) > (1<<5)) || ((r11->dx) < -(1<<5))
        || ((r11->dy) > (1<<8)) || ((r11->dy) < -(1<<8))) return;
    alienstopped(r11);
}

void alienstopped(alent* r11)
{
    if ((r11->downhit == 1) || (alonobj == 1))
        r11->dx = (random()&(1<<9))-(1<<8);
}

void alienstoppedfly(alent* r11)
{
    int r2 = random();

    if ((r2&6) == 0)
    {
       flyup:
        r11->dx = 0;
        r11->dy = -(1<<7);
    }
    else if ((r2&1) == 0)
    {
       flyupdown:
        r11->dx = 0;
        r11->dy = (r2&(1<<9))-(1<<8);
    }
    else
    {
        r11->dx = (r2&(1<<10))-(1<<9);
        r11->dy = 0;
    }
    return;
}

void almightjump(alent* r11)
{
    if (r11->downhit != 1) r11->dx = 0;
    else if ((random()&3) == 0) r11->dy = -(8<<8);
    almightjumpins(r11);
}

void alpossjump(alent* r11)
{
    if (r11->downhit != 1) r11->dx = 0;
    else if ((random()&0x1f) == 0) r11->dy = -(8<<8);
    almightjumpins(r11);
}

void almightwelljump(alent* r11)
{
    if (r11->downhit != 1) r11->dx = 0;
    else if ((random()&3) != 0) r11->dy = -(8<<8);
    almightjumpins(r11);
}

void alientestplat(alent* r11, char* r5)
{
    if (!alonplat) return;
    plattoobj(r5);
}

void decoration(alent* r11)
{
    if (--r11->r6 < 0)
    {
        r11->x += r11->dx; r11->y += r11->dy; r11->dy += (1<<4);
    }
    if ((r11->r5 -= (1<<16)) < 0)
        r11->type = 0;
    if (masterplotal == 0) return;
    mazescaleplot(blokeadr, r11->r5&0xff, _xofs+(r11->x>>8), _yofs+8+(r11->y>>8));
}

void extender(alent* r11)
{
    int tmpx = r11->x+r11->dx, tmpy = r11->y+r11->dy;

    if (!plcolcheck(r11->x+r11->dx, r11->y+r11->dy, (32<<8), (8<<8))) ; // horiz.size, vert. size
    r11->x += r11->dx, r11->y += r11->dy;

    char* r0 = fntranslate(tmpx, tmpy);
    char f = r11->r5&0xff;
    if (r11->r5&(1<<8))
    {
       contracting:
        if ((*r0 == f) || (*r0 == 0)) f = *r0 = 0;
        else if ((*r0 != f+1) && (*r0 != f+2) && !block_gas(*r0))
        {
            r11->type = 0;
            return;
        }
    }
    else
    {
        if (*r0 == 0) *r0 = f;
        if ((*r0 != f) && (*r0 != f+1) && (*r0 != f+2) && !block_gas(*r0))
        {
            r11->dx = -r11->dx;
            r11->r5 |= (1<<8);
        }
    }

   nostopextend:
    if (plotal == 0) return;
    r0 = fntranslate(r11->x+r11->dx*15, r11->y);
    f = r11->r5&0xff;
    if ((*r0 != 0) && (*r0 != f) && (*r0 != f+1) && (*r0 != f+2)) return;
    relplot(blockadr, r11->r5&0xff, r11->x+r11->dx*7, r11->y);
}

int alspintab[40];

void init_alspintab()
{
    for (int i = 0; i < 20; i++)
    {
        alspintab[i*2] = (int)((10<<8)*sin(i*2*M_PI/16));
        alspintab[i*2+1] = (int)((10<<8)*cos(i*2*M_PI/16));
    }
}

void alien1(alent* r11)
{
    colcheck(r11, 2, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwandernojump(r11);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight/2))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(1, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(1, r11);

    if (plotal == 0) return;

    relplot(alspradr, 8+(r11->dx > 0)*2+((framectr&8)>>3),
            r11->x, r11->y+(8<<8));
}

void alien2(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(2, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(2, r11);

    if (plotal == 0) return;

    relplot(alspradr, 0, r11->x, r11->y);
}


void alien3(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(3, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(3, r11);
    if ((random()&0x3f) == 0) alshoot(r11);

    if (plotal == 0) return;

    relplot(alspradr, 1, r11->x, r11->y);
}

void alien4(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    jumpyalwander(r11);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(4, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(4, r11);
    r11->r5 += (r11->r5<<16);
    if (r11->r5&(1<<29)) r11->r5 = alspinfire(r11);
    if (r11->r5&(1<<31)) r11->r5 -= 8;else r11->r5 += 4;
    if ((r11->r5&0xffff) == 0) r11->r5 = 0;

    if (plotal == 0) return;

    int x, y;
    relplot(alspradr, 2, r11->x, r11->y);
    relplot(alspradr, 4+((r11->r5>>24)&3), r11->x, r11->y-(7<<8));
    if (r11->r5&(1<<31)) return;
    int f = (r11->r5>>26)&3;
    int r0 = (r11->r5>>24)&3;
    int* r3 = alspintab+r0*2;
    for (int r4 = 4; r4 > 0; r4--)
    {
       loop81:
        relplot(exploadr, _bulspritebase+((r4+f)&3), r11->x+r3[0], r11->y-(7<<8)+r3[1]);
        r3 += 8; // yes, that's 8 ints (32 bytes)
    }
}


void alien5(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(5, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(5, r11);
    r11->r5 += (r11->r5<<16);
    if (r11->r5&(1<<29)) r11->r5 = alspinpowerfire(r11, 0);
    if (r11->r5&(1<<31)) r11->r5 -= 2;else r11->r5 += 2;
    if ((r11->r5&0xffff) == 0) r11->r5 = 0;

    if (plotal == 0) return;

    int x, y;
    relplot(alspradr, 2, r11->x, r11->y);
    relplot(alspradr, 4+((r11->r5>>24)&3), r11->x, r11->y-(7<<8));
    if (r11->r5&(1<<31)) return;
    int f = (r11->r5>>26)&3;
    int r0 = (r11->r5>>24)&3;
    int* r3 = alspintab+r0*2;
    for (int r4 = 4; r4 > 0; r4--)
    {
       loop83:
        relplot(exploadr, _bulspritebase+8+((r4+f)&1), r11->x+r3[0], r11->y-(7<<8)+r3[1]);
        r3 += 8; // yes, that's 8 ints (32 bytes)
    }
}

void alien6(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(6, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(6, r11);
    r11->r5 += (r11->r5<<16);
    if (r11->r5&(1<<29)) r11->r5 = alspinpowerfire(r11, 1);
    if (r11->r5&(1<<31)) r11->r5 -= 2;else r11->r5 += 8;
    if ((r11->r5&0xffff) == 0) r11->r5 = 0;

    if (plotal == 0) return;

    int x, y;
    relplot(alspradr, 3, r11->x, r11->y);
    relplot(alspradr, 4+((r11->r5>>24)&3), r11->x, r11->y-(7<<8));
    if (r11->r5&(1<<31)) return;
    int f = (r11->r5>>26)&3;
    int r0 = (r11->r5>>24)&3;
    int* r3 = alspintab+r0*2;
    for (int r4 = 4; r4 > 0; r4--)
    {
       loop85:
        relplot(exploadr, _bulspritebase+10+((r4+f)&1), r11->x+r3[0], r11->y-(7<<8)+r3[1]);
        r3 += 8; // yes, that's 8 ints (32 bytes)
    }
}

int alspinfire(alent* r11)
{
    int r0 = (random()&3);
    int* r6 = alspintab+(r0<<1);

    for (int r4 = 4; r4 > 0; r4--)
    {
       loop82:
        makebul(r6[0]+r11->x, r6[1]+r11->y-(7<<8), r6[8]>>2, r6[9]>>2,
                ((r4+r0)&3), (1<<8)*BULL_TTL);
        r6 += 8;
    }
    r6 -= 8;
    int r2 = 0x7f-abs(r6[0]-xpos)-abs(r6[1]-(7<<8)-ypos); // XXX scaling?
    if (r2 > 0x40)
        bidforsound(_Explochannel, _Sampsmallzap, r2, (fullpitch+0x1000) /*pitch*/,
                    0, 0, 2 /* lifetime (frames) */, (r6[1]-(7<<8))>>9, CHUNK_SPINFIRE);
   nospinzapsound:
    return 0x80000100;
}

int alspinpowerfire(alent* r11, int r7)
{
    int r0 = (random()&3);
    int* r6 = alspintab+(r0<<1);

    for (int r4 = 4; r4 > 0; r4--)
    {
       loop84:
        makebul(r6[0]+r11->x, r6[1]+r11->y-(7<<8), r6[8]>>2, r6[9]>>2,
                8+(r7<<1), (1<<8)*BULL_TTL);
        r6 += 8;
    }
    r6 -= 8;
    int r2 = 0x7f-abs(r6[0]-xpos)-abs(r6[1]-(7<<8)-ypos); // XXX scaling?
    if (r2 > 0x40)
        bidforsound(_Explochannel, _Sampbigzap, r2, fullpitch /*pitch*/,
                    0, 0, 2 /* lifetime (frames) */, (r6[1]-(7<<8))>>9, CHUNK_SPINPOWERFIRE);
   nopowerzapsound:
    return 0x80000040;
}

void alien7(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(7, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(7, r11);
    int rng = random();
    if ((rng&0x3f) == 0) alshootnutter(r11);
    else if ((rng&0xf) == 0) alshootfast(r11);

    if (plotal == 0) return;

    relplot(alspradr, 16, r11->x, r11->y);
}

void alien8(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(8, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(8, r11);
    int rng = random();
    if ((rng&0x3f) == 0) alshootnutter(r11);
    else if ((rng&0xf) == 0) alshootmental(r11);

    if (plotal == 0) return;

    relplot(alspradr, 16, r11->x, r11->y);
}

void alien9(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(9, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(9, r11);
    int rng = random();
    if ((rng&0x1f) == 0) alshootnutterplus(r11);

    if (plotal == 0) return;

    relplot(alspradr, 17, r11->x, r11->y);
}

void alien10(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy += 1<<7; // gravity
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwander(r11, r5);
    alientestplat(r11, r5);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(10, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(10, r11);
    int rng = random();
    if ((rng&0x1f) == 0) alshootnuttermental(r11);

    if (plotal == 0) return;

    relplot(alspradr, 17, r11->x, r11->y);
}

void alien11(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy; // no gravity

    alienwanderfly(r11);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > 2*_sleeprange) alsleep(11, r11);
    if (abs(r11->y-ypos) > 2*_sleeprange) alsleep(11, r11);
    int rng = random();
    if ((rng&0x1f) == 0) alshootfast(r11);

    if (plotal == 0) return;

    relplot(alspradr, 12+(random()&3), r11->x, r11->y);
}

void alien12(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy; // no gravity
    r11->dx += r11->dx>>6; r11->dy += r11->dy>>6;
    if (r11->dx > _speedlim) r11->dx = _speedlim;
    if (r11->dx < -_speedlim) r11->dx = -_speedlim;
    if (r11->dy > _speedlim) r11->dy = _speedlim;
    if (r11->dy < -_speedlim) r11->dy = -_speedlim;

    alienwanderfly(r11);

    bulcolchadd(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > 4*_sleeprange) alsleep(12, r11);
    if (abs(r11->y-ypos) > 4*_sleeprange) alsleep(12, r11);
    int rng = random();
    if ((rng&0x1f) == 0) alshootnuttermental(r11);

    if (plotal == 0) return;

    relplot(alspradr, 12+(random()&3), r11->x, r11->y);
}

void alien13(alent* r11)
{
    colcheck(r11, 2, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    alienwanderfly(r11);

    bulcolchaddshort(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight/2))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(13, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(13, r11);

    if (plotal == 0) return;

    relplot(alspradr, 8+2*(r11->dx > 0)+((framectr&8) == 8), r11->x, r11->y);
}

void alien14(alent* r11)
{
    colcheck(r11, 1, r11->y-(8<<8));
    char* r5 = albcheck(r11);
    r11->x += r11->dx; r11->y += r11->dy;

    r11->dy++;
    if (r11->dy > (1<<9)) r11->dy = 1<<8;

    bulcolchadd(r11);
    if (r11->downhit == 1) hamsterspecial(r11);
    if (plcolcheck(r11->x, r11->y, _alwidth, _alheight))
    {
        pllosestrength(_explocontloss); r11->r6 -= _explocontloss;
    }
    // [R11,#20] ; alien strength
    if (r11->r6 <= 0)
    {
        alkill(r11); return;
    }
    if (abs(r11->x-xpos) > _sleeprange) alsleep(14, r11);
    if (abs(r11->y-ypos) > _sleeprange) alsleep(14, r11);

    if (plotal == 0) return;

    relplot(alspradr, 18, r11->x, r11->y);
}

void hamsterspecial(alent* r11)
{
    r11->type = _Alien1+11;
    r11->r6 = 12<<8;
    r11->dx = r11->dy = 0;
}

void alkill(alent* r11)
{
    addtoscore(((r11->type-(_Alien1-1))&0x1f)*100);
    explogo(r11->x, r11->y, 0, 0, 0, /*r6*/ 0, r11 /*-4*/);
    exploins(r11);
}

void alshoot(alent* r11)
{
    int r2 = xpos-r11->x;
    int r3 = (ypos+(8<<8))-(r11->y-(6<<8)); //  aim at body

    if ((r2 > _firerange) || (r3 > _firerange)) return;
    if ((r2 < -_firerange) || (r3 < -_firerange)) return;
    int r4 = random()&7; if (r4 == 7) r4 = 6;
    makebul(r11->x, r11->y, (r2>>6), (r3>>6), r4, (1<<8)*BULL_TTL);

    bidforsound(_Explochannel, _Sampsmallzap, 0x78, fullpitch+0x1000, // pitch
                0, (fullpitch<<16)|0xfe00, 2 /* lifetime (frames) */, 0, CHUNK_SHOOT);
   noshootsound:;
}

void alshootfast(alent* r11)
{
    int r2 = xpos-r11->x;
    int r3 = (ypos+(8<<8))-(r11->y-(6<<8)); //  aim at body

    if ((r2 > _firerange) || (r3 > _firerange)) return;
    if ((r2 < -_firerange) || (r3 < -_firerange)) return;
    int r4 = random()&7; if (r4 == 7) r4 = 6;
    makebul(r11->x, r11->y, (r2>>5), (r3>>5), r4, (1<<8)*BULL_TTL);

    bidforsound(_Explochannel, _Sampsmallzap, 0x6f, fullpitch+0x1000, // pitch
                0, (fullpitch<<16)|0xfe00, 2 /* lifetime (frames) */, 0, CHUNK_SHOOT);
}

void alshootnutter(alent* r11)
{
    int r2 = xpos-r11->x;
    int r3 = (ypos+(12<<8))-(r11->y-(8<<8)); //fire from sensible place;  aim at body

    if ((r2 > _firerange) || (r3 > _firerange)) return;
    if ((r2 < -_firerange) || (r3 < -_firerange)) return;
    makebul(r11->x, r11->y, 0, -(1<<10), 12, (1<<6)*BULL_TTL);

    bidforsound(_Explochannel, _Sampbigzap, 0x6f, fullpitch, // pitch
                0, (fullpitch<<16)|0xfe00, 2 /* lifetime (frames) */, 0, CHUNK_SHOOTNUTTER);
}

void alshootnutterplus(alent* r11)
{
    int r2 = xpos-r11->x;
    int r3 = (ypos+(12<<8))-(r11->y-(8<<8)); //fire from sensible place;  aim at body

    if ((r2 > _firerange) || (r3 > _firerange)) return;
    if ((r2 < -_firerange) || (r3 < -_firerange)) return;
    makebul(r11->x, r11->y, 0, -(1<<10), 13, (1<<5)*BULL_TTL);

    bidforsound(_Explochannel, _Sampbigzap, 0x6f, fullpitch, // pitch
                0, (fullpitch<<16)|0xfe00, 2 /* lifetime (frames) */, 0, CHUNK_SHOOTNUTTER);
}

void alshootnuttermental(alent* r11)
{
    int r2 = xpos-r11->x;
    int r3 = (ypos+(12<<8))-(r11->y-(8<<8)); //fire from sensible place;  aim at body

    if ((r2 > _firerange) || (r3 > _firerange)) return;
    if ((r2 < -_firerange) || (r3 < -_firerange)) return;
    int r = random();
    if (r&(1<<11))
    {
        r2 = 0; r3 = -(1<<10);
    }
    else
    {
        r2 = (r&(1<<10)) ? (1<<10) : -(1<<10); r3 = 0;
    }
    makebul(r11->x, r11->y, r2, r3, 14, (1<<5)*BULL_TTL);

    bidforsound(_Explochannel, _Sampbigzap, 0x6f, fullpitch, // pitch
                0, (fullpitch<<16)|0xfe00, 2 /* lifetime (frames) */, 0, CHUNK_SHOOTNUTTER);
}

void alshootmental(alent* r11)
{
    int r2 = xpos-r11->x;
    int r3 = (ypos+(8<<8))-(r11->y-(6<<8)); //  aim at body

    if ((r2 > _firerange) || (r3 > _firerange)) return;
    if ((r2 < -_firerange) || (r3 < -_firerange)) return;
    int r4 = 8+(random()&2);
    makebul(r11->x, r11->y, (r2>>6), (r3>>6), r4, (1<<8)*BULL_TTL);

    bidforsound(_Explochannel, _Sampsmallzap, 0x6f, fullpitch+0x1000, // pitch
                0, (fullpitch<<16)|0xfe00, 2 /* lifetime (frames) */, 0, CHUNK_SHOOT);
}

void alsleep(int s, alent* r11)
{
    char* r0 = translate(r11->x-(4<<8), r11->y-(8<<8));

    if (*r0 != 0) return;
    *r0 = 240+(s-1);
    r11->type = 0;
}

void explo(alent* r11)
{
    r11->x += r11->dx;
    r11->y += r11->dy;
    int r4 = r11->r5;
    int r5 = options.explospeed<<2;
    if (r4 <= r5)
        if (plcolcheck(r11->x, r11->y, _expwidth, _expheight))  // object size
            pllosestrength(_explocontloss);
    exploins(r11);
}

void exploins(alent* r11)
{
    r11->r5 = r11->r5+options.explospeed;
    if (r11->r5 < 0) r11->r5 = 50<<8; //trap negatives
    char r0 = (r11->r5)&0xff;
    if (r0 >= (8<<2))                 //explo frame length
    {
        r11->type = 0; return;
    }
    if (plotal == 0) return;
    relplot(exploadr, (r11->r5>>2)&0x3f, r11->x, r11->y);
}

void booby(alent* r11)
{
    char* r0 = fntranslate(r11->x+r11->dx, r11->y);

    if (embertrybombtarget(r0, r11)) return;
   noboobybomb:
    if (*r0 >= _blim) r11->dx = -(r11->dx>>1);
    r0 = fntranslate(r11->x+r11->dx, r11->y+r11->dy);
    if (embertrybooby(r0, r11)) return;
   noboobybooby:
    if (*r0 >= _blim) r11->dy = -(r11->dy>>1);
    r11->x += r11->dx;
    r11->y += r11->dy;
    r11->dy += (1<<4);               //gravity
    r11->r5++;
    if (r11->r5 < 0) r11->r5 = 1<<8; //trap negatives
    if (r11->r5 >= 140)              // booby lifetime
    {
        r11->type = 0; return;
    }
    if (plotal == 0) return;
    relplot(exploadr, 12+((r11->r5>>2)&3), r11->x, r11->y);
}

void ember(alent* r11)
{
    char* r0 = fntranslate(r11->x+r11->dx, r11->y);

    if (embertrybomb(r0, r11)) return;
   noemberbomb:
    if (*r0 >= _blim) r11->dx = -(r11->dx>>1);
    r0 = fntranslate(r11->x+r11->dx, r11->y+r11->dy);
    if (embertrygas(r0, r11)) return;
   noembergas:
    if (*r0 >= _blim) r11->dy = -(r11->dy>>1);
    r11->x += r11->dx;
    r11->y += r11->dy;
    r11->dy += (1<<4); //gravity

    r11->r5++;
    if (r11->r5 < 0) r11->r5 = 1<<8; //trap negatives
    if (r11->r5 >= 70)               // ember lifetime
    {
        r11->type = 0; return;
    }
    if (plotal == 0) return;
    relplot(exploadr, 8+(((r11->r5)>>2)&3), r11->x, r11->y);
}

void normbomb(char* r0, alent* r11)
{
    if ((r11->type&0x1f) != 15) // booby
	r11->type = 0;
    normbombsurvive(r0);
}

void bonusobjgot(alent* r11)
{
    int r0 = r11->r6;
    bonuscommon(r0-_bonuslow, r11->x, r11->y);
    r11->type = 0;
    //r11->dx = (random()&(0xfe<<1))-0xfe;
    //r11->dy = -(random()&(0xfe<<2))-(1<<9);
    //r11->r5 = 0xf60+(r0<<8);
    //r11->type = _Scoreobj;
}

void flyingbonus(alent* r11)
{
    int r4 = 0; // hit wall marker
    char* r0 = fntranslate(r11->x+r11->dx-(8<<8), r11->y-(8<<8));

    if ((*r0 >= _bonuslow) || (r0[boardwidth] >= _bonuslow))
        if (r11->dx <= 0)
        {
            r11->dx = -r11->dx; r4 = 1;
        }
    if ((r0[1] >= _bonuslow) || (r0[boardwidth+1] >= _bonuslow))
        if (r11->dx >= 0)
        {
            r11->dx = -r11->dx; r4 = 1;
        }
   flyingskip:
    r0 = fntranslate(r11->x+r11->dx-(8<<8), r11->y+r11->dy-(8<<8));
    if ((*r0 >= _bonuslow) || (r0[1] >= _bonuslow))
        if (r11->dy <= 0)
        {
            r11->dy = -r11->dy; r4 = 1;
        }
    if ((r0[boardwidth] >= _bonuslow) || (r0[boardwidth+1] >= _bonuslow))
        if (r11->dy >= 0)
        {
            r11->dy = -r11->dy; r4 = 1;
        }
   flyingvertskip:
    r0 = fntranslate(r11->x+r11->dx-(8<<8), r11->y+r11->dy-(8<<8));
    if ((*r0 >= _bonuslow) || (r0[boardwidth] >= _bonuslow))
        if (r11->dx <= 0)
        {
            r11->dx = -r11->dx; r4 = 1;
        }
    if ((r0[1] >= _bonuslow) || (r0[boardwidth+1] >= _bonuslow))
        if (r11->dx >= 0)
        {
            r11->dx = -r11->dx; r4 = 1;
        }
    r0 = fntranslate(r11->x+r11->dx-(8<<8), r11->y+r11->dy-(8<<8));
    if ((*r0 >= _bonuslow) || (r0[1] >= _bonuslow))
        if (r11->dy <= 0)
        {
            r11->dy = -r11->dy; r4 = 1;
        }
    if ((r0[boardwidth] >= _bonuslow) || (r0[boardwidth+1] >= _bonuslow))
        if (r11->dy >= 0)
        {
            r11->dy = -r11->dy; r4 = 1;
        }
    r11->x += r11->dx;
    r11->y += r11->dy;
    r11->dy += (1<<5); //gravity
    if (r4 != 0)
    {
        r11->dx -= (r11->dx>>2);
        r11->dy -= (r11->dy>>2);
    }
   flyingnoslow:

    r11->r5 -= (1<<16);
    if (r11->r5 < 0)
    {
       bonusdowngrade:
        r11->type = _Dyingbonus;
        r11->r5 = r11->r6|(1<<20);
    }

    if (plcolcheck(r11->x, r11->y, (16<<8), 0))
        bonusobjgot(r11);


    if (plotal == 0) return;
// r8=8-(r8>>17); ???
    relplot(blockadr, r11->r6&0xff, r11->x, r11->y);
}

void dyingbonus(alent* r11)
{
    r11->r5 -= (1<<16);
    if (r11->r5 < 0)
    {
        deleteobj(r11); return;
    }
    if (plotal == 0) return;
    plotdying(blockadr, r11->r5&0xff, r11->x, r11->y, r11->r5);
}

void scoreobj(alent* r11)
{
    r11->x += r11->dx;
    r11->y += r11->dy;
    r11->dy += (1<<5);
    r11->r5 -= 1;
    if ((r11->r5&0xff) == 0)
    {
        r11->type = 0; return;
    }
    if (plotal == 0) return;
    relplot(blokeadr, (r11->r5>>8), r11->x, r11->y);
}


void plat1(alent* r11)       // alien routines
{
    r11->dx = 0;
    dvcheck(r11);
    r11->y += r11->dy;
    r11->dy += 1<<4;
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    if (r11->downhit != 0) platland(r11, _platno);
    colchadd(r11);
    platins(r11, _platno);
}


void plat2(alent* r11)
{
    r11->dx = 0;
    dvcheck(r11);
    r11->y += r11->dy;
    r11->dy += 1<<4;
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    if ((r11->downhit != 0) || (r11->uphit != 0)) plattoexplo(r11 /*,_platno+2*/);
    colchadd(r11);
    platins(r11, _platno+2);
}

void plat3(alent* r11)
{
    r11->dx = 0;
    dvcheck(r11);
    r11->y += r11->dy;
    r11->dy += 1<<4;
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    if (r11->downhit != 0) platland(r11, _platno+4);
    colchadd(r11);
    platins(r11, _platno+4);
}

void plat4(alent* r11)
{
    r11->dx = 0;
    dvcheck(r11);
    r11->y += r11->dy;
    r11->dy += 1<<4;
    if (r11->dy > (1<<11)) r11->dy = 1<<11;
    if (r11->downhit != 0) platland(r11, _platno-8+(r11->type<<1));
    colchadd(r11);
    platins(r11, _platno-8+(r11->type<<1));
}

void plat5(alent* r11)
{
    r11->dx = 0;
    dvcheck(r11);
    r11->y += r11->dy;
    r11->dy += 1<<2;
    if (r11->dy > (1<<9)) r11->dy = 1<<9;
    if (r11->downhit != 0) platland(r11, _platno+14);
    // .platlandins (duplicate label ignored)
    colchadd(r11);
    platins(r11, _platno+14);
}

void plattoobj(char* r0)
{
    plattoobjins(r0, 0);
}

void plattoobjins(char* r0, int r4)
{
    char r1 = *r0;

    if (1&r1) r0--;
    int x, y;
    backtranslate(r0, &x, &y);

    char r = (*r0-_platno+2*_Platbase)>>1;
    if (r == _Exploplat) r4 = -1-(1>>5); /* >> ???; stop instant explosion*/
    if (r >= _Downplat) r++;
    int v = makeobj(r, x+(8<<8), y+(15<<8), 0, r4, (32<<16)+16, *r0);

    if (!v)
    {
        *r0 = 0; r0[1] = 0;
    }
}

//.platlandins
//colchadd(r11);
//goto platins;

void platins(alent* r11, char r9)
{
    if (plotal == 0) return;
    relplot(blockadr, r9, r11->x-(8<<8), r11->y);
    relplot(blockadr, r9+1, r11->x+(8<<8), r11->y);
}

//aldone:
//LDMFD R13!,{R9,R11,PC}

void colchadd(alent* r11)
{
    if (colchptr-colchtab >= _colchlim) return;
    colchptr->r0 = r11;
    int xoff = (r11->r5)>>16;
    int yoff = (r11->r5)&0xffff;
    colchptr->xmin = r11->x-(xoff<<7); colchptr->ymin = r11->y-(yoff<<7);
    colchptr->xmax = r11->x+(xoff<<7); colchptr->ymax = r11->y+(yoff<<7);
    colchptr++;
}

void bulcolchadd(alent* r11)
{
    if (bulcolchptr-bulcolchtab >= _bulcolchlim) return;
    bulcolchptr->r0 = r11;
    bulcolchptr->xmin = r11->x-(32<<7); bulcolchptr->ymin = r11->y-(32<<7);
    bulcolchptr->xmax = r11->x+(32<<7); bulcolchptr->ymax = r11->y+(32<<7);
    bulcolchptr++;
}

void bulcolchaddshort(alent* r11)
{
    if (bulcolchptr-bulcolchtab >= _bulcolchlim) return;
    bulcolchptr->r0 = r11;
    bulcolchptr->xmin = r11->x-(32<<7); bulcolchptr->ymin = r11->y+(8<<8)-(16<<7);
    bulcolchptr->xmax = r11->x+(32<<7); bulcolchptr->ymax = r11->y+(8<<8)+(16<<7);
    bulcolchptr++;
}

void platland(alent* r11, char r9)
{
    char* r0 = translate(r11->x, r11->y);
    char r1 = *r0;

    if (*r0 >= _platblim) r0 -= boardwidth;
    if ((*(r0+boardwidth) != _extendno)
        && (*(r0+boardwidth+1) != _extendno))
    {
        *r0 = r9; r0[1] = r9+1;
        r11->type = 0;
    }
}

void plattoexplo(alent* r11)
{
    blowup(r11);
}

void deleteobj(alent* r11)
{
    r11->type = 0;
}

void blowup(alent* r11)
{
    explogo(r11->x, r11->y, 0, 1<<6, 0, /*r6*/ 0, r11 /*-4*/);
}

void switchcolch()
{
    oldcolchtab = colchtab;
    oldcolchptr = colchptr;
    colchplace = !colchplace;
    colchtab = colchptr = colchofs+(colchplace ? 0 : _colchlim+16); //+16 for safety

    bulcolchptr = bulcolchtab;                                      //reset bullet checking table
}

alent* bulcolcheck(int x, int y)
{
   bulcolchins:
    for (bulcolchent*r11 = bulcolchtab; r11 < bulcolchptr; r11++)
    {
       bl10:
        if ((r11->xmax >= x) && (r11->ymax >= y) && (x >= r11->xmin) && (y >= r11->ymin))
           bullethit: return r11->r0;
    }
   bulcolched:
    return NULL;
}

int projhital(alent* al, int loss)
{
    al->r6 -= loss;
    if (al->r6 > 0) return 0;
    int r0 = al->type;
    int r6 = (random()&7)+((r0 >= _Alien1) ? (r0-_Alien1) : 0)+_bonuslow;
    if (r6 > _bonushigh) r6 = _bonushigh-3;
    if (r6 < _bonuslow) r6 = _bonuslow;
    return r6;
}

void colcheck(alent* al, int colchecktype, int platypos)
{
    int x, y;

    dodgypointer = al;
    if (colchecktype != 0)
    {
        x = al->x; y = al->y;
    }
    else
    {
        x = xpos; y = ypos;
    }
    alonobj = colchecktype;
    if (colchecktype == 0) //player check
    {
        platsandstr = NULL;
        colchtabuse = colchtab;
        colchptruse = colchptr;
    }
    else
    {
        colchtabuse = oldcolchtab;
        colchptruse = oldcolchptr;
    }

//ADD R0,R0,R2
//ADD R1,R1,R3

    xtemp = x; ytemp = y;
    int r4 = 24<<8, r5 = 32<<8;
    int xlo = x-(r4>>1);
    int ylo = y-(r5>>1);
    ylo += 8<<8;
    int xhi = xlo+r4;
    int yhi = ylo+r5;
    yhi += 2<<8; //so always in contact with platforms

    colchent* r11 = colchtabuse-1;
    while (1)
    {
       colchcon:
       colchins:
        do
        {
           l10:
            r11++;
            if (r11 >= colchptruse)
               colched:
                return;
        }
        while (!((r11->xmax >= xlo) && (r11->ymax >= ylo) && (xhi >= r11->xmin) && (yhi >= r11->ymin)));
        if ((r11->ymax-ylo >= (1<<8))     //lim for plat on head -> can move horiz.
            && (yhi-r11->ymin >= (3<<8))) //same for standing on plat
        {
           limr2:
            if (r11->xmax > xhi) if (al->dx > 0) al->dx = 0;
            if (r11->xmin < xlo) if (al->dx < 0) al->dx = 0;
        }
       nolimr2:
        if (((r11->xmax-xlo) >= (6<<8))      //margin for right on edge
            && ((xhi-r11->xmin) >= (6<<8)))  //ditto
        {
            if ((yhi-r11->ymin) <= (16<<8))  //safety margin for platform under pl
                rise(r11->r0, al, colchecktype, platypos);
            if ((r11->ymax-ylo) <= (16<<8))  //ditto for plat on head
                platonhead(r11->r0, al, colchecktype);
        }
    }
}

void rise(alent* r6, alent* al, int colchecktype, int platypos)
{
    if (colchecktype == 2) //no rise
    {
        dontrise: al->dy = 0; alonobj = 1; return;
    }
    int r0 = r6->type;
    if (r0 == _Fastplatfire) platfire(r6);
    if ((r0 == _Downplat) || (r0 == _Fallplat)) //plat type 5 falls
    {
       platfall:
        if (r0 == _Fallplat) r0 = r6->dy;else r0 = (1<<4)+r6->dy;
        if (r0 < -(1<<5)) r0 += (1<<5); //plat is moving up - more speed
        if (r0 > (1<<11)) r0 = 1<<11;
        r6->dy = r0;
        if (al->dy > r0) al->dy = r0;
        if (colchecktype == 0)
        {
            al->falling = 0xff; platsandstr = &r6->dy;
        }
        al->dy += r6->y-(32<<8)-platypos;
        return;
    }
    if (r0 > _Fastplat)  //plat 6+ moves quickly
        r6->dy -= (1<<7);
    if (colchecktype == 0) r0 = al->pluphit;
    else r0 = alheadcheck();

    if (r0 == 0) r0 = r6->dy /*hit*/-(1<<5);
    else r0 = headonroof(r6, al, colchecktype, platypos);

    if (r0 > (1<<5)) r0 -= 1<<5; //plat is moving down - more speed

    if (r0 < -(3<<10))
    {
        r0 = -(3<<10);
       platmaxspeed:
        if (r6->type == _Fastplatstop)
        {
           platstop:
            platsurefire(r6);
            r0 = -(1<<9);
        }
        else if ((r6->type == _Fastplatexplo) || (r6->type == _Fastplatfire))
        {
           platexplo:
            platdestroy(r6);
            return;
        }
    }
   platmaxspeedins:;

    r6->dy = r0;
    if (al->dy >= r0) al->dy = r0;

    if (colchecktype == 0)
    {
        al->falling = 0xff; platsandstr = &r6->dy;
    }

    al->dy += (r6->y-(32<<8)-platypos);
    if (colchecktype != 0)
    {
        alonobj = 1; al->dx >>= 2;
    }

} //B colchcon


void platonhead(alent* r6, alent* al, int colchecktype)
{
    if (r6->type == _Exploplat)
    {
        platdestroy(r6); return;
    }
    if (r6->dy > 0) r6->dy = -r6->dy;
    if (r6->dy > al->dy) r6->dy += (al->dy>>1);
    // platuphit = 1; seems useless
    if (al->dy < 0) al->dy = 0;
    if (al->falling == 0xff) // only meaningful when colchecktype == 0
       platsandwich:
        if ((colchecktype == 0) && (platsandstr != NULL)) *platsandstr = (1<<8);
} //B colchcon

void platdestroy(alent* r6)
{
    explogo(r6->x, r6->y, 0, -(1<<6), 0, 0, r6);
}

void platfire(alent* r6)
{
// R3: best negative (up)
    int r4 = random();

    if ((r4&(3<<24)) == 0) // firing rate
        makebul(r6->x, r6->y+(16<<8), (r4&(0xfe<<2))-(0xfe<<1), -((r4&(0xfe<<12))>>11),
                8, (1<<8)*BULL_TTL);
   platfireskip:
    return;
}

void platsurefire(alent* r6)
{
// R3: best negative (up)
    int r4 = random();

    makebul(r6->x, r6->y+(16<<8), (r4&(0xfe<<2))-(0xfe<<1), -((r4&(0xfe<<12))>>11),
            14, (1<<8)*BULL_TTL);
}

//.colchcon
//LDMFD R13!,{R0,R1,R4,R5,R6,R11}
//B colchins

int headonroof(alent* r6, alent* al, int colchecktype, int platypos)
{
    int r0, r1;

    if (colchecktype != 0)
    {
        if ((random()&7) == 0)
            dodgypointer->y = (random()&(1<<9))-(1<<8);
       skipalrandload:
        r0 = 1; r1 = 0;
    }
    else
    {
        r0 = 1<<8; r1 = al->downpress;
    }
   skipalrand:
    r6->y = platypos+(32<<8); //force plat to player pos
    if (r1 > 64) r0 = 1<<11;
    if (r6->type == _Updownplat) r6->type = _Downplat;
    return r0;
}

int alheadcheck()
{
//ADD R1,R1,R3
    char* r0 = translate(xtemp, ytemp);

    return (*(r0-boardwidth) >= _blim) || (*(r0-boardwidth+1) >= _blim);
}

char* albcheck(alent* r11)
{
    r11->lefthit = r11->righthit = r11->uphit = r11->downhit = 0; // wipe all four
    alonplat = 0;
    xtemp = r11->x-(1<<8); ytemp = r11->y-(8<<8);
    int r6 = ytemp&(15<<8);
    char* z = translate(xtemp, ytemp+r11->dy);
// up
    if ((*(z-boardwidth) >= _blim) || (*(z-boardwidth+1) >= _blim))
        noup(r11);
//down
    if ((*(z+boardwidth) >= _blim) || (*(z+boardwidth+1) >= _blim))
        nodown(r11);
    if (r6 != (15<<8))
    {
        z = translate(xtemp+r11->dx, ytemp);
//left
        if ((*(z-boardwidth) >= _blim) || (*z >= _blim) || (*(z+boardwidth) >= _blim))
            noleft(r11);
//right
        if ((*(z-boardwidth+1) >= _blim) || (*(z+1) >= _blim) || (*(z+boardwidth+1) >= _blim))
            noright(r11);
    }
    else
    {
       alshort:
        z = translate(xtemp+r11->dx, ytemp);
        if ((*z >= _blim) || (*(z+boardwidth) >= _blim))
            noleft(r11);
//right
        if ((*(z+1) >= _blim) || (*(z+boardwidth+1) >= _blim))
            noright(r11);
    }

   albcheckins:
    z = translate(xtemp+r11->dx, ytemp+r11->dy);
// up
    if ((*(z-boardwidth) >= _blim) || (*(z-boardwidth+1) >= _blim))
        noup(r11);
//down
    if ((*(z+boardwidth) >= _blim) || (*(z+boardwidth+1) >= _blim))
        nodown(r11);
    char* r5 = (z+(boardwidth<<1)); //signal position to caller
    if ((*r5 <= _alplathighlim) && (*r5 >= _platlowlim)) alonplat = 1;
    return r5;
}



void noleft(alent* r11)
{
    if (r11->dx < 0)
    {
        r11->dx = 0;
        r11->lefthit = 1;
    }
}

void noright(alent* r11)
{
    if (r11->dx > 0)
    {
        r11->dx = 0;
        r11->righthit = 1;
    }
}

int fallinggap(alent* re)
{
    char* r11 = translate(re->x+re->dx, re->y /*+dy*/-((re->dy < 0) ? (15<<8) : 0));
    int r1 = (re->dx < 0) ? 0 : 1;

    if ((*(r11+r1) < _blim) && (*(r11+boardwidth+r1) < _blim))
    {
       stopfall:
        if (re->dy > 0)
        {
           stopfalldown:
            if (*(r11-boardwidth+r1) < _blim) return 0;
            if (*(r11+boardwidth*2+r1) < _blim) return 0;
            if (*(r11+boardwidth*2+(1-r1)) < _blim)
            {
                nodown(re); return 1;
            }
        }
        else if (re->dy < 0)
        {
           stopfallup:
            if ((ytemp&(15<<8)) > (8<<8)) return 0;
            if (*(r11-boardwidth+r1) < _blim) return 0;
            if (*(r11+boardwidth*2+r1) < _blim) return 0;
            if (*(r11+boardwidth*2+(1-r1)) < _blim)
            {
                noup(re); return -1;
            }
        }
    }
    return 0;
}

void nodown(alent* r11)
{
    if (r11->dy > 0)
    {
        r11->dy = ((15<<8)&((16<<8)-ytemp))-1;
        if (r11->dy < 0) r11->dy = 0;
    }
   nodownrel:
    r11->downhit = 1;
}

void nodownifplat(alent* r11)
{
    if (r11->falling != 0xff) return;
    if (r11->dy > 0)
    {
        r11->dy = ((15<<8)&((16<<8)-ytemp))-1;
        if (r11->dy < 0) r11->dy = 0;
    }
//nodownrel:
    r11->downhit = 1;
}

void noup(alent* r11)
{
    if (r11->dy >= 0) return;
    r11->dy = -((ytemp+(1<<8))&(15<<8));
    r11->uphit = 1;
}

void dvcheck(alent* r11)
{
    xtemp = r11->x; ytemp = r11->y;
    r11->uphit = r11->downhit = 0;
    char* r6 = translate(r11->x, r11->y+r11->dy);
// up
    if ((r6[-boardwidth] >= _platblim) || (r6[1-boardwidth] >= _platblim)) noup(r11);
//down
    if ((r6[0] >= _platblim) || (r6[1] >= _platblim)) nodown(r11);
}

void wipealtab()
{
    //aladr = alofs;
    alent* r10 = aladr;
    alctr = 0;
    for (int r9 = _alno; r9 >= 0; r9--)
    {
       l9:
        r10->type = 0;
        r10++;
    }
    boardreg(); // fallthrough?
}

void causeexplo(alent* r11)
{
    explogomaybe(r11->x-r11->dx, r11->y-r11->dy-(8<<8), 0, 0, 1, -1, 0);
}

void causeexplonopyro(alent* r11)
{
    explogonopyro(r11->x-r11->dx, r11->y-r11->dy-(8<<8), 0, 0, 1, -1, 0);
}

void explogonopyro(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    explocreate(r1, r2, r3, r4, r5, r6, r10);
}

void explogonopyroquiet(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    explocreatequiet(r1, r2, r3, r4, r5, r6, r10);
}

void explogoquiet(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    explocreatequiet(r1, r2, r3, r4, r5, r6, r10);
    embercreate(r1, r2, r6);
}

void explogomaybe(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    explocreate(r1, r2, r3, r4, r5, r6, r10);
    if ((random()&3) == 0) embercreate(r1, r2, r6);
}

void explogo(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    explocreate(r1, r2, r3, r4, r5, r6, r10);
    embercreate(r1, r2, r6);
}

void explocreate(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    if (r10 != NULL)
    {
        r1 = r10->x;  r2 = r10->y;
    }
    int vol = 0x7f-((abs(r1-xpos)+abs(r2-ypos))>>12);
    if (vol >= 80)
    {
        bidforsound(_Explochannel, _SampExplo, ((vol&0x7f) > 0x7c) ? 0x7c : (vol&0x7f),
                    //   channel,        , vol,
                    0x3800, 0, 0, 10, ((r1-xpos)>>9), CHUNK_EXPLO);
        //pitch, , ,lifetime (frames)
    }
   noexplosound:
    explocreatequiet(r1, r2, r3, r4, r5, r6, r10);
}


void explocreatequiet(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    if (r10 == NULL) //(r10<8)
       noobject:
        softmakeobj(_Explo, r1, r2+(8<<8), r3, r4, (r5 != 0) ? (1<<15) : 0, r6);
    else
    {
        r10->type = _Explo;
        r10->dx = r3; r10->dy = r4;
        r10->r5 = 0;
    }
   noobjectins:;
}

void embercreate(int r1, int r2, int r6)
{
    r2 += (8<<8); // The original code achieves this by modifying r2
                // inside explocreatequiet.  Arrgh.
    softmakeobj(_Ember, r1, r2, 1<<8, -(1<<8), 0, r6);
    softmakeobj(_Ember, r1, r2, -(1<<8), -(1<<8), 0, r6);
}

void atomexplogo(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10)
{
    explogoquiet(r1, r2, r3, r4, r5, r6, r10);
    explogoquiet(r1, r2-(16<<8), r3, -(1<<6), r5, r6, r10);
    explogoquiet(r1, r2-(32<<8), r3, -(1<<7), r5, r6, r10);
    explogoquiet(r1+(12<<8), r2-(24<<8), 1<<6, -(3<<5), r5, r6, r10);
    explogoquiet(r1-(12<<8), r2-(24<<8), -(1<<6), -(3<<5), r5, r6, r10);
    int r7 = (r1-xpos)>>9;
    r1 = 0x8f-((abs(r1-xpos)+abs(r2-ypos))>>12);
    if (r1 > 0x7f) r1 = 0x7f;
    if (r1 >= 60)
/* AND R2,R1,#&7F:MOV R2,#&7F ??? */
        bidforsound(_Explochannel, _SampAtomExplo, 0x7f, 0x2800, 0, 0, 50, r7, CHUNK_ATOM);
   noatomsound:
    return;
}

void save_alents(uint8_t store[_savearealen*28])
{
    uint8_t* st = store;
    uint8_t* st_end = st+(_savearealen-1)*28;
    alent* al = aladr;
    for (int r9 = _alno; r9 > 0; r9--)
    {
       procsaveal:
        if (st >= st_end) break;
        switch (al->type)
        {
        case _Explo: //don't save these
        case _Scoreobj:
        case _Flyingbonus:
        case _Dyingbonus:
        case 0:
            al++; break;
        default:
            write_littleendian(st, al->type);
            write_littleendian(st+4, al->x);
            write_littleendian(st+8, al->y);
            write_littleendian(st+12, al->dx);
            write_littleendian(st+16, al->dy);
            write_littleendian(st+20, al->r5);
            write_littleendian(st+24, al->r6);
            st += 28;
            al++;
        }
    }

    write_littleendian(st, 0xffffffff); // end marker
}

void restore_alents(uint8_t store[_savearealen*28])
{
    alent* al = aladr;
    uint8_t* st = store;
    uint8_t* st_end = st+(_savearealen-1)*28;
    for (; (read_littleendian(st) != 0xffffffff) && (st < st_end);)
    {
       sl1:
        al->type = read_littleendian(st);
        al->x = read_littleendian(st+4);
        al->y = read_littleendian(st+8);
        al->dx = read_littleendian(st+12);
        al->dy = read_littleendian(st+16);
        al->r5 = read_littleendian(st+20);
        al->r6 = read_littleendian(st+24);
        al++;
        st += 28;
    }
}

int softmakeobj(int r0, int r1, int r2, int r3, int r4, int r5, int r6)
{
    alent* r10 = aladr;
    int r9 = _alno;

    if (alctr >= r9) alctr = 0;
    r9 -= alctr;
    if (r9 < 10) /* check 10 object spaces */
    {
        alctr = 0;
        r10 = aladr;
    }
    else r10 += alctr;
    do
    {
       softloop:
        if (r10->type == 0) return foundmakeal(r10, alctr, r0, r1, r2, r3, r4, r5, r6);
        r10++;
        alctr++;
    }
    while (--r9 > 0);
    return 1;
}

int makeobj(int r0, int r1, int r2, int r3, int r4, int r5, int r6)
{
    int tmpalctr = alctr;
    alent* r10 = aladr;
    int r9 = _alno;

    if (tmpalctr >= r9) tmpalctr = 0;
    r9 -= tmpalctr;
    r10 += tmpalctr;
    do
    {
       loop52:
        if (r10->type == 0) return foundmakeal(r10, tmpalctr, r0, r1, r2, r3, r4, r5, r6);
        r10++;
        tmpalctr++;
    }
    while (--r9 > 0);
    r10 = aladr;
    r9 = alctr;
    do
    {
       loop53:
        if (r10->type == 0) return foundmakeal(r10, tmpalctr, r0, r1, r2, r3, r4, r5, r6);
        r10++;
        tmpalctr++;
    }
    while (--r9 > 0);
    return 1;
}

int foundmakeal(alent* r10, int newalctr, int r0, int r1, int r2, int r3, int r4, int r5, int r6)
{
    r10->type = r0;
    r10->x = r1; r10->y = r2; r10->dx = r3; r10->dy = r4;
    r10->r5 = r5; r10->r6 = r6;

    alctr = newalctr+1;
    if (alctr >= _alno) alctr = 0;
    return 0;
}

void init_chunk_alien()
{
    CHUNK_EXPLO = make_sound(_SampExplo, 0x3800, 0, 0, 10);
    CHUNK_ATOM = make_sound(_SampAtomExplo, 0x2800, 0, 0, 50);
    CHUNK_SPINFIRE = make_sound(_Sampsmallzap, fullpitch+0x1000, 0, 0, 2);
    CHUNK_SPINPOWERFIRE = make_sound(_Sampbigzap, fullpitch, 0, 0, 2);
    CHUNK_SHOOT = make_sound(_Sampsmallzap, fullpitch+0x1000, 0, (fullpitch<<16)|0xfe00, 2);
    CHUNK_SHOOTNUTTER = make_sound(_Sampbigzap, fullpitch, 0, (fullpitch<<16)|0xfe00, 2);
}
