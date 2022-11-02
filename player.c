/*  player.c */

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

extern fastspr_sprite blokeadr[77], blockadr[256], alspradr[256], exploadr[32];

extern const char _translowlim = 54, _transhighlim = 63;
const char _weapmpmgblam = 102, _weaprocketblam = 111;
extern const char _blim = 112;
extern const char _platlowlim, _plathighlim;
const char _teleplowlim = 192, _telephighlim = 199;
extern const char _extendno = 152;
const char _shutdownno = 200;

#define _neuronstoget 8

#define _rockettablen 192

int* rockettabadr;
char falling;
char plframe, plface;
char plfired, plweapontype, plweaponspeed;
char pluphit, firerate;
char rocketflag, electrocuting, platuphit;
char blamctr, rocketblamctr, plotterofs, hiddenplatctr, extending;
char bonusctr;
int16_t bonusreplot;
int windctr;
int firelastframe;
int rocketctr;
int shutdownctr;
char atombombctr;
int laststrength;
int telepctr;
int telepxpos; int telepypos;
int bonusxpos; int bonusypos; int bonustimer;
int lagerctr, plstrength;
int neuronctr;
int snuffctr;
int lives;
int plscoreadd;
extern char plscore[8];
extern int boardwidth;
extern int framectr;
extern char frameinc;
extern int xpos, ypos, xposmax, yposmax;

key_state ks;
int initplx, initply, hvec, vvec;

const char _mpmgblamno = 8, _rocketblamno = 16;

#define _funnyfacesprbase 66
#define _scoresprbase 16
#define _skelspriteno 46
#define _starssprbase 58
#define _deadsprbase 70

#define _rockbase 9
#define _mpmgbase 1

#define _plheight (32<<8)
#define _plwidth (16<<8)
#define _strengthinit (108<<8)

Mix_Chunk* CHUNK_ELEC_1;
Mix_Chunk* CHUNK_ELEC_2;
Mix_Chunk* CHUNK_ELEC_3;
Mix_Chunk* CHUNK_JUMP;
Mix_Chunk* CHUNK_SHUTDOWN_1;
Mix_Chunk* CHUNK_SHUTDOWN_2;
Mix_Chunk* CHUNK_SHUTDOWN_3;
Mix_Chunk* CHUNK_TELEP_1;
Mix_Chunk* CHUNK_TELEP_2;
Mix_Chunk* CHUNK_TELEP_3;
Mix_Chunk* CHUNK_BLAM;
Mix_Chunk* CHUNK_FIRE;
Mix_Chunk* CHUNK_BLAMFIRE;
Mix_Chunk* CHUNK_ROCKET;
Mix_Chunk* CHUNK_STUNNED[17];
Mix_Chunk* CHUNK_WEAPON_1;
Mix_Chunk* CHUNK_WEAPON_2;
Mix_Chunk* CHUNK_WEAPON_3;
Mix_Chunk* CHUNK_WEAPON_4;
Mix_Chunk* CHUNK_OBJGOT[10];

char *pladr1, *pladr2, *pladr3, *pladr4, *pladr5, *pladr6, *pladr7, *pladr8;
int pllx, plly, plhx, plhy;

const int _savevalid = 0x4b4f6349;

void save_player_state(uint8_t store[24])
{
    write_littleendian(store, lives);
    write_littleendian(store+4, plstrength);
    write_littleendian(store+8, neuronctr);
    write_littleendian(store+12, shutdownctr);
    memcpy(store+16, plscore, 8);
}

void restore_player_state(uint8_t store[24])
{
    lives = read_littleendian(store);
    plstrength = read_littleendian(store+4);
    neuronctr = read_littleendian(store+8);
    shutdownctr = read_littleendian(store+12);
    memcpy(plscore, store+16, 8);
}

void save_player(uint8_t store[30])
{
    write_littleendian(store, _savevalid);
    write_littleendian(store+4, xpos); write_littleendian(store+8, ypos);
    write_littleendian(store+12, initplx); write_littleendian(store+16, initply);
    write_littleendian(store+20, hvec); write_littleendian(store+24, vvec);
    store[28] = bonusctr; store[29] = plweapontype;
}

int restore_player(uint8_t store[30])
{
    if (_savevalid != read_littleendian(store)) return 1;
    write_littleendian(store, 0);
    xpos = read_littleendian(store+4); ypos = read_littleendian(store+8);
    initplx = read_littleendian(store+12); initply = read_littleendian(store+16);
    hvec = read_littleendian(store+20); vvec = read_littleendian(store+24);
    bonusctr = store[28]; plweapontype = store[29];
    return 0;
}

void electrocute(char* r5)
{
    electrocuting = 1;
    int x, y;
    backtranslate(r5, &x, &y);
    explogonopyro(x, y+(16<<8), (random()&(0xfe<<2))-(0xfe<<1),
                  (random()&(0xfe<<2))-(0xfe<<1), 0, 0, 0);
}

void playerplot(int whendead)
{
    if ((!whendead) ^ (!snuffctr)) return;
    set_player_clip();
    if (snuffctr)
    {
       snuffhandler:
        snuffctr += frameinc;
        plotterofs = ((snuffctr-96 < 0) ? 0 : (snuffctr-96))>>1;
        blokeplot(blokeadr, 70, 0, 8);
// goto playerplotins;
        seestars();
        writeclip();
        return;
    }

    if (telepctr)
    {
        int telepinc = frameinc;
        if (telepinc > 2) telepinc = 2;
        if (telepctr == 31) telepinc = 1;
        telepctr += telepinc;
        if (telepctr > 64) telepctr = 0;
        if (telepctr == 32)
        {
           dotelep:
            xpos = telepxpos; ypos = telepypos;
            screenwakeup(xpos, ypos);
        }
    }

   telepskip:
    plotterofs = (telepctr > 32) ? (64-telepctr) : telepctr;
    plframe = (xpos>>10)&3;
    if (plframe == 2) plframe = 0;
    else if (plframe == 3) plframe = 2;
    if (plface == 1) plframe += 3;

    if (electrocuting)
    {
       plotskel:
        blokeplot(blokeadr, _skelspriteno+(plface == 1), 0, 8);
       plotskelins:
        seestars();
        writeclip();
        return;
    }

    blokeplot(blokeadr, (plface == 1) ? 1 : 0, 0, 8);
    blokeplot(blokeadr, plframe+6, 0, 19);

    int r0;
    if (plstrength < laststrength)
        r0 = _funnyfacesprbase+((framectr>>2)&3);
    else
       nofunnyface: if (plface == 1) r0 = 15; else r0 = 12;
   plotface:
    blokeplot(blokeadr, r0, 0, -2); //head

    if (plweapontype == 0) plotarms();
    else if (plweapontype == _mpmgblamno) plotmpmgblam();
    else if (plweapontype == _rocketblamno) plotrocketblam();
    else if (plweapontype < _rockbase) plotmpmg();
    else plotrocket();

   playerplotins:
    seestars();
    writeclip();
}

