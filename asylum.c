/*  asylum.c */

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

#include "asylum_os.h"
#include "asylum.h"

#define _firstzone 0

#define _soundentlen 16


char bank;
char masterplotal;
char frameinc = 1;
char rate50;
char cheatpermit;
char charsok, arm3, savestart;
asylum_options options;

board *boardadr;
fastspr_sprite blokeadr[77];
fastspr_sprite blockadr[256];
char* backadr;
int framectr;
board *brainadr;
board *neuronadr;
int fspareat;
fastspr_sprite exploadr[32];
fastspr_sprite charsadr[48];
fastspr_sprite alspradr[256];
int currentzone;

int xpos, ypos;
char plscore[8];
int plzone;

void init()
{
// SWI "FastSpr_GetAddress";
// set up fspplot, fspvars, fspareat=fspvars+24
    setdefaults();
    loadconfig();
    vduread(options);
    swi_removecursors();
    bank = 1;
    switchbank(); //set up bank variables
    switchbank(); //set up bank variables
    checkifarm3();
    if (getfiles()) abort_game();
    //vduread(options); // set screen size from options

    scorezero();
    cheatpermit = prelude();
    if (cheatpermit == 2)  abort_game();
    for ( ; ; )
    {
       playloop:
        osbyte_7c();
        if (options_menu(0)) // not in game
        {
            abort_game(); return;
        }
	currentzone = 0;
        if (getlevelfiles())
        {
            notgotlevel: if (1) abort_game();
            // or, depending on what getlevelfiles() returned
            continue;
        }

        swi_bodgemusic_stop();
        if (game()) continue;
        if (gotallneurons())
        {
           zonedone:
            if (options.idpermit != 1) permitid();
            swi_bodgemusic_start(1, 0); // ?? (3,0) in original
        }
        else                            // was "else if overflow clear"
        {
            if (options.soundtype == 2) swi_bodgemusic_start(2, 0);
            swi_sound_qtempo(0x980);
            swi_bodgemusic_volume(options.musicvol);
        }
        showhighscore();
    }
}

int abort_game()
{
    swi_bodgemusic_stop();
    losehandlers();
    osbyte_7c();
    SDL_Quit();
    exit(0);
}

int game()
{
    wipetexttab();
    setup();
    switchcolch(); //setup colch vars
    startmessage();
    startplayer();
    getarms();
    if (cheatpermit == 1)  zonecheatread(&plzone);
    do
    {
       zonerestart:
        if (options.mentalzone == 4)  loadgame();
        else  restartplayer();
        do
        {
           mainrestart:
            showgamescreen();
            if (options.soundtype == 2) swi_bodgemusic_start((plzone != 0), 0);
            swi_bodgemusic_volume(options.musicvol);
            frameinc = 1;
            rate50 = 1;
            swi_blitz_wait(1);
            while (!swi_readescapestate())
            {
                mainloop:
                if (plzone != currentzone)
                {
                    switchzone: loadzone(); goto zonerestart;
                }
//BL saveal
//BL restoreal
                if ((char)rate50 != 1)
                {
                    plmove();
                    bonuscheck();
                    fuelairproc();
                    switchcolch();
                    masterplotal = 0;
                    moval();
                    project();
                    bullets();
                    alfire();
                    wakeupal(xpos, ypos);
                }
               rate50link:
                plmove();
                bonuscheck();
                fuelairproc();
                mazeplot(xpos, ypos);
                switchcolch();
                masterplotal = 1;
                playerplot(true);
                moval();
                project();
                bullets();
                alfire();
                playerplot(false);
                bonusplot();
                scoreadd();
                update_show_strength();
                texthandler(1);
                seeifdead();
//makesounds();
                wakeupal(xpos, ypos);
                if (cheatpermit == 1) cheatread();
                scorewipe();
                plotscore();
                frameinc = ((options.gearchange == 0) ? 2 : 1);
                swi_blitz_wait(frameinc);
                if ((rate50 != 1) && (frameinc < 2)) //rate 25 but one frame passed
                {
                    swi_blitz_wait(1);
                    frameinc = 2;
                    rate50 = 1;
                }
                else if (frameinc > 1) rate50 = 0;
               rateskip:

                framectr += frameinc;

                switchbank();
                //swi_blitz_smallretrieve();
                switch (player_dead())
                {
		   case 1:
                    goto zonerestart;
		   case 2:
		    return 0;  
		   case 0:
		    ;
                }
            }
            swi_bodgemusic_stop();
            redraw_bonus();
            if (escapehandler()) return 1;
	    backprep(backadr);
        }
        while (1);
    }
    while (1);
}

