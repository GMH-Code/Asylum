/*  maze.c */

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
#include <SDL/SDL_mixer.h>

extern const char _bonuslow = 16, _bonushigh = 31, _megabonuslim = 35;
const char _neuronlowlim = 40, _neuronhighlim = 47;
const char _gaslowlim = 54, _gashighlim = 55;
const char _animlowlim = 54, _animhighlim = 63;
const char _eleclowlim = 56, _elecvertlowlim = 60, _elechighlim = 63;
const char _fuelairno = 68;
const char _bomblowlim = 64, _bombhighlim = 79;
const char _atomlowlim = 72, _atomhighlim = 75;
const char _boobylowlim = 76, _boobyhighlim = 79;
extern const char _targetlowlim = 80, _powertarget = 84, _nuttertarget = 88, _targethighlim = 95;
const char _weaplowlim = 96, _weaphighlim = 111;
const char _solidlowlim = 128, _solidhighlim = 131;
const char _crumblelowlim = 160, _crumblehighlim = 167;
const char _crumblestandlowlim = 168, _crumblestandhighlim = 175;
const char _starsno = 15;
const char _markerno = 255;

const char _triggerlim = 193;
#define _boxwidth 20
#define _boxheight 16
#define _boardhdrlen 32

extern fastspr_sprite blockadr[256];
extern board *boardadr;
extern int framectr;
int boardwidth;
int xposmax; int yposmax;
char *boardlowlim, *boardhighlim;
extern char atombombctr;

inline int block_anim(char b) {return ((b >= _animlowlim) && (b <= _animhighlim));}
inline int block_solid(char b) {return ((b >= _solidlowlim) && (b <= _solidhighlim));}
inline int block_crumblestand(char b) {return ((b >= _crumblestandlowlim) && (b <= _crumblestandhighlim));}
inline int block_bomb(char b) {return ((b >= _bomblowlim) && (b <= _bombhighlim));}
inline int block_target(char b) {return ((b >= _targetlowlim) && (b <= _targethighlim));}
inline int block_booby(char b) {return ((b >= _boobylowlim) && (b <= _boobyhighlim));}
int block_gas(char b) {return ((b >= _gaslowlim) && (b < _gashighlim));}

#define targetscore 500
#define bombloss (4<<8)
#define atomloss (32<<8)

Mix_Chunk* CHUNK_FUELAIR;
Mix_Chunk* CHUNK_BONUS_1;
Mix_Chunk* CHUNK_BONUS_2;

#define _windspeed (1<<8)

#define _fuelairduration 0x18

#define _fueltablim 0x3c0
char* fueltabofs[2][_fueltablim]; // &8000
// don't forget there's two of these

char** fueltabread;
char** fueltabwrite;
int fueltabctr;
int fuelairlastframe;

void fuelairproc()
{
    if (framectr < fuelairlastframe+((_fuelairduration-fueltabctr)>>1)) return;
    fuelairlastframe = framectr;

    char **ftr, **ftw;
    ftr = fueltabread; fueltabread = ftw = fueltabwrite; fueltabwrite = ftr;

    if (--fueltabctr < 0)
    {
        *ftw = 0;  return;
    }

    for (char** r7 = ftw+_fueltablim; r7 > ftw;)
    {
       loop22:;
        char* r8 = *(ftr++);
        if (r8 == NULL)
        {
            *ftw = 0;  return;
        }
        char r0 = (*r8)&~1;
        if (r0 != _gaslowlim) skipfuelair : continue;
        char* r1 = r8-1;
        if (*r1 == 0)
        {
            *r1 = _gaslowlim; *(ftw++) = r1;
        }
        r1 = r8+boardwidth;
        if (*r1 == 0)
        {
            *r1 = _gaslowlim; *(ftw++) = r1;
        }
        r1 = r8+1;
        if (*r1 == 0)
        {
            *r1 = _gaslowlim; *(ftw++) = r1;
        }
        r1 = r8-boardwidth;
        if (*r1 == 0)
        {
            *r1 = _gaslowlim; *(ftw++) = r1;
        }
    }
   fuelprocdone:
    *ftw = 0;
}

void prepfueltab()
{
    fueltabread = fueltabofs[0];
    fueltabwrite = fueltabofs[1];
    *fueltabread = 0;
    *fueltabwrite = 0;
    fueltabctr = 0;
    fuelairlastframe = 0;
}

int embertrybomb(char* r0, alent* r11)
{
    if (block_bomb(*r0))
    {
	emberbomb(r0, r11);
	return 1;
    }
    else return 0;
}

