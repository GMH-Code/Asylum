/*  asylum.h */

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
#include <SDL/SDL_mixer.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PROJ_TTL (1<<16)
#define PROJ_ROCKET (1<<15)
#define PROJ_FIVEWAY (1<<14)
#define PROJ_SLOWSPLIT (1<<13)
#define PROJ_WEIRDSPLIT (1<<12)
#define ROCK_DIVIDE (1<<14)
#define ROCK_REDIVIDE (1<<13)
#define ROCK_BURST (1<<12)
#define PROJ_EXPLO (1<<11)
#define PROJ_SPLIT (1<<10)
#define PROJ_ATOM (1<<9)

#define BULL_TTL (1<<16)
#define BULL_EXPLO (1<<15)
#define BULL_ACCEL (1<<14)
#define BULL_HOME (1<<13)
#define BULL_SLOWHOME (1<<12)
#define BULL_SPLIT (1<<10)

#define xlowlim 0
#define ylowlim (15<<8)
#define _speedlim (15<<8)

#define _Playerchannel 0
#define _Explochannel 3
#define _Firechannel 1
#define _Sparechannel 2

#define _SampJump 1
#define _SampBonus 2
#define _SampExplo 3
#define _SampAtomExplo 4
#define _SampCannon 5
#define _SampRocket 6
#define _SampHiss 7
#define _SampStunned 8
#define _Sampsmallzap 9
#define _Sampbigzap 10
#define _Samporgan 16
#define _Samprave 19

// alien object types
#define _Explo (1)
#define _Ember (2)
#define _Platbase (3)
#define _Riseplat (_Platbase)
#define _Exploplat (_Platbase+1)
#define _Updownplat (_Platbase+2)
#define _Downplat (_Platbase+3)
#define _Fastplat (_Platbase+4)
#define _Fastplatstop (_Platbase+5)
#define _Fastplatexplo (_Platbase+6)
#define _Fastplatfire (_Platbase+7)
#define _Fallplat (_Platbase+8)
#define _Scoreobj (12)
#define _Dyingbonus (13)
#define _Flyingbonus (14)
#define _Booby (15)
#define _Decoration (16)
#define _Extender (17)
#define _Alien1 (18)

const int fullpitch = 0x2155;

typedef struct fastspr_sprite { int x; int y; int w; int h; GLuint t;
                                int texw; int texh; SDL_Surface* s; } fastspr_sprite;

typedef struct board { int first_int; int width; int height;
                       int fourth; int fifth; int sixth; int seventh; int eighth;
                       char contents[65536]; } board;

typedef struct alent
{
    int type; int x; int y; int dx;
    int dy; int r5; int r6;
    // transient flags for colcheck
    char colchecktype; char downpress; char falling; char pluphit;
    char lefthit; char righthit; char uphit; char downhit;
} alent;
typedef struct colchent
{
    alent* r0; int xmin; int ymin; int xmax; int ymax;
} colchent;
typedef struct bulcolchent
{
    alent* r0; int xmin; int ymin; int xmax; int ymax;
} bulcolchent;
typedef struct projent
{
    int type; int x; int y;
    int dx; int dy; int flags;
} projent;
typedef struct bulent
{
    int type; int x; int y;
    int dx; int dy; int flags;
} bulent;
typedef struct asylum_options
{
    char soundtype, soundquality, explospeed, gearchange;
    char fullscreen, opengl, size, scale, mentalzone;
    int leftkey, rightkey, upkey, downkey, firekey;
    char soundvol, musicvol, joyno;
    char idpermit;
    char initials[3];
} asylum_options;
typedef struct key_state
{
    char uppress, downpress, leftpress, rightpress, fire, keypressed;
} key_state;

void fspplot(fastspr_sprite*, char, int, int);
void mazescaleplot(fastspr_sprite*, char, float, float);
void relplot(fastspr_sprite*, char, int, int);
void cenplot(fastspr_sprite*, char, int, int);
void blokeplot(fastspr_sprite*, char, int, int);
void blokeplot_cen(fastspr_sprite*, char, int, int);
void plotdying(fastspr_sprite*, char, int, int, int);
void cenplotdying(fastspr_sprite*, char, int, int, int);

