/*  file.c */

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "asylum.h"

#define STOREAREALEN (16*0x40000)

char storage[STOREAREALEN];
#define storageend (storage+STOREAREALEN)

//const char idpermitpath[]="<PsychoResource$Path>Idpermit";

static char resource_path[240];
static char score_path[240];

char configname[] = "/.asylum"; //"<PsychoResource$Path>Config";
char savegamename[] = "/.asylum_game";

static const char* score_name[4] = {
    "/EgoHighScores", "/PsycheHighScores", "/IdHighScores", "/ExtendedHighScores"
};

FILE* score_file[4];


FILE* find_game(int op)
{
    char fullname[240] = "";

    char* home = getenv("HOME");
    if (home)
	strcat(fullname, home);
    else
	return NULL;
    strcat(fullname, savegamename);
    switch (op)
    {
    case 0x40: return fopen(fullname, "rb");
    case 0x80: return fopen(fullname, "wb");
    case 0xc0: return fopen(fullname, "ab");
    default: return NULL;
    }
}

FILE* find_config(int op)
{
    char fullname[240] = "";

    char* home = getenv("HOME");
    if (home)
	strcat(fullname, home);
    else
	strcat(fullname, resource_path);
    strcat(fullname, configname);
    switch (op)
    {
    case 0x40: return fopen(fullname, "rb");
    case 0x80: return fopen(fullname, "wb");
    case 0xc0: return fopen(fullname, "ab");
    default: return NULL;
    }
}

void dropprivs()
{
#ifndef _WIN32
    setregid(getgid(), getgid());
    setreuid(getuid(), getuid());
#endif
}

uint32_t read_littleendian(uint8_t* bytes)
{
    return (*bytes)|(bytes[1]<<8)|(bytes[2]<<16)|(bytes[3]<<24);
}

uint32_t read_littleendian(uint32_t* word)
{
    return read_littleendian((uint8_t*)word);
}

void write_littleendian(uint8_t* bytes, uint32_t word)
{
    *bytes = word & 0xff;
    bytes[1] = (word>>8) & 0xff;
    bytes[2] = (word>>16) & 0xff;
    bytes[3] = (word>>24) & 0xff;
}

int loadhammered_game(char** spaceptr, char* r1, char* path)
{
    int reload;
    do
    {
        reload = loadhammered(spaceptr, r1, path);
        if (reload == -1)
        {
            badload(); return 0;
        }
    }
    while (reload == 1);
    return reload;
}

int loadhammered_level(char** spaceptr, char* r1, char* path)
{
    int reload;
    do
    {
        reload = loadhammered(spaceptr, r1, path);
        if (reload == -1) reload = badlevelload();
    }
    while (reload == 1);
    return reload;
}

int loadvitalfile(char** spaceptr, char* r1, char* path)
{
// if VS or if r0==1
    char fullname[240] = "";

    strcat(fullname, path);
    strcat(fullname, r1);
    int r4 = swi_osfile(15, fullname, 0, 0);
    if (r4 <= 0) fatalfile();
    *spaceptr = (char*)malloc(r4);
    if (swi_osfile(14, fullname, *spaceptr, 0)) fatalfile();
    return r4;
}

int loadhammered(char** spaceptr, char* r1, char* path)
{
    int r3 = swi_blitz_hammerop(0, r1, path, NULL);

    if (r3 == -1)
    {
        filenotthere(); return -1;
    }
    if (r3 == 0) return loadfile(spaceptr, r1, path);
    *spaceptr = (char*)malloc(r3);
    if (*spaceptr == NULL)
    {
        nomemory(); return 1;
    }
    int k;
    if ((k = swi_blitz_hammerop(1, r1, path, *spaceptr)) < 0)
    {
        printf("Error %i\n", -k); filesyserror(); return 1;
    }
    return k;
}

int loadfile(char** spaceptr, char* r1, char* path)
{
    int r4 = filelength(r1, path);
    char fullname[240] = "";
    // hack: +4 as feof doesn't trigger until we've passed the end
    *spaceptr = (char*)malloc(r4+4);

    strcat(fullname, path);
    strcat(fullname, r1);
    if (r4 == -1)
    {
        filenotthere(); return 1;
    }
    if (*spaceptr == NULL)
    {
        nomemory(); return 1;
    }
    if (swi_osfile(14, fullname, *spaceptr, 0))
    {
        filesyserror(); return 1;
    }
    return r4;
}

void set_paths()
{
#ifdef RESOURCEPATH
    if (chdir(RESOURCEPATH) == 0)
    {
        strcpy(resource_path, RESOURCEPATH);
        strcpy(score_path, SCOREPATH);
        /* We could fall back to ~/.asylum/ if SCOREPATH is not writable.
           However just assuming the current directory is ok is not cool. */
        return;
    }
#endif

    fprintf(stderr, "Running as uninstalled, looking for files in local directory.\n");

#ifdef HAVE_GET_EXE_PATH
    char exe_path[240];
    if (get_exe_path(exe_path, sizeof(exe_path)))
    {
        strcpy(resource_path, exe_path);
        strncat(resource_path, "/data");

        strcpy(score_path, exe_path);
        strcat(score_path, "/hiscores");
        return;
    }
#endif

    strcpy(resource_path, "data");
    strcpy(score_path, "../hiscores"); /* relative to resource_path */
}

void open_scores()
{
    char filename[PATH_MAX];

    for (int i = 0; i < 4; ++i)
    {
        strcpy(filename, score_path);
        strcat(filename, score_name[i]);
        score_file[i] = fopen(filename, "r+b");
        if (score_file[i] == NULL)
        {
            // Perhaps the file didn't exist yet
            score_file[i] = fopen(filename, "w+b");
            if (score_file[i] == NULL)
            {
                // Perhaps we don't have write permissions :(
                score_file[i] = fopen(filename, "rb");
                if (score_file[i] == NULL)
                    fprintf(stderr, "Couldn't open %s, check if the directory exists\n", filename);
                else
                    fprintf(stderr, "Opening %s read-only, high scores will not be saved\n", filename);
            }
        }
    }
}