int embertrybombtarget(char* r0, alent* r11)
{
    if (block_bomb(*r0) || block_target(*r0)) // was excluding _targethighlim
    {
	emberbomb(r0, r11);
	return 1;
    }
    else return 0;
}

int embertrybooby(char* r0, alent* r11)
{
    if (block_booby(*r0))
    {
	emberbomb(r0, r11);
	return 1;
    }
    else return 0;
}

int embertrygas(char* r0, alent* r11)
{
    if (block_gas(*r0))
    {
	emberbomb(r0, r11);
	return 1;
    }
    else return 0;
}

//void emberbooby(char* r0)
//{
//if ((*r0 < _atomlowlim) || (*r0 > _atomhighlim)) normbombsurvive(r0);
//else atombomb(r0);
//}

void emberbomb(char* r0, alent* r11)
{
    if ((*r0 < _atomlowlim) || (*r0 > _atomhighlim)) normbomb(r0, r11);
    else atombomb(r0);
}


void normbombsurvive(char* r0)
{
    if ((*r0 == _fuelairno) || (*r0 == _fuelairno+1))
    {
        fuelbomb(r0); return;
    }
   normbombins:
    destroy(r0);
}

void fuelbomb(char* r0)
{
    if (*r0 != _fuelairno) r0--;
    *r0 = 0; r0[1] = 0;
    int x, y;
    backtranslate(r0, &x, &y);
    explogo(x+(8<<8), y, 0, -(1<<7), 0, 0, 0);
}

int seeifwind(char* r1, int* dx, int* dy, int retval)
{
    if (*r1 == 2)
    {
        *dx -= _windspeed*4; retval = 1;
    }
    if (*r1 == 3)
    {
        *dx += _windspeed*4; retval = 1;
    }
    if (*r1 == 12)
    {
        *dy -= _windspeed; retval = 1;
    }
    if (*r1 == 13)
    {
        *dy += _windspeed; retval = 1;
    }
    return retval;
}



void crumblecheck(char* r1)
{
    if (!block_crumblestand(*r1)) return;
    (*r1)++;
    if (((*r1)&3) == 0) *r1 = 0;
}

char* translate(int r0, int r1)
{
    return (char*)boardadr->contents+(r0>>12)+(r1>>12)*boardadr->width;
}

char* fntranslate(int r0, int r1)
{
    return (char*)boardadr->contents+((r0+(8<<8))>>12)+((r1-(8<<8))>>12)*boardadr->width;
}

void backtranslate(char* r, int* x, int* y)
{
    int r0 = r-boardadr->contents;
    int r1 = boardwidth;

    *x = (r0%r1)<<12;
    *y = (r0/r1)<<12;
}
/*LDR R1,[R12,#boardadr]
   ADD R1,R1,#boardhdrlen
   SUB R0,R0,R1
   LDR R1,[R12,#boardwidth]
   MOV R2,#0
   MOV R3,#0
   MOV R4,#1<<31
   .l9
   MOVS R0,R0,ASL #1
   ADC R3,R3,R3
   CMP R3,R1
   SUBGE R3,R3,R1
   ORRGE R2,R2,R4
   MOVS R4,R4,LSR #1
   BNE l9

   MOV R0,R3,LSL #12
   MOV R1,R2,LSL #12
   MOV PC,R14*/

int block_weapon(char b)
{
    if ((b < _weaplowlim) || (b > _weaphighlim)) return 0;
    return b-_weaplowlim+1;
}

int plbombcheck(char* r5)
{
    int r1 = bombcheck(r5);

    if ((*r5 >= _targetlowlim) && (*r5 <= _targethighlim))
        r1 = normalbomb(r5);
   nopltarg:
    return r1;
}

int bombcheck(char* r5)
{
    char r0 = *r5;

    if ((r0 < _bomblowlim) || (r0 > _bombhighlim)) return 0;
    if ((r0 == _fuelairno) || (r0 == _fuelairno+1)) return fuelairbomb(r5);
    if (r0 >= _boobylowlim) return boobybomb(r5);
    if ((r0 < _atomlowlim) || (r0 > _atomhighlim)) return normalbomb(r5);
    return atombomb(r5);
}