void plotarms()
{
    blokeplot(blokeadr, (plframe%3)+2, (plframe > 2), 9);
}

void plotmpmg()
{
    blokeplot(blokeadr, 42+(plface == 1), -6+(plface != 1)*12+(plfired ? 0 : 2*plface-1), 10);
    blokeplot(blokeadr, 44+(plface == 1), 0, 8);
    plfired = 0;
}

void plotmpmgblam()
{
    int r2;

    if (blamctr&8) r2 = (blamctr&15)-15;
    else r2 = blamctr&15;
    blokeplot(blockadr, _weapmpmgblam+(plface == 1),
              -6+(plface != 1)*12+(plfired ? plface*2-1 : 0), 11+(r2>>1));
    blokeplot(blokeadr, 44+(plface == 1), 0, 8+(r2>>1));
    plfired = 0;
}

void plotrocket()
{
    int r0 = rockettabadr[rocketctr];

    blokeplot(blokeadr, 50+(r0 > 170)+(r0 > 60)-(r0 < -60)-(r0 < -170),
              (plfired ? (plface*2-1) : 0), 1);
    blokeplot(blokeadr, 53, plface, 6);
    plfired = 0;
    if (rocketflag == 0) rocketctr = 0;
}

void plotrocketblam()
{
    int r2;

    if (blamctr&8) r2 = (blamctr&15)-15;
    else r2 = blamctr&15;
    blokeplot(blockadr, _weaprocketblam, (plfired ? 0 : 2*plface-1), (r2>>1));
    blokeplot(blokeadr, 53, 0, (r2>>2)+6);
    plfired = 0;
}

int rockettab[_rockettablen];

void initrockettab()
{
    rocketctr = 0; rocketflag = 0; rockettabadr = rockettab;
}

void init_rockettab()
{
    for (int i = 0; i < 184; i++)
        rockettab[4+i] = (int)(256*sin(i*2*M_PI/(192-8)));
    for (int i = 0; i < 4; i++)
        rockettab[i] = rockettab[188+i] = 0;
}

void seestars()
{
    if (plstrength >= laststrength)
    {
        laststrength = plstrength;
        return;
    }
    laststrength -= 1<<8;
    int r2, r1 = (framectr>>1)&7;
    int r0 = _starssprbase+r1;
    if ((r1&3) == 0) r2 = -6;
    else if (r1 > 4) r2 = -7;
    else r2 = -5;
    blokeplot(blokeadr, r0, 0, r2);

    int r4 = laststrength-plstrength;
    if (r4 == 0) return;
    if (electrocuting)
    {
       elecsound:
        bidforsound(_Playerchannel, _Samprave, 0x7f, 0x2000, (0xff)<<8, 0, 10, 0, CHUNK_ELEC_1);
        bidforsound(_Firechannel, _Samprave, 0x7f, 0x2400, (0xff)<<8, 0, 10, 127, CHUNK_ELEC_2);
        bidforsound(_Sparechannel, _Samprave, 0x7f, 0x2800, (0xff)<<8, 0, 10, -127, CHUNK_ELEC_3);
    }
    else bidforsound(_Playerchannel, _SampStunned, 0x70, 0x4000-((r4 > 0x1000) ? 0x1000 : r4),
                     0, 0, 50, 0, CHUNK_STUNNED[(r4 >= 0x1000) ? 16 : ((r4 < 0) ? 0 : (r4>>8))]);
}


void seeifdead()
{
    if ((plstrength > 0) || (snuffctr > 0)) return;
    snuffctr = 1;

    makeobj(_Decoration, 1<<8, (2<<8)-(12<<8), 0, -(2<<8), _deadsprbase-3+(250<<16), 24);
    makeobj(_Decoration, (1<<8)-(5<<8), 2<<8, -(1<<8), -(3<<8), _deadsprbase+1+(250<<16), 24);
    makeobj(_Decoration, (1<<8)+(6<<8), 2<<8, 1<<8, -(3<<8), _deadsprbase+2+(250<<16), 24);

    deathmessage();
    if (lives == 0) endgamemessage();
}

alent fraudalent;

