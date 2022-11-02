/*  projectile.c */

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

#define _projno 32
#define _projlim 0x1000

#define _projsmallno 54
#define _rocketspriteno 32
#define _rocketspritelim 41

#define bulletloss (2<<8)

const char _spcrumblelowlim = 140, _spcrumblehighlim = 143;
extern const char _translowlim, _transhighlim;

extern int xposmax, yposmax;
extern fastspr_sprite blokeadr[];
extern char masterplotal;

projent projofs[_projlim];

projent* projadr;
int projctr;

void causeexplo(projent* r11)
{
    explogomaybe(r11->x-r11->dx, r11->y-r11->dy-(8<<8), 0, 0, 1, -1, 0);
}
void causeexplonopyro(projent* r11)
{
    explogonopyro(r11->x-r11->dx, r11->y-r11->dy-(8<<8), 0, 0, 1, -1, 0);
}

void project()       // the projectile handler
{
    int i = _projno; // get table length
    projent* r11 = projadr;

    for (; i > 0; i--, r11++)
    {
       loop41:
        if (r11->type == 0) continue;
       foundproj:
        r11->x += r11->dx; r11->y += r11->dy;
        if ((r11->x <= xlowlim) || (xposmax <= r11->x) || (r11->y <= ylowlim) || (yposmax <= r11->y))
        {
            // goto projoffscr
            r11->type = 0;
            continue;
        }
        if (r11->flags&PROJ_ROCKET)
        {
            r11->dx += r11->dx>>4;  r11->dy += r11->dy>>4;
        }

       projnoacc:
        if (r11->dx > _speedlim) r11->dx = _speedlim;
        if (r11->dx < -_speedlim) r11->dx = -_speedlim;
        if (r11->dy > _speedlim) r11->dy = _speedlim;
        if (r11->dy < -_speedlim) r11->dy = -_speedlim;

        r11->flags -= PROJ_TTL; // decrement the life counter
        if (r11->flags < 0)     // out of time
        {
           projdestroy:
            if (r11->flags&PROJ_SPLIT)
            {
                projsplit(r11); continue;
            }
           projoffscr:
            r11->type = 0;
            continue;
        }
        alent* rs = bulcolcheck(r11->x, r11->y);
        if (rs != NULL)
        {
            int r6;
            if (r11->flags&PROJ_ROCKET)
	        r6 = projhital(rs,bulletloss<<2);
            else r6 = projhital(rs, bulletloss);
            r11->type = 0;
            if (r6 == 0) continue;
            makeobj(_Flyingbonus, r11->x, r11->y, r11->dx>>2, -(1<<10), (0x200<<16), r6);
            if (r11->flags&PROJ_ROCKET) causeexplo(r11);
            else causeexplonopyro(r11);
            continue;
        }

        char* r0 = fntranslate(r11->x, r11->y);
        char r1 = *r0;
        if ((r1 < 16) ||
            //    projhit:  // hit a block
            ((r1 >= _translowlim) && (r1 <= _transhighlim) && !(r11->flags&PROJ_ATOM)))
        {
           projhitins:
            if (masterplotal == 0) continue;
            relplot(blokeadr, r11->type, r11->x, r11->y);
            continue;
        }
       projhitcont:
        if ((r1 >= _spcrumblelowlim) && (r1 <= _spcrumblehighlim)
            && ((r11->type == _projsmallno+2) || (r11->flags&PROJ_WEIRDSPLIT)))
          // hack, better than original && (plweapontype == 5)
        {
            *r0 = 0;
            explogo(r11->x, r11->y, 0, 0, 0, 0, 0);
        }
       nospcrumble:
        r11->type = 0;
        destroy(r0);
        if (r11->flags&PROJ_ATOM)
        {
            atomrocket(r11, r0); continue;
        }
        if (r11->flags&PROJ_EXPLO)
        {
            causeexplonopyro(r11); continue;
        }
        if (r11->flags&PROJ_ROCKET)
        {
            causeexplo(r11); continue;
        }
    }
}

int projsplittab[10];

void init_projsplittab()
{
    for (int i = 0; i < 5; i++)
    {
        projsplittab[i*2] = (int)(0x40*(0.5-sin(i/4.0*M_PI)));
        projsplittab[i*2+1] = 0x100*(i-2);
    }
}