int atombomb(char* r5) // Caution - recursive routine
{
    if (atombombctr >= 0x1f)  atomabandon: return atomloss;
    atombombctr++;
    char r0 = (*r5)-_atomlowlim;
    if (r0&1) r5 -= 1;
    if (r0&2) r5 -= boardwidth;

    *r5 = 0;
    *(r5+1) = 0;
    *(r5+boardwidth) = 0;
    *(r5+boardwidth+1) = 0; // delete this a-bomb
    procatom(r5-boardwidth);
    procatom(r5-boardwidth+1);
    procatom(r5-1);
    procatom(r5+2);
    procatom(r5+boardwidth-1);
    procatom(r5+boardwidth+2);
    procatom(r5+2*boardwidth);
    procatom(r5+2*boardwidth+1);
    int x, y;
    backtranslate(r5, &x, &y);
    atomexplogo(x+(8<<8), y+(24<<8), 0, 0, 0, 0, 0 /*signal no existing object*/);
    return atomloss;
}

void procatom(char* r5)
{
    char r0 = *r5;

    if ((r0 >= _targetlowlim) && (r0 <= _targethighlim))
        shoottarget(r5);
   noatomtarget:
    *r5 = 0;

    if ((r0 < _atomlowlim) || (r0 > _atomhighlim)) return;

    *r5 = r0; //replace bomb segment
    atombomb(r5);
}

int fuelairbomb(char* r5)
{
    deletetwin(r5);
    char* r0 = r5-boardwidth;
    *r0 = _gaslowlim;
    *(r0+1) = _gaslowlim;
    if ((*fueltabread == 0) || (*(fueltabread+1) == 0))
        *(fueltabread+2) = 0; // replace end marker if overwritten
    *fueltabread = r0;
    *(fueltabread+1) = r0+1;
    fueltabctr = _fuelairduration; // fuel-air duration
    bidforsound(_Explochannel, _SampHiss, 0x6f, 0x6000, 0xff80, 0x2000ff00, 50, 0, CHUNK_FUELAIR);
    return 0;
}

int normalbomb(char* r5)
{
    elecdestroy(r5);
    *r5 = 0;
    int x, y;
    backtranslate(r5, &x, &y);
    explogo(x, y+(8<<8), 0, 0, 0, 0, 0 /*signal no existing object*/);
    return bombloss;
}

int boobyvectab[] =
{ -0x400, -0x80, 0x400, -0x80, 0, -0x400, 0, 0x400 };

int boobybomb(char* r5)
{
    int* r6 = boobyvectab+2*((*r5-_boobylowlim)&3);

    *r5 = 0;
    int x, y;
    backtranslate(r5, &x, &y);
    explogonopyro(x, y+(8<<8), -(r6[0]>>2), -(r6[1]>>2),
                  0, 0, 0 /*;signal no existing object*/);
    makeobj(_Booby, x, y+(8<<8), r6[0], r6[1], 0, 0);
    return bombloss;
}

void bonuslim(char* r5)
{
    if ((*r5 >= _neuronlowlim) && (*r5 <= _neuronhighlim))
    {
	plsetneuronzone(*r5-(_neuronlowlim-1));
        *r5 = 0;
        return;
    }
   noneuron:
    if ((*r5 >= _eleclowlim) && (*r5 <= _elechighlim))
    {
        electrocute(r5); return;
    }
   bonuslimcont:
    if (*r5 < _bonuslow) return;
    if (*r5 > _megabonuslim) return;
    if (*r5 > _bonushigh) megabonus(r5);
    else bonusgot(r5); // was bonusgot
}

void deadbonuslim(char* r5)
{
    char r0 = *r5;

    if ((r0 < _bonushigh-2) || (r0 > _bonushigh)) return;
    bonusgot(r5);
}

void bonusgot(char* r5)
{
// MOV R1,R0 ???
    int r0 = *r5;
    *r5 = 0;
    int x, y;
    backtranslate(r5, &x, &y);
    bonuscommon(r0-_bonuslow, x, y);
    //makescoreobj(x, y, 0xf60+(r0<<8));
}

void megabonus(char* r5)
{
    char r0 = *r5;

    *r5 = 0;
    bidforsound(_Playerchannel, _SampBonus, 0x7e, 0x2200, 0, 0, 5, 127, CHUNK_BONUS_1);
    bidforsound(_Sparechannel, _SampBonus, 0x7e, 0x2000, 0, 0, 5, -127, CHUNK_BONUS_2);
    if (r0 == _bonushigh+1)
    {
       bonus1:
        bonusnumb(10);
        message(96, 224, 0, -2, "Bonus 10000");
        addtoscore(10000);
    }
    else if (r0 == _bonushigh+2)
    {
       bonus2:
        bonusnumb(20);
        message(96, 224, 0, -2, "Bonus 20000");
        addtoscore(20000);
    }
    else if (r0 == _bonushigh+3)
    {
       bonus3:
        bonusnumb(30);
        message(96, 224, 0, -2, "Bonus 30000");
        addtoscore(30000);
    }
    else if (r0 == _bonushigh+4)
    {
       bonus5:
        bonusnumb(50);
        message(96, 224, 0, -2, "Bonus 50000");
        addtoscore(50000);
    }
    else
    {
       bonus10:
        bonusnumb(100);
        message(96, 224, 0, -2, "Bonus 100000");
        addtoscore(100000);
    }
    return;
}