void bcheck()
{
    fraudalent.uphit = fraudalent.downhit = 0;
    int ytemp;
//AND R6,R1,#15<<8
    fraudalent.x = xpos; ytemp = fraudalent.y = ypos;
    fraudalent.dx = hvec; fraudalent.dy = vvec;
    fraudalent.falling = falling;
    char* r11 = translate(fraudalent.x, fraudalent.y+fraudalent.dy);
// up
    if ((*(r11-boardwidth) >= _blim) || (*(r11-boardwidth+1) >= _blim))
        noup(&fraudalent);
//down

    if ((*(r11+boardwidth) >= _blim) || (*(r11+boardwidth+1) >= _blim))
        nodown(&fraudalent);
    if ((*(r11+boardwidth*2) >= _blim) || (*(r11+boardwidth*2+1) >= _blim))
        nodownifplat(&fraudalent);
//fallinggap(&fraudalent);
// CMP R6,#15<<8
// not sure how the original game manages without fallinggap's return value.
    int fg = fallinggap(&fraudalent);
    if ((!fg) && ((ytemp&(15<<8)) != (15<<8)))
    {
        r11 = translate(fraudalent.x+fraudalent.dx, fraudalent.y);
        char* r10 = translate(fraudalent.x+fraudalent.dx, fraudalent.y /*+(4<<8)*/); // 4<<8 was commented out
//left

        if ((*(r10-boardwidth) >= _blim) || (*(r11) >= _blim) || (*(r11+boardwidth) >= _blim))
            noleft(&fraudalent);
//right

        if ((*(r10-boardwidth+1) >= _blim) || (*(r11+1) >= _blim) || (*(r11+boardwidth+1) >= _blim))
            noright(&fraudalent);
    }
    else if (fg == -1)
    {
        r11 = translate(fraudalent.x+fraudalent.dx, fraudalent.y);
        if ((*(r11-boardwidth) >= _blim) || (*r11 >= _blim))
            noleft(&fraudalent);
//right
        if ((*(r11-boardwidth+1) >= _blim) || (*(r11+1) >= _blim))
            noright(&fraudalent);
    }
    else
    {
       shorT:
        r11 = translate(fraudalent.x+fraudalent.dx, fraudalent.y);
        if ((*(r11) >= _blim) || (*(r11+boardwidth) >= _blim))
            noleft(&fraudalent);
//right
        if ((*(r11+1) >= _blim) || (*(r11+1+boardwidth) >= _blim))
            noright(&fraudalent);
    }

   bcheckins:
    r11 = translate(fraudalent.x+fraudalent.dx, fraudalent.y+fraudalent.dy);
//up
    if ((*(r11-boardwidth) >= _blim) || (*(r11-boardwidth+1) >= _blim))
        noup(&fraudalent);
//down
    if ((*(r11+boardwidth) >= _blim) || (*(r11+boardwidth+1) >= _blim))
        nodown(&fraudalent);
    falling = fraudalent.falling;
    xpos = fraudalent.x; ypos = fraudalent.y;
    hvec = fraudalent.dx; vvec = fraudalent.dy;
}

void plmove()
{
    if (extending) extending--;
    if (shutdownctr != 0) shutdownctr++;
    if (shutdownctr == 150) change_zone(0);
    else if (shutdownctr != 0) deletepoint();
    keyread(&ks);
//if (fire) goto cheatmove;

    if (snuffctr != 0)
	ks.uppress = ks.downpress = ks.leftpress = ks.rightpress = ks.fire = 0;
    vvec -= vvec>>5;
    hvec -= hvec>>5;
    if (ks.leftpress != ks.rightpress)
	plface = (ks.leftpress > ks.rightpress) ? 1 : 0;

    int r2;
    if ((ks.leftpress < 4) && (ks.rightpress < 4)) r2 = 1<<8; else r2 = 2<<8;
    if (ks.leftpress > ks.rightpress) r2 = -r2;
    if (ks.leftpress == ks.rightpress) r2 = 0;

    if (falling <= 4) hvec = r2;
    else hvec += r2>>5;

   cheatins:
    if (ks.downpress >= 32) windctr = (1<<10);
    if (windctr) windcheck();
    if (hvec > _speedlim) hvec = _speedlim;
    if (hvec < -_speedlim) hvec = -_speedlim;
    if (vvec > _speedlim) vvec = _speedlim;
    if (vvec < -_speedlim) vvec = -_speedlim;
    fraudalent.x = xpos; fraudalent.y = ypos; fraudalent.dx = hvec; fraudalent.dy = vvec;
    fraudalent.falling = falling;
    fraudalent.pluphit = pluphit;
    fraudalent.downpress = ks.downpress;
    colcheck(&fraudalent, 0, ypos);
    falling = fraudalent.falling;
    xpos = fraudalent.x; ypos = fraudalent.y; hvec = fraudalent.dx; vvec = fraudalent.dy;
    //pluphit = platuphit; seems useless
//if ((!)fire)

    bcheck();
    pluphit = fraudalent.uphit;
    int dy = vvec;
    if (dy > (1<<12)) dy = 1<<12;
    if (dy < -(1<<12)) dy = -(1<<12);
    xpos += hvec; ypos += dy;
    if ((xpos <= xlowlim) || (xposmax <= xpos) || (ypos <= ylowlim) || (yposmax <= ypos))
        restartplayer();
    int x = xpos, y = ypos;

    if (!snuffctr)
    {
        y += (6<<8);
        vvec = dy;
        pllx = x-(_plwidth/2); plhx = x+(_plwidth/2);
        plly = y-(_plheight/2)+(plotterofs<<8);
        plhy = y+(_plheight/2);
        y = ypos-(6<<8); // - adjustment
    }
    else
    {
       fakecolch:
        plhx = plhy = 0;
        pllx = plly = 1;
    }
   fakeins:
    pladr1 = translate(x, y);
    pladr2 = pladr1+1;
    pladr3 = pladr1+boardwidth;
    pladr4 = pladr3+1;
    pladr5 = pladr1+2*boardwidth;
    pladr6 = pladr5+1;

    if (((falling == 0) || (falling == 0xff)) && (ks.uppress != 0))
    {
        if (ks.downpress > ks.uppress)
            vvec = -((0x100-(ks.downpress-ks.uppress))<<3);
        else vvec = -(19<<7);
        falling = 1;
        bidforsound(_Playerchannel, _SampJump, 0x5f, 0x4600, 0, 0, 4, 0, CHUNK_JUMP);
    }

    char* r11;
   jumpins:
    r11 = translate(xpos, ypos+(1<<8));
    pladr7 = r11+boardwidth;
    pladr8 = pladr7+1;
    if (falling >= 8)
        if (vvec >= (2<<8))
	{
            crumblecheck(pladr7);
            crumblecheck(pladr8);
        }
    if ((*pladr7 < _blim) && (*pladr8 < _blim))
    {
        if (++falling == 0xff) falling = 0xfe;
        if (falling)  // [else] standing on a platform
            if (falling > 1) if (vvec < (12<<8)) vvec += (8<<4);
    }
    else nofall:
        falling = 0;
   nofallins:;
   fallins:
    if ((*pladr8 >= _platlowlim+1) && (*pladr8 <= _plathighlim+1) && (1&*pladr8))
        pladr8[-1] = *pladr8-1;
   noplatrebuild:
    if ((*pladr7 >= _platlowlim) && (*pladr7 <= _plathighlim))
    {
        if (*pladr7 != _plathighlim)
        {
            hiddenplatctr = 0;
           nohidplat:
            if (!(1&*pladr7))
            {
                plplattoobj(pladr7); return;
            }
        }
        else
        {
            hiddenplatctr += frameinc;
            if ((hiddenplatctr >= 50) && !(1&*pladr7))
            { /* I added */
                hiddenplatctr = 0; plplattoobj(pladr7); return;
            }
        }
    }

   fallinscont:
    if ((!(1&*pladr7)) && (*pladr7 >= _teleplowlim) && (*pladr7 <= _telephighlim))
        telep();

   notelep:
    if ((*(pladr1-boardwidth) >= _platlowlim)
        && (*(pladr1-boardwidth) <= (_plathighlim-1)))  //not hidden platforms
        plplattoobj(pladr1-boardwidth);
    else notelepcont1 :
        if ((*(pladr1-boardwidth+1) >= _platlowlim)
            && (*(pladr1-boardwidth+1) <= (_plathighlim-1)))  //not hidden platforms
            plplattoobj(pladr1-boardwidth+1);
        else notelepcont2 :
            if (*pladr7 == _extendno+1)
            {
               doextendfromleft:
                if (!extending)
                    if (*(pladr7+1) == 0)
                    {
                        int x, y;
                        backtranslate(pladr7, &x, &y);
                        makeobj(_Extender, x-(8<<8), y+(15<<8), 1<<8, 0, _extendno, 0);
                        extending = 20;
                    }
            }
            else if (*pladr8 == _extendno+2)
            {
               doextendfromright:
                if (!extending)
                    if (*(pladr8-1) == 0)
                    {
                        int x, y;
                        backtranslate(pladr8, &x, &y);
                        makeobj(_Extender, x+(8<<8), y+(15<<8), -(1<<8), 0, _extendno, 0);
                        extending = 20;
                    }
            }
            else if (*pladr7 == _shutdownno)
            {
                *pladr7 = 14;
                shutdownctr = 1;
                neuronctr++;
		int soundvol = 0x7f;
                bidforsoundforce(3, _Samprave, 0x1f, (3<<8)|0x3000, (soundvol<<25)|0x1000,
                                 0x100ff00, 100, (3<<6)-96, CHUNK_SHUTDOWN_1);
                bidforsoundforce(2, _Samprave, 0x1f, (2<<8)|0x3000, (soundvol<<25)|0x1000,
                                 0x100ff00, 100, (2<<6)-96, CHUNK_SHUTDOWN_2);
                bidforsoundforce(1, _Samprave, 0x1f, (1<<8)|0x3000, (soundvol<<25)|0x1000,
                                 0x100ff00, 100, (1<<6)-96, CHUNK_SHUTDOWN_3);
            }
//plattoins:
    playerfire();
}