void showtext()
{
    texthandler(0);
    switchbank();
}

uint8_t store_for_neuron[30+78*28];
uint8_t store_for_savegame[30+78*28];
uint8_t store_player_state[25];

void saveal(uint8_t store[30+78*28])
{
    save_player(store);
    save_alents(store+30);
}

void restoreal(uint8_t store[30+78*28])
{
    if (restore_player(store)) return;
    wipealtab();
    restore_alents(store+30);
   restored:;
}

void bonus1()
{
    bonusnumb(10); message(96, 224, 0, -2, "Bonus 10000"); addtoscore(10000);
}

const int keydefs[] =
{ -SDLK_z, -SDLK_x, -SDLK_SEMICOLON, -SDLK_PERIOD, -SDLK_RETURN };

void setdefaults()
{
    checkifarm3();
    options.soundtype = 2;
    options.soundquality = (arm3 == 0) ? 0 : 1;
    options.soundvol = 0x7f;
    options.musicvol = 0x7f;
    options.leftkey = keydefs[0];
    options.rightkey = keydefs[1];
    options.upkey = keydefs[2];
    options.downkey = keydefs[3];
    options.firekey = keydefs[4];
    options.gearchange = (arm3 == 0) ? 0 : 1;
    options.explospeed = (arm3 == 0) ? 2 : 1;
    options.fullscreen = 0;
    options.opengl = 1;
    options.size = 1; // 640 x 512
    options.scale = 1;
    options.joyno = 0;
    options.mentalzone = 1;
    options.initials[0] = 'P';
    options.initials[1] = 'S';
    options.initials[2] = 'Y';
}

void soundupdate()
{
    return;
}
/*
   .soundupdate
   STMFD R13!,{R14}
   LDRB R5,[R12,#options.soundtype]
   LDRB R6,[R12,#options.soundquality]
   CMP R5,#0
   BEQ soundkill
   MOV R0,#2
   SWI "XSound_Enable"
   MOV R0,#129
   MOV R1,#0
   MOV R2,#&FF
   SWI "OS_Byte"
   MOV R8,R1

   MOV R0,#8; channels
   CMP R5,#1
   MOVEQ R0,#4
   MOV R1,#208
   MOV R2,#96
   TST R6,#1
   MOVNE R2,#48
   CMP R8,#&A3
   MOVLT R2,#0; RISC OS 2 bug

   MOV R3,#0
   MOV R4,#0
   SWI "XSound_Configure"
   ADR R0,sptunenorm
   TST R6,#1
   ADRNE R0,sptunehq
   CMP R8,#&A3
   ADRLT R0,sptunehq
   SWI "XOS_CLI"
   LDRB R0,[R12,#options.soundquality]
   TST R0,#1
   MOV R0,#6
   MOVNE R0,#12
   SWI "XSound_QBeat"
   MOV R0,#&1000
   SWI "XSound_QTempo"
   LDRB R0,[R12,#options.musicvol]
   SWI "XBodgeMusic_Volume"
   LDRB R0,[R12,#options.soundquality]
   MOV R0,R0,LSR #1
   MOV R1,#1
   SWI "XStasis_Control"
   LDMFD R13!,{PC}
   .sptunenorm
   EQUS "StasisTune &8000"
   EQUB 0
   .sptunehq
   EQUS "StasisTune &4000"
   EQUB 0
   ALIGN
   .soundkill
   MOV R0,#1
   SWI "XSound_Enable"
   LDMFD R13!,{PC}
 */
// buf:

void checkifarm3()
{
// The ARM3 is 25-33MHz with a 4Kb cache.  If you have
// less than that you should probably clear this flag.
    arm3 = 1;
}

int checkifextend()
{
    return (NULL != find_game(0x40));
}


char buffer[256];

//.hta
void errorhandler()
{
    losehandlers();
    exit(printf("Error from Asylum:\n%s", buffer+4));
}

void exithandler()
{
    losehandlers();
    exit(0);
}

void losehandlers()
{
    SDL_Quit(); return;
}

void loadzone()
{
    int r1 = currentzone;

    if ((currentzone = plzone) == 0) exitneuron(r1);
    else enterneuron(r1);
}

void change_zone(int zone)
{
    plzone = zone; 
}