void destroy(char* r5)
{
/*LDR R1,[R12,#boardlowlim]
   CMP R0,R1 ???
   LDR R1,[R12,#boardhighlim]
   CMP R0,R1*/

    bombcheck(r5);

    char r0 = *r5;

    if ((r0 == _gaslowlim) || (r0 == _gashighlim))
    {
        normalbomb(r5); return;
    }
    if ((r0 >= _bonuslow) && (r0 <= _bonushigh))
    {
        int x, y;
        backtranslate(r5, &x, &y);
        y += (8<<8);
        makeobj(_Dyingbonus, x, y+(7<<8), 0, 0, (*r5)|(1<<20), 666);
        *r5 = 0;
    }
   noshootbonus:
    if ((r0 >= _targetlowlim) && (r0 <= _targethighlim))
    {
        shoottarget(r5); return;
    }
   noshoottarget:
    if ((r0 >= _crumblelowlim) && (r0 <= _crumblehighlim))
        if (0 == (3&(++ (*r5)))) *r5 = 0;
   nocrumble:
    return;
}

void atomrocket(projent* r11, char* r0)
{
    char r1 = *r0; /* switched r1 and r0 from ARM */

    if (block_solid(r1))
    {
        causeexplo(r11); return;
    }
   atomdest:
    if (((r1 >= _eleclowlim) && (r1 <= _elechighlim))
        || ((r1 >= _targetlowlim) && (r1 <= _targethighlim)))
       elecatom:
        elecdestroy(r0);
   noelecatom2:
    if ((r1 >= _neuronlowlim) && (r1 <= _neuronhighlim))
    {
        causeexplo(r11); return;
    }
   notreasatom:
    *r0 = 0;
    causeexplo(r11);
}


int foundtarget(int x, int y, int dx, int dy)
{
    char target = *translate(x, y);
    if (!block_target(target)) return 0;
    y += 16<<8; // aim at body
    char* r0 = translate(x+(8<<8)+(dx>>4), y-(8<<8)+(dy>>4));
    if (*r0 > _targethighlim) return 1;
    target &= ~3;
    int r4 = random()&7;
    if (r4 == 7) r4 = 6;
    if (target >= _powertarget) r4 -= 6;
    if (target == _powertarget) r4 = 8+(r4&2);
    if (target == _nuttertarget) r4 = 12+(r4&3);
    makebul(x, y, dx, dy, r4, (1<<8)*BULL_TTL);
    return 1;
}

void shoottarget(char* r5)
{
    char r3 = *r5;

    *r5 = 0;
    addtoscore(targetscore);
    if (r3 >= _powertarget) elecdestroy(r5);

    int x, y;
    backtranslate(r5, &x, &y);

    explogo(x, y+(8<<8), 0, 0, 0, 0, 0 /* signal no existing object */);
    makescoreobj(x, y, 0xf60+((targetscore/100)<<8));
}

void elecdestroy(char* r5)
{
    eleccheck(r5-boardwidth);
    eleccheck(r5-1);
    eleccheck(r5+1);
    eleccheck(r5+boardwidth);
}

void eleccheck(char* r10)
{
    char r0 = *r10;

    if ((r0 < _eleclowlim) || (r0 > _elechighlim)) return;
    int r4 = (r0 >= _elecvertlowlim) ? boardwidth : 1;
    elecdelete(r4, r10);
    elecdelete(-r4, r10);
}

void elecdelete(int r4, char* r10)
{
    char r2 = _eleclowlim;

    for (; (r2 >= _eleclowlim) && (r2 <= _elechighlim); r2 = *(r10 += r4))
       loop78:
        *r10 = 0;
}

void deletetwin(char* r5)
{
    if ((*r5)&1) *(r5-1) = 0;
    else *(r5+1) = 0;
    *r5 = 0;
}