void windcheck()
{
    if (*pladr1 == 0) return;
    if (!seeifwind(pladr4, &hvec, &vvec,
		   seeifwind(pladr3, &hvec, &vvec,
			     seeifwind(pladr2, &hvec, &vvec,
				       seeifwind(pladr1, &hvec, &vvec, 0)))))
        windctr -= 64;
    if (--windctr < 0) windctr = 0;
}

void deletepoint()
{
    for (int r3 = 16; r3 > 0; r3--)
    {
       delloop:;
       dodeletepoint:;
        int x, y;
        x = xpos+(((random()&15)+(random()&7)-11)<<12);
        if (x < (1<<12)) return;
        y = ypos+(((random()&15)-7)<<12);
        if (y < (1<<12)) return;
        if (x > xposmax-(1<<12)) return;
        if (y > yposmax-(1<<12)) return;
        x &= ~0xfff;
        y &= ~0xfff;
        char* z = translate(x, y);
        if (*z == 0) return;
        *z = 0;
        makeobj(_Dyingbonus, x, y+(15<<8), 0, 0, r3|(1<<20), 666);
    }
}

void alfire()
{
    int x = xpos+(((random()&15)+(random()&7)-11)<<12);
    int y = ypos+(((random()&15)-7)<<12);

    if (x < (1<<12)) return;
    if (y < (1<<12)) return;
    if (x > xposmax-(1<<12)) return;
    if (y > yposmax-(1<<12)) return;
    x &= ~0xfff; y &= ~0xfff;
    if (foundtarget(x, y, (xpos-x)>>6, ((ypos+(12<<8))-y-(16<<8))>>6))
	return;
   nofoundtarget:
    if (foundtarget(x+(1<<12), y, (xpos-x-(1<<12))>>6, ((ypos+(12<<8))-y-(16<<8))>>6))
	return;
    return;
}

void makescoreobj(int x, int y, int type)
{
    makeobj(_Scoreobj, x, y, (random()&(0xfe<<1))-(0xfe)+(hvec>>2),
            -(random()&(0xfe<<2)), type, 666);
}


void foundmarker(char* r11)
{
    bonusxpos = xpos;
    bonusypos = ypos;
    foundresetmarker(r11);
}

void foundresetmarker(char* r11)
{
    *r11 = 0;
    backtranslate(r11, &telepxpos, &telepypos);
    telepypos -= (1<<8);
    telepctr = 1;
    bonustimer = 0x180;
}

void normreset()
{
    telepctr = 1;
    telepxpos = bonusxpos;
    telepypos = bonusypos;
    bonustimer = 0;
}