void projsplit(projent* r11)
{
    int x = r11->x, y = r11->y, dx = (r11->dx)>>1, dy = (r11->dy)>>1;
    int flags = r11->flags;
    int* r10;
    int r9;

    if (flags&PROJ_ROCKET)
    {
        rocketsplit(r11); return;
    }

    int r7 = r11->type, r4, r6;

    if (flags&PROJ_FIVEWAY)
    {
        r10 = projsplittab; r9 = 5; r4 = _projsmallno+1;
    }
    else
    {
        r10 = projsplittab+2; r9 = 3; r4 = _projsmallno;
    }
    if ((flags&PROJ_SLOWSPLIT) == 0) dx >>= 1;
    if (flags&PROJ_WEIRDSPLIT)
    {
        r4 = _projsmallno+2; dx <<= 2;
    }
    if (flags&PROJ_EXPLO)
    {
        r4 = _projsmallno+3; r6 = PROJ_ROCKET;
    }
    else r6 = 64*PROJ_TTL;

    for (; r9 > 0; r9--)
    {
       loop76:
        makeproj(x, y, dx+r10[0], dy+r10[1], r4, r6);
        r10 += 2;
    }
    r11->type = 0;
}

int rocketbursttab[10];

void init_rocketbursttab()
{
    for (int i = 0; i < 5; i++)
    {
        rocketbursttab[i*2] = (int)(0x200*sin((0.5+i)/2.5*M_PI));
        rocketbursttab[i*2+1] = (int)(0x200*cos((0.5+i)/2.5*M_PI));
    }
}

void rocketsplit(projent* r11)
{
    int x = r11->x, y = r11->y, dx = (r11->dx)>>3, dy = (r11->dy)>>3;
    int flags = r11->flags;

    if (flags&ROCK_DIVIDE)
    {
        rocketpair(r11); return;
    }
   rocketburst:;

    int r5 = (64*PROJ_TTL)|PROJ_ROCKET|PROJ_EXPLO;
    int* r10 = rocketbursttab;
    for (int r9 = 5; r9 > 0; r9--)
    {
       loop77:
        makeproj(x, y, dx+r10[0], dy+r10[1], _projsmallno+3, r5);
        r10 += 2;
    }
    r11->type = 0;
    causeexplonopyro(r11);
}

void rocketpair(projent* r11)
{
    int x = r11->x, y = r11->y, dx = r11->dx, dy = r11->dy;
    int flags = r11->flags;
    int type = r11->type;
    int newflags = 64*PROJ_TTL;

    if (flags&ROCK_BURST) newflags = (16*PROJ_TTL)|ROCK_BURST|PROJ_SPLIT;
    if (flags&ROCK_REDIVIDE) // continue splitting
    {
        newflags &= ~(64*PROJ_TTL); newflags |= (16*PROJ_TTL)|ROCK_DIVIDE|PROJ_SPLIT;
    }
    newflags |= PROJ_ROCKET;

    int newtype = type-2;
    if (newtype < _rocketspriteno) newtype += 2;

    if (type&1) makeproj(x, y, dx-(dy>>2), dy+(dx>>2)-(dy>>4), newtype, newflags);
    else makeproj(x, y, dx+(dy>>2), dy-(dx>>2)+(dy>>4), newtype, newflags);

    newtype = type+2;
    if (newtype > _rocketspritelim) newtype -= 2;

    if (type&1) makeproj(x, y, dx+(dy>>2), dy-(dx>>2)+(dy>>4), newtype, newflags);
    else makeproj(x, y, dx-(dy>>2), dy+(dx>>2)-(dy>>4), newtype, newflags);

    r11->type = 0;
}

int makeproj(int x, int y, int dx, int dy, int type, int flags)
{
    projent* r10 = projadr+projctr;
    int r9 = _projno-projctr;
    int r8 = projctr;

    for (; r9 > 0; r9--)
    {
       loop42:
        if (r10->type == 0) return foundmakeproj(r10, r8, x, y, dx, dy, type, flags);
        r10++;
        r8++;
    }
    r10 = projadr;
    for (r9 = projctr; r9 > 0; r9--)
    {
       loop43:
        if (r10->type == 0) return foundmakeproj(r10, r8, x, y, dx, dy, type, flags);
        r10++;
        r8++;
    }
    return 1;
}

int foundmakeproj(projent* r10, int r8, int x, int y, int dx, int dy, int type, int flags)
{
    if (flags < (1*PROJ_TTL)) flags |= (64*PROJ_TTL);
    r10->type = type; r10->x = x; r10->y = y;
    r10->dx = dx; r10->dy = dy; r10->flags = flags;

    for (r8++; r8 >= _projno; r8 -= _projno)
       loop45:;
    projctr = r8;
    return 0;
}

void initprojtab()
{
    projadr = projofs;
    projent* r10 = projadr;
    projctr = 0;
    for (int r9 = _projno; r9 > 0; r9--)
    {
       loop44:
        *(int*)r10 = 0;
        r10++;
    }
}