void enterneuron(int r1)
{
    if (r1 == 0) saveal(store_for_neuron);
    currentzone = plzone = getneuronfiles(plzone);
    if (!currentzone)
    {
        exitneuron(r1); return;
    }
    wipealtab();
    getarms();
    initweapon();
    initprojtab();
    initbultab();
    backprep(backadr);
    boardreg();
    prepfueltab();
    startplayer();
}

void exitneuron(int r1)
{
    boardadr = brainadr;
    restoreal(store_for_neuron);
    initweapon();
    initprojtab();
    initbultab();
    retrievebackdrop();
    backprep(backadr);
    boardreg();
    prepfueltab();
    wipesoundtab();
}


void setup()
{
    framectr = 0;
    plzone = _firstzone;
    wipesoundtab();
    initweapon();
    initprojtab();
    initbultab();
    initrockettab();
    getvars();
    prepstrength();
    scorezero();
    backprep(backadr);
    boardreg();
    wipealtab();
    prepfueltab();
}

void wipesoundtab()
{
//r10=&soundtabofs; temporarily undefined
    for (int r3 = _soundentlen*8; r3 > 0; r3 -= sizeof(int))
       loop51:;
        //*(r10++)=0;
    for (int r0 = 7; r0 >= 0; r0--)
       soundkillloop:
        swi_stasis_volslide(r0, 0xfc00, 0);
}

void screensave()
{
    plotscore();
//oscli("Mount");
//oscli("Screensave Screenfile");
}

void c_array_initializers()
{
    init_projsplittab(); init_rocketbursttab(); init_alspintab(); init_rockettab();
    init_palette(); init_splittab();
    load_voices();
    init_keyboard();
}

int main(int argc, char** argv)
{
    find_resources();

    if ((argc > 2) && !strcmp(argv[1], "--dumpmusic"))
    {
	dropprivs();
        load_voices();
        dumpmusic(argc,argv);
        exit(0);
    }
    
    open_scores();
    dropprivs();

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    SDL_WM_SetCaption("Asylum", "Asylum");
    SDL_EnableUNICODE(1);
#ifndef _NO_SOUND
    init_audio();
#endif
    c_array_initializers();
    swi_stasis_control(8, 8);
    init(); // while (snuffctr>=300);
    SDL_Quit();
    exit(0);
    return 0;
}



char gamescreenpath[] = "GameScreen";
char chatscreenpath[] = "ChatScreen";
char blokepath[] = "FSPBloke";
char boardpath[] = "Brain";
char blockpath[] = "FSPBlocks";
char backpath[] = "Backfile";
char neuronbackpath[] = "Neurons/Backfile";
char neuronpath[] = "Neurons/Cell1";
char* neuronnumber = neuronpath+12;
char explopath[] = "FSPExplo";
char charpath[] = "FSPChars";
char alienpath[] = "FSPAliens";
char resourcepath[] = "./Resources/";
char extendpath[] = "./Extend/";
char idpath[] = "./Id/";
char psychepath[] = "./Psyche/";
char egopath[] = "./Ego/";
char egomusic1path[] = "./Ego/Music1";
char egomusic2path[] = "./Ego/Music2";
char psychemusic1path[] = "./Psyche/Music1";
char psychemusic2path[] = "./Psyche/Music2";
char idmusic1path[] = "./Id/Music1";
char idmusic2path[] = "./Id/Music2";
char mainmusicpath[] = "./Resources/Music1";
char deathmusicpath[] = "./Resources/Music2";

int getfiles()
{
    getvitalfiles();
    showloading();
    init_sounds();
    getmusicfiles();
    swi_bodgemusic_start(1, 0);
    getgamefiles();
    return 0;
}

void getvitalfiles()
{
    charsok = 0;
    char *chatscreenadr, *charsadr_load;
    loadvitalfile(&chatscreenadr, chatscreenpath, resourcepath);
    int charslen = loadvitalfile(&charsadr_load, charpath, resourcepath);
    initialize_sprites(charsadr_load, charsadr, 48, charsadr_load+charslen);
    initialize_chatscreen(chatscreenadr);
    charsok = 1;
}

void getmusicfiles()
{
    swi_bodgemusic_load(1, mainmusicpath);
    swi_bodgemusic_load(2, deathmusicpath);
}

char* currentpath;