void dowakeup(char* r7)
{
    char r0 = *r7;
    int r1, r2;

    if ((r0 < 240) || (r0 > 253)) return;
// r7-=r5;  not for me
    *r7 = 0;
    backtranslate(r7, &r1, &r2);
    r0 = r0-(240-_Alien1);
    makeobj(r0, r1+(8<<8), r2+(7<<8), 0, 0, 0, (r0<<9)-((_Alien1-1)<<9));
}

void telep()
{
    initplx = xpos; initply = ypos;
    if (ks.downpress < 16) return;
    if (telepctr) return;

    char r4 = *pladr7;
    int r5 = (r4&2) ? 1 : -1;
    if ((ks.leftpress >= 64) && (ks.uppress == 0) && (ks.fire != 0)
	&& (ks.fire <= ks.downpress) && (ks.rightpress >= 64))
    {
        r5 = -r5;
        bidforsound(_Explochannel, _SampJump, 0x7f, 0x1000, 0, 0, 50, 0, CHUNK_TELEP_1);
    }
    ks.downpress = 0;
    char* r1 = normtelep(pladr7, r5, r4^2);
    if (r1 == NULL) return;
   telepfound:
    backtranslate(r1, &telepxpos, &telepypos);
    telepxpos += (8<<8);
    telepypos -= (17<<8);
    telepctr = 1;
    bidforsound(_Playerchannel, _Samporgan, 0x7f, 0x1000, 0xff00, 0x50000a00, 25, 127, CHUNK_TELEP_2);
    bidforsound(_Sparechannel, _Samporgan, 0x7f, 0x4000, 0xff00, 0x1000f400, 25, -127, CHUNK_TELEP_3);
}

void playerfire()
{
    if (snuffctr) return;
    if (blamctr) blamctr += 2;
    if (blamctr >= 224)
    {
        goblam(); return;
    }
    if (plweapontype == 0) return;
    if (ks.fire) dofire();
    else // I added this, can't find it in the source
    if (rocketflag) launchrocket();
}

void goblam()
{
    if (plweapontype == _rocketblamno)
    {
        rocketblam(); return;
    }
    for (int r9 = 32; r9 > 0; r9--)
    {
       loopa0:;
        int r3 = r9-16;
        int r4 = (r3 < 0) ? -r3 : r3;
        int r2, r0;
        if (plface == 1)
        {
            r0 = xpos-(20<<8); r2 = (r4<<5)-(4<<8);
        }
        else
        {
            r0 = xpos+(20<<8); r2 = (4<<8)-(r4<<5);
        }

        makeproj(r0, ypos+(12<<8), r2, r3<<6, 57, PROJ_ROCKET);
    }
    int r1, r3;
    if (plface == 1)
    {
        r1 = xpos-(24<<8); r3 = -(1<<8);
    }
    else
    {
        r1 = xpos+(24<<8); r3 = (1<<8);
    }
    explogonopyroquiet(r1, ypos+(4<<8), r3, 0, 0, 0 /*r6?*/, 0);
    plfired = 1;
    blamctr = 0;
    bidforsound(_Firechannel, _SampAtomExplo, 0x7f, fullpitch, 0, 0, 8, 0, CHUNK_BLAM);
}

void rocketblam()
{
    int r0, r1, r2, r3, r4;

    for (int r9 = 32; r9 > 0; r9--)
    {
       loopa1:
        r1 = ypos+(2<<8);
        r3 = *(rockettabadr+r9*6);
        if (plface == 1)
        {
            r0 = xpos-(12<<8); r4 = 1; r2 = -(4<<8); r3 = -r3;
        }
        else
        {
            r0 = xpos+(12<<8); r4 = 0; r2 = (4<<8);
        }
        r3 *= 2;

        if (r3 > 170) r4 += 2;
        if (r3 > 60) r4 += 2;
        if (r3 < -60) r4 -= 2;
        if (r3 < -170) r4 -= 2;
// ??? CMP R3,#1
        r4 += 36;
        makeproj(r0+r2*2, r1+r3*2, r2, r3, r4, PROJ_WEIRDSPLIT|PROJ_ATOM);
    }

    if (plface == 1)
    {
        r1 = xpos+(24<<8); r3 = (1<<9);
    }
    else
    {
        r1 = xpos-(24<<8); r3 = -(1<<9);
    }
    explogonopyroquiet(r1, ypos-(8<<8), r3, 0, 1, /*r6*/ 0, 0);

    plfired = 1;
    blamctr = 0;

    bidforsound(_Firechannel, _SampAtomExplo, 0x7f, fullpitch, 0, 0, 8, 0, CHUNK_BLAM);
    rocketblamctr -= 1;
    if ((rocketblamctr&(1<<7)) == 0) return;
    plweapontype = _rockbase;
    explogoquiet(xpos, ypos+(4<<8), 0, -(1<<7), 0, /*r6*/ 0, 0);
}

void dofire()
{
    if (plweapontype >= _rockbase)
    {
        firerocket(); return;
    }                                                // no return
   firempmg:
    if (plweapontype == _mpmgblamno)
    {
        blamfire(); return;
    }

    if (framectr-firelastframe < firerate) return;
    firelastframe = framectr;
    int r2 = (plface == 1) ? -(hvec>>1)-(plweaponspeed<<8) : (hvec>>1)+(plweaponspeed<<8);
    int r5;
    switch (plweapontype&7)
    {
    case 2: r5 = (4*PROJ_TTL)|PROJ_SPLIT; break;
    case 3: r5 = (4*PROJ_TTL)|PROJ_SPLIT|PROJ_FIVEWAY; break;
    case 4: r5 = (4*PROJ_TTL)|PROJ_SPLIT|PROJ_FIVEWAY|PROJ_SLOWSPLIT; break;
    case 5: r5 = (20*PROJ_TTL)|PROJ_SPLIT|PROJ_SLOWSPLIT|PROJ_WEIRDSPLIT; r2 >>= 2; break;
    case 6: r5 = (4*PROJ_TTL)|PROJ_SPLIT|PROJ_EXPLO; break;
    case 7: r5 = (4*PROJ_TTL)|PROJ_SPLIT|PROJ_FIVEWAY|PROJ_EXPLO; break;
    default: r5 = (64*PROJ_TTL);
    }
    makeproj(xpos+((plface == 1) ? -(14<<8) : (14<<8)), ypos+(12<<8),
             r2, (random()&0xff)-0x7f, (plface == 1) ? 31 : 30, r5);
    plfired = 1;
    bidforsound(_Firechannel, _SampCannon, 0x7e, fullpitch, 0, 0, 2, 0, CHUNK_FIRE);
}