void screenwakeup(int xpos, int ypos)
{
    char* r7 = translate(xpos, ypos)-_boxwidth-(boardwidth<<4);
    char* r8 = boardadr->contents;                  //set up outer limits: start
    char* r9 = r8+boardadr->width*boardadr->height; // end

    for (int r5 = _boxheight*2; r5 > 0; r7 += boardwidth, r5--)
       loopa4:
        linecheck(r7, r8, r9);
}

void linecheck(char* r7, char* r8, char* r9)
{
    for (int r4 = _boxwidth*2; r4 > 0; r4--)
    {
       loopa3:
        if ((r7 > r8) && (r9 > r7) && (*r7 > _triggerlim))
            dowakeup(r7);
        r7++;
    }
}

void wakeupal(int xpos, int ypos)
{
    char* r7 = translate(xpos, ypos)-(_boxwidth/2)-(boardwidth<<3);
    char* r8 = boardadr->contents;                  //set up outer limits: start
    char* r9 = r8+boardadr->width*boardadr->height; // end

    boxcheck(_boxwidth, 1, &r7, r8, r9);
    boxcheck(_boxheight, boardwidth, &r7, r8, r9);
    boxcheck(_boxwidth, -1, &r7, r8, r9);
    boxcheck(_boxheight, -boardwidth, &r7, r8, r9);
}

void boxcheck(int r4, int r5, char** r7, char* r8, char* r9)
{
    for (; r4 > 0; r4--)
    {
       loop80:
        if ((*r7 > r8) && (r9 > *r7) && (**r7 > _triggerlim))
            dowakeup(*r7);
        *r7 += r5;
// badalmarker: *(*r7-r5)=0;
       wakeinsert:;
    }
}

void findplayer(int *initplx, int *initply)
{
    char* r5 = NULL;
    for (int r3 = boardadr->width*boardadr->height; r3 > 0; r3--)
    {
       loop60:
        if (*(boardadr->contents-1+r3) == _markerno)
        {
            *(boardadr->contents-1+r3) = 0; r5 = boardadr->contents-1+r3;
        }
    }
    int x, y;
    if (r5) backtranslate(r5, &x, &y);
    else x = y = 64<<8;
    *initplx = x; *initply = y-(1<<8);
}

char* normtelep(char* start, int dir, char find)
{
    char* r1;
    char* r2 = boardadr->contents;
    char* r3 = r2+boardadr->width*boardadr->height;
    for (r1 = start;; r1 += dir)
    {
       loop62:
        if (r1 <= r2) telepoffleft: return NULL;
        if (r1 >= r3) telepoffright: return NULL;
        if (*r1 == find) return r1;
    }
}

char* bonusfind()
{
    char* r11 = boardadr->contents;
    for (int bdx = boardadr->width*boardadr->height; bdx > 0; r11++, bdx--)
    {
        if (*r11 == _starsno) return r11;
    }
    return NULL;
}

void boardreg()
{
    boardwidth = boardadr->width;
    xposmax = boardwidth<<12;
    yposmax = boardadr->height<<12;
    boardlowlim = boardadr->contents;
    boardhighlim = boardlowlim+boardadr->width*boardadr->height;
}




void draw_block(fastspr_sprite* blockadr, int block, float x, float y, int layer)
{
    if (layer)
    {
	if ((block >= 8) && (block < 12)) mazescaleplot(blockadr, block, x, y);
	return;
    }
    if ((block == 0) || ((block >= 8) && (block < 12))) return; // was if (block==0)
    if ((block&~7) == _neuronlowlim) block = _neuronlowlim+((framectr&(7<<2))>>2);
    if (block <= _crumblestandhighlim)
    {
	if (block >= _crumblelowlim) block &= ~8; //alter for crumbles
	if (block_anim(block))
	{
	    int r1 = random()&7;
	    if ((block == _gaslowlim) || (block == _gashighlim)) block ^= r1>>2;
	    else block ^= r1>>1;
	}
    }
   noanimate:
    if ((block >= _weaplowlim+1) && (block <= _weaplowlim+6))
	block = _weaplowlim+1;
    mazescaleplot(blockadr, block, x, y);
}

void init_chunk_maze()
{
    CHUNK_FUELAIR = make_sound(_SampHiss, 0x6000, 0xff80, 0x2000ff00, 50);
    CHUNK_BONUS_1 = make_sound(_SampBonus, 0x2200, 0, 0, 5);
    CHUNK_BONUS_2 = make_sound(_SampBonus, 0x2000, 0, 0, 5);
}