void getgamefiles()
{
    char *gamescreenadr, *blokeadr_load, *exploadr_load;

   load1:
    loadhammered_game(&gamescreenadr, gamescreenpath, resourcepath);
    initialize_gamescreen(gamescreenadr);
   load2:
    int blokelen = loadhammered_game(&blokeadr_load, blokepath, resourcepath);
    initialize_sprites(blokeadr_load, blokeadr, 77, blokeadr_load+blokelen);
   load3:
    int explolen = loadhammered_game(&exploadr_load, explopath, resourcepath);
    initialize_sprites(exploadr_load, exploadr, 32, exploadr_load+explolen);
}

void getlevelsprites()
{
    char *blockadr_load, *alienadr_load;
    switch (options.mentalzone)
    {
    case 2: currentpath = psychepath; break;
    case 3: currentpath = idpath; break;
    case 4: currentpath = psychepath /*XXX*/; break;
    default: currentpath = egopath;
    }
   load4:
    int blocklen = loadhammered_level(&blockadr_load, blockpath, currentpath);
    initialize_sprites(blockadr_load, blockadr, 256, blockadr_load+blocklen);

   load5:
    int alienlen = loadhammered_level(&alienadr_load, alienpath, currentpath);
    initialize_sprites(alienadr_load, alspradr, 256, alienadr_load+alienlen);
}

int getlevelfiles()
{
    showgamescreen();
    switch (options.mentalzone)
    {
    case 2: currentpath = psychepath; break;
    case 3: currentpath = idpath; break;
    case 4: currentpath = psychepath /*XXX*/; break;
    default: currentpath = egopath;
    }
    getlevelsprites();

   load6:
    loadhammered_level((char**)&brainadr, boardpath, currentpath);
    boardadr = brainadr;
// hack: fix endianness
    boardadr->width = read_littleendian((uint32_t*)&boardadr->width);
    boardadr->height = read_littleendian((uint32_t*)&boardadr->height);

    loadhammered_level(&backadr, backpath, currentpath);

    char* r1;
    switch (options.mentalzone)
    {
    case 2: r1 = psychemusic1path; break;
    case 3: r1 = idmusic1path; break;
    default: r1 = egomusic1path;
    }

    swi_bodgemusic_load(0, r1);

    switch (options.mentalzone)
    {
    case 2: r1 = psychemusic2path; break;
    case 3: r1 = idmusic2path; break;
    default: r1 = egomusic2path;
    }
    swi_bodgemusic_load(1, r1);
    return 0;
}

int retrievebackdrop()
{
    char* r9 = currentpath;

   load10:
    loadhammered_level(&backadr, backpath, currentpath);
    return 0;
}

int getneuronfiles(int plzone)
{

//STR R10,[R12,#backadr]
   load8:
    loadhammered_level(&backadr, neuronbackpath, currentpath);
    while (1)
    {
       neuronloadloop:
        *neuronnumber = '0'+plzone;
        if (filelength(neuronpath, currentpath)) break;
        if (--plzone == 0) noneuronshere: return 0;
    }
   load9:
    loadhammered_level((char**)&neuronadr, neuronpath, currentpath);
    boardadr = neuronadr;
// hack: fix endianness
    boardadr->width = read_littleendian((uint32_t*)&boardadr->width);
    boardadr->height = read_littleendian((uint32_t*)&boardadr->height);
    return plzone;
}

char config_keywords[16][12] =
{ "LeftKeysym",    "RightKeysym", "UpKeysym",   "DownKeysym", "FireKeysym",
  "SoundType",   "SoundQ",      "FullScreen",
  "OpenGL", "ScreenSize", "ScreenScale",
  "SoundVolume", "MusicVolume", "MentalZone", "Initials",   "You" };

char idpermitstring[] = "You are now permitted to play the ID!!!\n";

void loadconfig()
{
    char keyword[12];

    FILE* r0 = find_config(0x40); // read access
    if (r0 != NULL)
    {
        //swi_osgbpb(3, /* read bytes from pointer R4 */
        //r0,&savestart,&saveend,0);
        while (fscanf(r0, " %12s", keyword) != EOF)
        {
            int i;
            for (i = 0; i < 16; i++)
                if (!strncmp(keyword, config_keywords[i], 12)) break;
            if (i == 14)
            {
                fscanf(r0, " %3c", options.initials); continue;
            }
            if (i == 15)
            {
                options.idpermit = 1; break;
            }                       // end of file
            if (i == 16) break;     // parsing failed
            int temp;
            fscanf(r0, " %i", &temp);
            switch (i)
            {
            case 0: options.leftkey = -temp; break;
            case 1: options.rightkey = -temp; break;
            case 2: options.upkey = -temp; break;
            case 3: options.downkey = -temp; break;
            case 4: options.firekey = -temp; break;
            case 5: options.soundtype = temp; break;
                //case 6: options.soundquality=temp; break;
            case 7: options.fullscreen = temp; break;
            case 8: options.opengl = temp; break;
            case 9: options.size = temp; break;
            case 10: options.scale = temp; break;
            case 11: options.soundvol = temp; break;
            case 12: options.musicvol = temp; break;
            case 13: options.mentalzone = temp; break;
            }
        }
        fclose(r0);
    }
   findoutid:;
    if (!options.idpermit) if (options.mentalzone > 2) options.mentalzone = 2;
    //int idp=swi_osfile(5,options.idpermitpath,NULL,NULL);
    //options.idpermit=(idp==1)?1:0;
}