void blamfire()
{
    if (blamctr != 0) return;
    if (plweapontype == 8) blamctr = 128;
    else blamctr = 1;
    bidforsound(_Firechannel, _Samprave, 0x7e, fullpitch, 0, (fullpitch<<17)+0x200, 40, 0, CHUNK_BLAMFIRE);
}

void firerocket()
{
    if (plweapontype == _rocketblamno)
    {
        blamfire(); return;
    }
    rocketflag = 1;
    int r2 = 2;
    if (plweapontype == _rockbase+6) r2 = 8;
    else if (plweapontype >= _rockbase+1) r2 = 4;
    if (plface == 1) rocketctr += r2;
    else rocketctr -= r2;
    if (rocketctr >= _rockettablen) rocketctr = 0;
    if (rocketctr < 0) rocketctr = _rockettablen-1;
}

void launchrocket()
{
    if (framectr-firelastframe < (int)firerate) return;
    firelastframe = framectr;
    int r0, r1, r2, r3, r4;
    if (plface == 1)
    {
        r0 = xpos-(12<<8); r2 = -(plweaponspeed<<8); r3 = -*(rockettabadr+rocketctr); r4 = 1;
    }
    else
    {
        r0 = xpos+(12<<8); r2 = plweaponspeed<<8; r3 = *(rockettabadr+rocketctr); r4 = 0;
    }
    r1 = ypos+(2<<8);
    if (r3 > 170) r4 += 2;
    if (r3 > 60) r4 += 2;
    if (r3 < -60) r4 -= 2;
    if (r3 < -170) r4 -= 2;
// ??? CMP R3,#1
    r4 += 36;
    int r6 = plweapontype-_rockbase;
    if (r6 >= 6) r6 = random()&7;
    int r5;
    switch (r6)
    {
    case 1: r5 = (8*PROJ_TTL)|PROJ_SPLIT|ROCK_DIVIDE;                                      // split
        break;
    case 2: r5 = (8*PROJ_TTL)|PROJ_SPLIT|ROCK_DIVIDE|ROCK_REDIVIDE;                        //split twice
        break;
    case 3: r5 = (20*PROJ_TTL)|PROJ_SPLIT|PROJ_ROCKET|ROCK_BURST;                          // burst a safe distance away
        break;
    case 4: r5 = (14*PROJ_TTL)|PROJ_SPLIT|PROJ_ROCKET|ROCK_DIVIDE|ROCK_BURST;              // dual burst
        break;
    case 5: r5 = (8*PROJ_TTL)|PROJ_SPLIT|PROJ_ROCKET|ROCK_DIVIDE|ROCK_REDIVIDE|ROCK_BURST; // quad burst
        /*(1<<18)*/
        break;
    default: r5 = (64*PROJ_TTL);
    }
    r5 |= PROJ_ROCKET;
    makeproj(r0+r2+r2, r1+r3+r3, r2, r3, r4, r5);
    plfired = 1;
    bidforsound(_Firechannel, _SampRocket, 0x7e, fullpitch, 0, 0, 2, 0, CHUNK_ROCKET);
    rocketflag = 0;
}

void getarms()
{
    blamctr = 0;
    plweapontype = 0;
}

void getrocket()
{
    plweapontype = _rockbase+(random()&7);
    plweaponspeed = 2;
    firerate = 12;
    blamctr = 0;
}

void getmpmg()
{
    plweapontype = _mpmgbase+(random()&7);
    plweaponspeed = 8;
    firerate = 4;
    blamctr = 0;
}

void prepstrength()
{
    laststrength = plstrength = _strengthinit;
    snuffctr = lagerctr = 0;
}

void loselife()
{
    lagerctr = 0;
    plstrength = 0; //no return
    rejoin();
}

void update_show_strength()
{
    if (lagerctr != 0)
    {
        if ((lagerctr -= frameinc) < 0) lagerctr = 0;
        plstrength += frameinc<<6;
        if (plstrength > _strengthinit) plstrength = _strengthinit;
        laststrength += frameinc<<6;
        if (laststrength > _strengthinit) laststrength = _strengthinit;
    }
    showstrength(plstrength);
}

void bonuscheck()
{
    if (snuffctr)
    {
       deadbonuscheck:
        atombombctr = 0;
        electrocuting = 0;
        deadbonuslim(pladr1);
        deadbonuslim(pladr2);
        deadbonuslim(pladr3);
        deadbonuslim(pladr4);
        return;
    }

    atombombctr = 0;
    electrocuting = 0;
    plstrength -= plbombcheck(pladr1);
    bonuslim(pladr1);
    weaponcheck(pladr1);
    plstrength -= plbombcheck(pladr2);
    bonuslim(pladr2);
    weaponcheck(pladr2);
    plstrength -= plbombcheck(pladr3);
    bonuslim(pladr3);
    weaponcheck(pladr3);
    plstrength -= plbombcheck(pladr4);
    bonuslim(pladr4);
    weaponcheck(pladr4);
}


void bonuscommon(int bonus, int x, int y)
{
    sortbonus(bonus);
    int score = (bonus+2)>>1;
    if (score > 10) score = 10;
    addtoscore(score*100);
    makescoreobj(x, y, 0xf60+(score<<8));
    bidforsound(_Playerchannel, _SampBonus, 0x6e, 0x3000+(score<<6), 0, 0, 3, 0, CHUNK_OBJGOT[score-1]);
}
    