void find_resources()
{
    set_paths();
    if (chdir(resource_path) != 0)
    {
        fprintf(stderr, "Couldn't find resources directory %s\n", resource_path);
        exit(1);
    }
}

void savescores(char* highscorearea, int mentalzone)
{
    highscorearea[13*5] = swi_oscrc(0, highscorearea, highscorearea+13*5, 1);
    if (mentalzone >= 1 && mentalzone <= 4 && score_file[mentalzone - 1] != NULL)
    {
        FILE * f = score_file[mentalzone - 1];
        fseek(f, 0, SEEK_SET);
        fwrite(highscorearea, 1, 13*5+1, f);
        fflush(f);
    }
}

void loadscores(char* highscorearea, int mentalzone)
{
    if (mentalzone >= 1 && mentalzone <= 4 && score_file[mentalzone - 1] != NULL)
    {
        FILE * f = score_file[mentalzone - 1];
        fseek(f, 0, SEEK_SET);
        if (fread(highscorearea, 1, 13*5+1, f) == 13*5+1 &&
            swi_oscrc(0, highscorearea, highscorearea+13*5, 1) == highscorearea[13*5])
        {
            return;
        }
    }
    setdefaultscores();
}

int filelength(char* name, char* path)
{
    char fullname[240] = "";

    strcat(fullname, path);
    strcat(fullname, name);
    int r4 = swi_osfile(15, fullname, NULL, NULL);
    if (r4 == -1)
    {
        filesyserror(); return 0;
    }
//if (r0!=1) {fileerror: return -1;}
    return r4;
}

void swi_osgbpb(int op, FILE* f, char* start, char* end, int b)
{
    switch (op)
    {
    case 3: for (char* i = start; (i < end) && !feof(f); i++) *i = fgetc(f);
        break;
    case 1: for (char* i = start; (i < end) && !feof(f); i++) fputc(*i, f);
        break;
    }
}

int swi_osfile(int op, const char* name, char* start, char* end)
{
    FILE* f;
    int x;

    //printf("Looking for %s\n",name);
    switch (op)
    {
    case 10: // save file
        f = fopen(name, "wb");
        for (char* i = start; i < end; i++) fputc(*i, f);
        fclose(f);
        return 0;
    case 5: // test file existence
        f = fopen(name, "rb");
        if (f == NULL) return 0;
        fclose(f);
        return 1;
    case 15: // file length
        f = fopen(name, "rb");
        if (f == NULL) return -1;
        fseek(f, 0, SEEK_END);
        x = ftell(f);
        fclose(f);
        return x;
    case 0xff:
    case 14: // load file
        f = fopen(name, "rb");
        if (f == NULL) return -1;
        for (char* i = start; !feof(f); i++) *i = fgetc(f);
        fclose(f);
        return 0;
    }
}

int swi_blitz_hammerop(int op, char* name, char* path, char* space)
{
    char fullname[240] = "";

    strcat(fullname, path);
    strcat(fullname, name);
    FILE* f = fopen(fullname, "rb");
    if (f == NULL) return -1; // file does not exist
    if ((getc(f) != 'H') || (getc(f) != 'm') || (getc(f) != 'r'))
    {
        fclose(f); return op;
    }                            // file is not Hammered

    if (op == 0) return 0x24000; // hack: should return length
    char a[524288];
    int p = 0;
    char c;
    int fmt = getc(f);
    int len = 0;
    for (int i = 0; i < 4; i++) len = ((len>>8)&0xffffff)|((getc(f)<<24)&0xff000000);

    while (!feof(f))
    {
        c = getc(f);
        if (c == 0xff) break; // end flag
        else if (c < 0x10)
        {
            // type 1
            int n = (c == 15) ? 256 : c+2;
            char d = getc(f);
            for (int i = 0; i < n; i++) a[p++] = d;
        }
        else if (c < 0x20)
        {
            // type 2
            int n = (c&0xf)+1;
            for (int i = 0; i < n; i++) a[p++] = getc(f);
        }
        else if (c < 0x40)
        {
            // type 3
            char d = getc(f);
            int P = p-1-((c&3)<<8)-d;
            if (P < 0)
            {
                fclose(f); return -3;
            }
            int n = ((c&0x1c)>>2)+2;
            for (int i = 0; i < n; i++) a[p++] = a[P--];
        }
        else if (c < 0x80)
        {
            // type 4
            char d = getc(f);
            int D = 1<<(((c&4) ? ((c&6)+2) : (c&6))>>1);
            int P = p-1-((c&1)<<8)-d;
            if (P < 0)
            {
                fclose(f); return -4;
            }
            int n = ((c&0x38)>>3)+2;
            for (int i = 0; i < n; i++) a[p++] = a[P+(i*D)/4];
        }
        else
        {
            // type 5
            char d = getc(f);
            int P = p-1-((c&7)<<8)-d;
            if (P < 0)
            {
                fclose(f); return -5;
            }
            int n = ((c&0x78)>>3)+2;
            for (int i = 0; i < n; i++) a[p++] = a[P++];
        }
    }

    if (fmt != 0)
    {
        int wlen = (len+3)>>2;
        for (int i = 0; i < p; i++)
            space[i] = a[(i%4)*wlen+(i>>2)];
    }
    else
        for (int i = 0; i < p; i++)
            space[i] = a[i];

    fclose(f);
    return p;
}