void saveconfig()
{
    FILE* r0 = find_config(0x80); // create file with read/write access
    if (r0 == NULL) return;
//swi_osgbpb(1, /* write bytes to pointer R4 */
//r0,&savestart,&saveend,0);
    fprintf(r0, "%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %i\n%s %c%c%c\n%s",
            config_keywords[0], -options.leftkey,
            config_keywords[1], -options.rightkey,
            config_keywords[2], -options.upkey,
            config_keywords[3], -options.downkey,
            config_keywords[4], -options.firekey,
            config_keywords[5], options.soundtype,
            //config_keywords[6], options.soundquality,
            config_keywords[7], options.fullscreen,
            config_keywords[8], options.opengl,
            config_keywords[9], options.size,
            config_keywords[10], options.scale,
            config_keywords[11], options.soundvol,
            config_keywords[12], options.musicvol,
            config_keywords[13], options.mentalzone,
            config_keywords[14], options.initials[0],
	                         options.initials[1],
	                         options.initials[2],
            ((options.idpermit == 1) ? idpermitstring : ""));
    fclose(r0);
}

void loadgame()
{
    FILE* r0 = find_game(0x40);
    if (r0 == NULL) /* XXX failing silently is bad */ return;
    //uint8_t dimensions[8];
    fread(&options.mentalzone, 1, 1, r0);
    fread(&plzone, 1, 1, r0);  currentzone = plzone;
    fread(store_player_state, 1, 25, r0);
    restore_player_state(store_player_state);
    getlevelfiles();
    fread(store_for_savegame, 1, 30+78*28, r0);
    fread(store_for_neuron, 1, 30+78*28, r0);
    restoreal(store_for_savegame);
    //fread(dimensions, 8, 1, r0);
    //brainadr->width = read_littleendian(dimensions);
    //brainadr->height = read_littleendian(dimensions+4);
    fread(brainadr->contents, brainadr->width, brainadr->height, r0);
    if (plzone)
    {
        getneuronfiles(plzone); /* HACK determine dimensions and allocate neuronadr */
        //neuronadr->width = read_littleendian(dimensions);
        //neuronadr->height = read_littleendian(dimensions+4);
        fread(neuronadr->contents, neuronadr->width, neuronadr->height, r0);
    }
    fclose(r0);
    backprep(backadr);
    boardreg();
    reinitplayer();
}

void savegame()
{
    FILE* r0 = find_game(0x80);
    if (r0 == NULL) /* XXX failing silently is bad */ return;
    //uint8_t dimensions[8];
    fwrite(&options.mentalzone, 1, 1, r0);
    fwrite(&plzone, 1, 1, r0);
    save_player_state(store_player_state);
    fwrite(store_player_state, 1, 25, r0);
    saveal(store_for_savegame);
    fwrite(store_for_savegame, 1, 30+78*28, r0);
    fwrite(store_for_neuron, 1, 30+78*28, r0);
    //write_littleendian(dimensions, brainadr->width);
    //write_littleendian(dimensions+4, brainadr->height);
    //fwrite(dimensions, 8, 1, r0);
    fwrite(brainadr->contents, brainadr->width, brainadr->height, r0);
    if (plzone)
    {
        //write_littleendian(dimensions, neuronadr->width);
        //write_littleendian(dimensions+4, neuronadr->height);
        fwrite(neuronadr->contents, neuronadr->width, neuronadr->height, r0);
    }
    fclose(r0);
}

void permitid()
{
    //swi_osfile(10,options.idpermitpath,idpermitstring,idpermitstring+40);

    FILE* r0 = find_config(0xc0);
    if (r0 != NULL)
    {
        fprintf(r0, "%s", idpermitstring);
        fclose(r0);
    }
    options.idpermit = 1;
}