void sortbonus(char r0)
{
    if ((r0 == 13) || (r0 == 14))
    {
       bonusstrength:
        lagerctr += (r0 == 14) ? 150 : 50;
        laststrength = plstrength;
    }

    if (r0 == bonusctr)
    {
       advancebonus:
        bonusctr++;
        //sortreplot:
        bonusreplot = 28;
        return;
    }

    if (r0 == 15)
    {
        bonusctr = 0;
        if (snuffctr)
            if (plstrength > 0)
            {
                prepstrength();
                message(48, 24, 0, 0.5, "A potion and Skull!");
                message(48, 176, 0, -0.5, "Rise from the dead!");
            }
       sortreplot:
        bonusreplot = 28;
    }
    return;
}


void bonusnumb(int r9)
{
    int r6 = -1;

    for (r9 &= 0xff; r9 > 0; r9 -= (r6+1))
    {
       loopc3:
        r6 = (r6+1)&3; // was random()&3
        makescoreobj(xpos, ypos, 0xf60+(10<<8)+(r6<<8));
    }
}


void initweapon()
{
    firelastframe = 0;
    if (plweapontype >= _rockbase)
    {
        plweaponspeed = 2; firerate = 12;
    }
    else
    {
        plweaponspeed = 8; firerate = 4;
    }
    rocketblamctr = 5;
}

int plcolcheck(int x, int y, int dx, int dy) // returns 0 for EQ
{
    return (x+(dx>>1) > pllx) && (plhx > x-(dx>>1))
           && (y+(dy>>1) > plly) && (plhy > y-(dy>>1));
}

void settestal()
{
    makeobj(_Alien1+13, xpos-(65<<8), ypos-(24<<8), 0, 0, 0x100020, 1<<10);
}


void weaponcheck(char* r5)
{
    int weap = block_weapon(*r5);
    if (!weap) return;
    plweapontype = weap;
    *r5 = 0;
    if (plweapontype >= _rockbase)
    {
        plweaponspeed = 2; firerate = 12;
    }
    else
    {
        plweaponspeed = 8; firerate = 4;
    }
    rocketblamctr = 5;

    int r2 = plweapontype-1;
    int r7, r0;
    if ((r2&7) == 7)
    {
        r7 = _scoresprbase+12; r0 = 4000;
    }
    else if (r2&4)
    {
        r7 = _scoresprbase+10; r0 = 2000;
    }
    else
    {
        r7 = _scoresprbase+9; r0 = 1000;
    }
    addtoscore(r0);

    makescoreobj(xpos, ypos, 60+(r7<<8));
    bidforsound(_Playerchannel, (r7 == _scoresprbase+12) ? _Samprave : _Samporgan,
                0x7f, 0x3000, 0xff00, (r7 == _scoresprbase+12) ? 0x60000800 : 0x60000200, 10, 127,
                (r7 == _scoresprbase+12) ? CHUNK_WEAPON_3 : CHUNK_WEAPON_1);
    bidforsound(_Sparechannel, (r7 == _scoresprbase+12) ? _Samprave : _Samporgan,
                0x7f, 0x3800, 0xff00, (r7 == _scoresprbase+12) ? 0x60000800 : 0x60000200, 10, -127,
                (r7 == _scoresprbase+12) ? CHUNK_WEAPON_4 : CHUNK_WEAPON_2);
/* XXX Original code also kills message so the same one is never redisplayed */
    switch ((plweapontype-1)&15)
    {
    case 0:  message_scroll("Standard Mini-Gun"); break;
    case 1:  message_scroll("Mini-Gun with spray"); break;
    case 2:  message_scroll("Five Stream Gun"); break;
    case 3:  message_scroll("Fast 5 way - zap those nasties!"); break;
    case 4:  message_scroll("A weird one!"); break;
    case 5:  message_scroll("Three Way Blitzer"); break;
    case 6:  message_scroll("Blitzing Five Way - Lets Party"); break;
    case 7:  message_scroll("MegaBlam 5000 Mini-Gun!!!!"); break;
    case 8:  message_scroll("Standard Rocket Launcher"); break;
    case 9:  message_scroll("Twin Rocket Launcher"); break;
    case 10: message_scroll("Quad Launcher - Open Fire!"); break;
    case 11: message_scroll("Launcher with starburst"); break;
    case 12: message_scroll("Twin rockets with starburst - let's see some fireworks!"); break;
    case 13: message_scroll("Quad Launcher with starburst! Arrgghhhh!!!!"); break;
    case 14: message_scroll("A Pot Pourri Rocket Launcher - mix and match!"); break;
    case 15: message_scroll("MegaBlam Rocket Launcher!!!!  Six Shots Only!"); break;
    }
}

void scoreadd()
{
    if (!plscoreadd) return;
    char* r10 = plscore;
    int placeval = 100000000;
    for (int r6 = 8; r6 > 0; r6--)
    {
	placeval /= 10;
       loop37:
        if (plscoreadd >= placeval)
        {
            char digit = 0;
            if (plscoreadd >= (placeval<<3))
            {
                digit += 8; plscoreadd -= (placeval<<3);
            }
            if (plscoreadd >= (placeval<<2))
            {
                digit += 4; plscoreadd -= (placeval<<2);
            }
            if (plscoreadd >= (placeval<<1))
            {
                digit += 2; plscoreadd -= (placeval<<1);
            }
            if (plscoreadd >= (placeval<<0))
            {
                digit += 1; plscoreadd -= (placeval<<0);
            }
            *r10 += digit;
        }
       scoreaddskip:
        r10++;
    }
    char carry = 0;
    for (int r6 = 8; r6 > 0; r6--) // now handle carries
    {
       loop38:
        r10--;
        *r10 += carry;
        if (*r10 >= 10)
        {
            *r10 -= 10; carry = 1;
        }
        else carry = 0;
    }
}

void plplattoobj(char* r0)
{
    plattoobjins(r0, vvec>>3);
    vvec = 0;
   plattoins:
    playerfire();
}