void init();
int abort_game();
int game();
void switchcolch();
void switchbank();
void showtext();
void texthandler(int do_animation);
void deathmessage();
void endgamemessage();
void alfire();
int foundtarget(int x, int y, int dx, int dy);
void bullets();  // the bullet handler (aliens fire these)
int makebul(int x, int y, int dx, int dy, int type, int flags);
void project();  // the projectile handler
void atomrocket(projent* r11, char* r0);
void projsplit(projent* r11);
void rocketsplit(projent* r11);
void rocketpair(projent* r11);
int makeproj(int x, int y, int dx, int dy, int type, int flags);
int foundmakeproj(projent* r10, int r8, int x, int y, int dx, int dy, int type, int flags);
int softmakeobj(int r0, int r1, int r2, int r3, int r4, int r5, int r6);
int makeobj(int r0, int r1, int r2, int r3, int r4, int r5, int r6);
int foundmakeal(alent* r10, int newalctr, int r0, int r1, int r2, int r3, int r4, int r5, int r6);
void seestars();
void seeifdead();
void plotscore();
void showscore(char plscore[8]);
void scoreadd();
void showstrength(int r3);
void update_show_strength();
void fuelairproc();
int block_gas(char b);
int block_weapon(char b);
void explogoquiet(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void explogomaybe(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void explogo(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void explocreate(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void explocreatequiet(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void embercreate(int r1, int r2, int r6);
void atomexplogo(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void screenwakeup(int xpos, int ypos);
void linecheck(char* r7, char* r8, char* r9);
void wakeupal(int xpos, int ypos);
void boxcheck(int r4, int r5, char** r7, char* r8, char* r9);
void dowakeup(char* r7);
void saveal();
void restoreal();
void save_player_state(uint8_t*);
void restore_player_state(uint8_t*);
void save_player(uint8_t*);
int restore_player(uint8_t*);
void save_alents(uint8_t*);
void restore_alents(uint8_t*);
void moval();
void procal(alent* r11);
void alienwander(alent* r11, char* r5);
void almightjumpins(alent* r11);
void jumpyalwander(alent* r11);
void alienwanderfly(alent* r11);
void alienwandernojump(alent* r11);
void alienstopped(alent* r11);
void alienstoppedfly(alent* r11);
void almightjump(alent* r11);
void alpossjump(alent* r11);
void almightwelljump(alent* r11);
void alientestplat(alent* r11, char* r5);
void decoration(alent* r11);
void extender(alent* r11);
void alien1(alent* r11);
void alien2(alent* r11);
void alien3(alent* r11);
void alien4(alent* r11);
void alien5(alent* r11);
void alien6(alent* r11);
int alspinfire(alent* r11);
int alspinpowerfire(alent* r11, int r7);
void alien7(alent* r11);
void alien8(alent* r11);
void alien9(alent* r11);
void alien10(alent* r11);
void alien11(alent* r11);
void alien12(alent* r11);
void alien13(alent* r11);
void alien14(alent* r11);
void hamsterspecial(alent* r11);
void alkill(alent* r11);
void alshoot(alent* r11);
void alshootfast(alent* r11);
void alshootnutter(alent* r11);
void alshootnutterplus(alent* r11);
void alshootnuttermental(alent* r11);
void alshootmental(alent* r11);
void alsleep(int s, alent* r11);
void explo(alent* r11);
void exploins(alent* r11);
void booby(alent* r11);
void ember(alent* r11);
void flyingbonus(alent* r11);
void dyingbonus(alent* r11);
void scoreobj(alent* r11);
void plat1(alent* r11);
void plat2(alent* r11);
void plat3(alent* r11);
void plat4(alent* r11);
void plat5(alent* r11);
void platins(alent* r11, char r9);
void colchadd(alent* r11);
void bulcolchadd(alent* r11);
void bulcolchaddshort(alent* r11);
void platland(alent* r11, char r9);
int embertrybomb(char* r0, alent* r11);
int embertrybombtarget(char* r0, alent* r11);
int embertrybooby(char* r0, alent* r11);
int embertrygas(char* r0, alent* r11);
void emberbooby(char* r0);
void emberbomb(char* r0, alent* r11);
void normbomb(char* r0, alent* r11);
void normbombsurvive(char* r0);
void fuelbomb(char* r0);
int plcolcheck(int x, int y, int dx, int dy);
void settestal();
void dvcheck(alent* r11);
void playerplot(int whendead);
void plotmpmg();
void plotmpmgblam();
void plotrocket();
void plotrocketblam();
void plotbonus(char bonusctr, int16_t bonusreplot);
void bonusplot();
void bonusbonus();
void bonusreset();
void foundmarker(char* r11);
void foundresetmarker(char* r11);
void normreset();
void zonecheatread(int* zone);
void cheatread();
void keyread(key_state* ks);
void plmove();
void windcheck();
int seeifwind(char* r1, int* dx, int* dy, int retval);
void plattoobjins(char* r0, int r4);
void plplattoobj(char* r0);
void crumblecheck(char* r1);
void telep();
char* normtelep(char* start, int dir, char find);
void playerfire();
void goblam();
void rocketblam();
void dofire();
void blamfire();
void firerocket();
void launchrocket();
void getarms();
void getrocket();
void getmpmg();
alent* bulcolcheck(int x, int y);
int projhital(alent* al, int loss);
void colcheck(alent* al, int colchecktype, int platypos);
void rise(alent* r6, alent* al, int colchecktype, int platypos);
void platonhead(alent* r6, alent* al, int colchecktype);
void platdestroy(alent* r6);
void platfire(alent* r6);
void platsurefire(alent* r6);
int headonroof(alent* r6, alent* al, int colchecktype, int platypos);
int alheadcheck();
char* albcheck(alent* r11);
void bcheck();
void noleft(alent* r11);
void noright(alent* r11);
int fallinggap(alent* re);
void nodown(alent* r11);
void nodownifplat(alent* r11);
void noup(alent* r11);
char* translate(int r0, int r1);
char* fntranslate(int r0, int r1);
void backtranslate(char* r, int* x, int* y);
void bonuscheck();
void weaponcheck(char* r5);
int plbombcheck(char* r5);
int bombcheck(char* r5);
int atombomb(char* r5);
void procatom(char* r5);
int fuelairbomb(char* r5);
int normalbomb(char* r5);
int boobybomb(char* r5);
void bonusobjgot(alent* r11);
void bonuslim(char* r5);
void deadbonuslim(char* r5);
void bonusgot(char* r5);
void bonuscommon(int bonus, int x, int y);
void sortbonus(char r0);
void bonusnumb(int r9);
char* bonusfind();
void bonus1();
void megabonus(char* r5);
void addtoscore(int sc);
void pllosestrength(int str);
void plsetneuronzone(int zone);
void makescoreobj(int x, int y, int type);
void electrocute(char* r5);
void destroy(char* r5);
void shoottarget(char* r5);
void elecdestroy(char* r5);
void eleccheck(char* r10);
void elecdelete(int r4, char* r10);
void deletetwin(char* r5);
void deletepoint();
void mazeplot(int xpos, int ypos);
void draw_block(fastspr_sprite* blockadr, int block, float x, float y, int layer);
void backdrop(int xpos, int ypos);
int escapehandler();
void loselife();
void rejoin();
void adjustopt();
void copyscreen();
void set_player_clip();
void writeclip();
void releaseclip();
void restartplayer();
void reinitplayer();
void redraw_bonus();
void completedzone();
void findplayer(int *initplx, int *initply);
void startplayer();
void showgamescreen();
void showlives();
void showlives(int lives);
void showchatscreen();
void showchatscores();
void clearkeybuf();
void setdefaults();
int options_menu(int gameon);
void dosaveconf();
void getzone();
void choosecontrol();
void choosekeys();
void choosestick();
void tunegame();
void tunesound();
void soundfillin();
void tunevolume();
void maketestsound(int r1);
void tunespeed();
int selectkey(int x, int y, int xv, int yv, const char* a);
int readopt(int maxopt);
int prelude();
void checkifarm3();
int checkifextend();
void permitid();
void dropprivs();
void loadconfig();
void saveconfig();
void loadgame();
void savegame();
int getfiles();
void getvitalfiles();
void getmusicfiles();
void getgamefiles();
void getlevelsprites();
int getlevelfiles();
int retrievebackdrop();
int getneuronfiles(int plzone);
int loadvitalfile(char** spaceptr,char* r1, char* path);
int loadhammered_game(char** spaceptr, char* r1, char* path);
int loadhammered_level(char** spaceptr, char* r1, char* path);
int loadhammered(char** spaceptr, char* r1, char* path);
int loadfile(char** r1, char* path, char* name);
void find_resources();
void open_scores();
void savescores(char* highscorearea, int mentalzone);
void loadscores(char* highscorearea, int mentalzone);
void setdefaultscores();
void fatalfile();
void showloading();
void filenotthere();
void filesyserror();
void badload();
int badlevelload();
void nomemory();
int filelength(char* name, char* path);
void showerror();
void showerrorok();
int errorwait();
void errorhandler();
void exithandler();
void loadzone();
void change_zone(int zone);
void enterneuron(int r1);
void exitneuron(int r1);
int showhighscore();
void updatehst();
int comparescore(char* r10);
void showhst();
void setup();
void wipesoundtab();
void wipetexttab();
void initprojtab();
void initbultab();
void initweapon();
void prepfueltab();
void scorezero();
void wipealtab();
void boardreg();
void backprep(char* backadr);
void prepstrength();
int gotallneurons();
int player_dead();
void screensave();
void getvars();
void init_palette();
void vduread(asylum_options);
int main(int argc, char** argv);
void load_voices();
void init_sounds();
void init_keyboard();
void message_scroll(const char* a);
void message(int x, int y, float xv, float yv, const char* a);

void startmessage();
void causeexplo(alent* r11);
void causeexplo(projent* r11);
void causeexplonopyro(alent* r11);
void causeexplonopyro(projent* r11);
void scorewipe();
void scorewiperead();
void explogonopyro(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void explogonopyroquiet(int r1, int r2, int r3, int r4, int r5, int r6, alent* r10);
void plattoexplo(alent* r11);
void deleteobj(alent* r11);
void blowup(alent* r11);
void plotarms();
void plattoobj(char* r0);
//SDL_Surface* decomp(char* r11);
void decomp(fastspr_sprite* DecompScreen, char* r11);
void soundupdate();
void losehandlers();
void initrockettab();
void initialize_music(int a);
void swi_bodgemusic_start(int a, int b);
void swi_bodgemusic_stop();
void swi_bodgemusic_volume(int v);
void swi_bodgemusic_load(int a, char* b);
void swi_sound_qtempo(int t);
void swi_sound_control(int c, int a, int p, int d);
int swi_sound_speaker(int s);
void swi_stasis_link(int a, int b);
void swi_stasis_control(int a, int b);
void swi_stasis_volslide(int a, int b, int c);
void swi_removecursors();
int osbyte_79(int c);
int osbyte_79_unicode(int c);
int osbyte_7a();
void osbyte_7c();
int osbyte_81(int c);
char swi_oscrc(int w, char* start, char* end, int bytes);
FILE* find_game(int op);
FILE* find_config(int op);
void swi_osgbpb(int n, FILE* f, char* start, char* end, int b);
int swi_osfile(int op, const char* name, char* start, char* end);
int swi_joystick_read(int a, int* x, int* y);
void swi_blitz_wait(int d);
void swi_blitz_screenflush();
int swi_blitz_hammerop(int op, char* name, char* path, char* space);
void swi_fastspr_clearwindow();
void swi_fastspr_setclipwindow(int x1, int y1, int x2, int y2);
int swi_readescapestate();
int readmousestate();
int swi_joystick_read(int a, int* x, int* y);
void initialize_chatscreen(char* data);
void initialize_gamescreen(char* data);
int initialize_sprites(char* start, fastspr_sprite* sprites, int max_sprites, char* end);

void dumpmusic(int argc, char** argv);
void update_keyboard();
void load_voice(int v, const char* filename);
Mix_Chunk* make_sound(char samp, int initpitch, int volslide, int pitchslide, char frames);
void soundclaim(int r0, char r1, char r2, int r3, int r4, int r5, char r6, int r7,
                Mix_Chunk* static_chunk);
void bidforsound(int r0, char r1, char r2, int r3, int r4, int r5, char r6, int r7,
                 Mix_Chunk* chunk);
void bidforsoundforce(int r0, char r1, char r2, int r3, int r4, int r5, char r6, int r7,
                      Mix_Chunk* chunk);
void soundclaimmaybe(int r0, char r1, char r2, int r3, int r4, int r5, char r6, int r7,
                 Mix_Chunk* chunk);
void soundclaimexplo(int r0, char r1, char r2, int r3, int r4, int r5, char r6, int r7,
                 Mix_Chunk* chunk);
void init_mulaw();
void init_audio();
uint32_t read_littleendian(uint8_t* word);
uint32_t read_littleendian(uint32_t* word);
void write_littleendian(uint8_t* bytes, uint32_t word);
void init_projsplittab();
void init_rockettab();
void init_rocketbursttab();
void init_alspintab();
void init_splittab();
void init_chunk_bullet();
void init_chunk_maze();
void init_chunk_alien();
void init_chunk_player();

int need_redraw();