void plotscore()
{
    showscore(plscore);
}

void addtoscore(int sc)
{
    plscoreadd += sc;
}

void pllosestrength(int str)
{
    plstrength -= str;
}

void plsetneuronzone(int zone)
{
    initplx = xpos;
    initply = ypos;
    change_zone(zone);
}

void bonusplot()
{
    int r2 = (frameinc > 2) ? 2 : frameinc;

    if (bonustimer != 0) bonustimer -= r2;
    if ((bonustimer == 1) || (bonustimer == 2)) bonusreset();

    if (bonusreplot)
    {
        bonusreplot -= r2; //from above
        if (bonusreplot < 0) bonusreplot = 0;
    }
    else if (bonusctr >= 13) bonusbonus();
    plotbonus(bonusctr, bonusreplot);
}

void bonusbonus()    //not to be called by BL
{
    bonusctr = 8;
    bonusreplot = 28;
    ks.keypressed = 0;
    char* r11 = bonusfind();
    if (r11) foundmarker(r11);
    else bonus1();
}

void bonusreset()
{
    if (ks.keypressed)
    {
        normreset(); return;
    }
    bonusctr = 0;
    bonusreplot = 28;
    ks.keypressed = 1;
    char* r11 = bonusfind();
    if (r11) foundresetmarker(r11);
    else
    {
        bonus1(); normreset(); return;
    }
}

void restartplayer()
{
    xpos = initplx;
    ypos = initply;
    screenwakeup(xpos, ypos);
    reinitplayer();
}

void reinitplayer()
{
    telepctr = 33;
//bonusctr=0; (commented out in original)
    bonusreplot = 28;
    pladr1 = 0; windctr = 0; bonustimer = 0;
    rocketblamctr = 0; shutdownctr = 0;
    if (gotallneurons())
    {
	completedzone();
	lives = 0;
	snuffctr = 1;
    }
    return;
}

void redraw_bonus()
{
    bonusreplot = 4;
}

int gotallneurons()
{
    return (neuronctr >= _neuronstoget);
}

int player_dead()
{
    if (snuffctr >= 300)
    {
       playerdead:
	showgamescreen();
	if (lives == 0) return 2;
	lives--;
	if (lives > 9) lives = 9;
	prepstrength();
	return 1;
    }
    else return 0;
}

void showlives()
{
    showlives(lives);
}

void scorezero()
{
    plscoreadd = 0;
    *(uint64_t*)plscore = 0;
    lives = 2;
    neuronctr = 0;
}

void startplayer()
{
    bonusctr = 0;
    findplayer(&initplx, &initply);
}


void getvars()
{
    //getstrengthtab();
    xpos = 64<<8; ypos = 48<<8; initplx = 0; initply = 0;
    hvec = 0; vvec = 0; pladr1 = 0; pladr2 = 0;
    plface = 0;
}

void init_chunk_player()
{
    int soundvol = 0x7f;
    CHUNK_ELEC_1 = make_sound(_Samprave, 0x2000, 0xff00, 0, 10);
    CHUNK_ELEC_2 = make_sound(_Samprave, 0x2400, 0xff00, 0, 10);
    CHUNK_ELEC_3 = make_sound(_Samprave, 0x2800, 0xff00, 0, 10);
    CHUNK_JUMP = make_sound(_SampJump, 0x4600, 0, 0, 4);
    CHUNK_SHUTDOWN_1 = make_sound(_Samprave, 0x3300, (soundvol<<25)|0x1000, 0x100ff00, 100);
    CHUNK_SHUTDOWN_2 = make_sound(_Samprave, 0x3200, (soundvol<<25)|0x1000, 0x100ff00, 100);
    CHUNK_SHUTDOWN_3 = make_sound(_Samprave, 0x3100, (soundvol<<25)|0x1000, 0x100ff00, 100);
    CHUNK_TELEP_1 = make_sound(_SampJump, 0x1000, 0, 0, 50);
    CHUNK_TELEP_2 = make_sound(_Samporgan, 0x1000, 0xff00, 0x50000a00, 25);
    CHUNK_TELEP_3 = make_sound(_Samporgan, 0x4000, 0xff00, 0x1000f400, 25);
    CHUNK_BLAM = make_sound(_SampAtomExplo, fullpitch, 0, 0, 8);
    CHUNK_FIRE = make_sound(_SampCannon, fullpitch, 0, 0, 2);
    CHUNK_BLAMFIRE = make_sound(_Samprave, fullpitch, 0, (fullpitch<<17)+0x200, 40);
    CHUNK_ROCKET = make_sound(_SampRocket, fullpitch, 0, 0, 2);
    for (int i = 0; i < 17; i++) CHUNK_STUNNED[i] = make_sound(_SampStunned, 0x4000-i*0x100, 0, 0, 50);
    CHUNK_WEAPON_1 = make_sound(_Samporgan, 0x3000, 0xff00, 0x60000200, 10);
    CHUNK_WEAPON_2 = make_sound(_Samporgan, 0x3800, 0xff00, 0x60000200, 10);
    CHUNK_WEAPON_3 = make_sound(_Samprave, 0x3000, 0xff00, 0x60000800, 10);
    CHUNK_WEAPON_4 = make_sound(_Samprave, 0x3800, 0xff00, 0x60000800, 10);
    for (int i = 0; i < 10; i++) CHUNK_OBJGOT[i] = make_sound(_SampBonus, ((i+1)<<6)+0x3000, 0, 0, 3);
}

void blokeplot(fastspr_sprite* sprites, char n, int x, int y)
{
    blokeplot_cen(sprites, n, x, y+plotterofs);
}

void relplot(fastspr_sprite* sprites, char n, int x, int y)
{
    cenplot(sprites, n, (x>>8)-(xpos>>8), (y>>8)-(ypos>>8));
}

void plotdying(fastspr_sprite* sprites, char n, int x, int y, int r5)
{
    cenplotdying(sprites, n, (x>>8)-(xpos>>8), (y>>8)-(ypos>>8), r5);
}
