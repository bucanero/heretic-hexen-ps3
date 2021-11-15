/* hhexen for PS3- ported by Hermes */

/*   
    Copyright (C) 2010 Hermes
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <string.h>
#include <malloc.h>

#include <stdlib.h>
#include <unistd.h>

#include <psl1ght/lv2.h>
#include <psl1ght/lv2/spu.h>
#include <lv2/spu.h>

#include <lv2/process.h>
#include <psl1ght/lv2/timer.h>
#include <psl1ght/lv2/thread.h>
#include <sysmodule/sysmodule.h>
#include <sysutil/events.h>

#include <io/pad.h>

#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>

#include "h2def.h"
#include "r_local.h"
#include "p_local.h"    // for P_AproxDistance
#include "sounds.h"
#include "i_sound.h"
#include "soundst.h"
#include "st_start.h"

#include "graphics.h"
#include "spu_soundmodule.bin.h" // load SPU Module
#include "spu_soundlib.h"


// SPU
u32 spu = 0;
sysSpuImage spu_image;

#define SPU_SIZE(x) (((x)+127) & ~127)

// Macros

#define stricmp strcasecmp
#define DEFAULT_ARCHIVEPATH     ""
#define PRIORITY_MAX_ADJUST 10
#define DIST_ADJUST (MAX_SND_DIST/PRIORITY_MAX_ADJUST)


char *str_trace= "";

int16_t ShortSwap(int16_t b)
{

    b=(b<<8) | ((b>>8) & 0xff);

    return b;
}

int32_t LongSwap(int32_t b)
{

    b=(b<<24) | ((b>>24) & 0xff) | ((b<<8) & 0xff0000) | ((b>>8) & 0xff00);

    return b;
}

int is_16_9=0;

//byte *pcscreen, *destscreen;


unsigned temp_pad=0,new_pad=0,old_pad=0;

PadInfo padinfo;
PadData paddata;
int pad_alive=0;

#define BUTTON_LEFT       32768
#define BUTTON_DOWN       16384
#define BUTTON_RIGHT      8192
#define BUTTON_UP         4096
#define BUTTON_START      2048
#define BUTTON_R3         1024
#define BUTTON_L3         512
#define BUTTON_SELECT     256
            
#define BUTTON_SQUARE     128
#define BUTTON_CROSS      64
#define BUTTON_CIRCLE     32
#define BUTTON_TRIANGLE   16
#define BUTTON_R1         8
#define BUTTON_L1         4
#define BUTTON_R2         2
#define BUTTON_L2         1

int rumble1_on=0;
int rumble2_on=0;
int last_rumble=0;

void send_damage_signal(int flag)
{
    if(!flag)
        rumble1_on=1;
    else
        rumble2_on=1;
}

unsigned ps3pad_read()
{
int n;

PadActParam actparam;

unsigned butt=0;

    pad_alive=0;

    sysCheckCallback();

    ioPadGetInfo(&padinfo);

    for(n = 0; n < MAX_PADS; n++) {
            
       if(padinfo.status[n])  {
                    
                    ioPadGetData(n, &paddata);
                    pad_alive=1;
                    butt= (paddata.button[2]<<8) | (paddata.button[3] & 0xff);
                    break;
        
         }
    }

			
    
    if(!pad_alive) butt=0;
    else {
        actparam.small_motor = 0;
		actparam.large_motor = 0;
        
        if(rumble1_on) {
            
			actparam.large_motor = 255;
              
            rumble1_on++;
          
            if(rumble1_on>15) rumble1_on=0;
          
        }

        if(rumble2_on) {
           
            actparam.small_motor = 1;

            rumble2_on++;

            if(rumble2_on>10) rumble2_on=0;
        }

        last_rumble=n;
        ioPadSetActDirect(n, &actparam);
    }

        temp_pad=butt;

        new_pad=temp_pad & (~old_pad);old_pad=temp_pad;


return butt;
}


int exit_by_reset=0;

int return_reset=2;

void reset_call() {exit_by_reset=return_reset;}
void power_call() {exit_by_reset=3;}


// Public Data

int DisplayTicker = 0;

// sd mounted?

int sd_ok=0;



u32 *ps3_scr, *ps3_scr2; // ps3 texture displayed





#define TEXT_W SCREENWIDTH
#define TEXT_H SCREENHEIGHT

u32 inited;

#define INITED_CALLBACK     1
#define INITED_SPU          2
#define INITED_SOUNDLIB     4


void My_Quit(void) {
    
    PadActParam actparam;
    actparam.small_motor = 0;
    actparam.large_motor = 0;

    if(inited & INITED_CALLBACK)
        sysUnregisterCallback(EVENT_SLOT0);
    
    if(inited & INITED_SOUNDLIB)
        SND_End();

    if(inited & INITED_SPU) {
        cls();
        s_printf("Destroying SPU... ");
        scr_flip();
        sleep(1);
        lv2SpuRawDestroy(spu);
        sysSpuImageClose(&spu_image);
    }

    inited=0;
    
}

static void sys_callback(uint64_t status, uint64_t param, void* userdata) {

     switch (status) {
        case EVENT_REQUEST_EXITAPP:
                
            My_Quit();
            sysProcessExit(1);
            break;
      
       default:
           break;
         
    }
}


extern u8 aspect;

static char my_wad[2048];

void copyfiles(char *src, char *dst)
{
    Lv2FsFile dir;

    char name1[1024], name2[1024];

    void *mem = NULL;

    FILE *fp1, *fp2;

    int readed=0;
    int copied=0;

    lv2FsOpenDir(src, &dir);

    cls();

    mem= malloc(256*1024);

    while(1) {

        u64 read = sizeof(Lv2FsDirent);
        Lv2FsDirent entry;

        if(lv2FsReadDir(dir, &entry, &read)<0 || read <= 0) {
            lv2FsCloseDir(dir);
            break;
        }

        if(!(entry.d_type & 2)) continue;

        sprintf(name1, "%s%s", src, entry.d_name);
        sprintf(name2, "%s%s", dst, entry.d_name);
        
        s_printf("Copying %s\n", entry.d_name);
        scr_flip();

        fp1 = fopen(name1, "r");
        
        if(fp1) {

            fp2 = fopen(name2, "w");
            if(fp2) {

                copied++;

                while(1) {
                
                    readed =  fread(mem, 1, 256*1024, fp1);
                    if(readed<=0) break;

                    fwrite(mem, 1, readed, fp2);
                }

            fclose(fp2);   
            }
        fclose(fp1);
        }

        //s_printf("%x %s\n", entry.d_type, entry.d_name);

    }
    
    if(mem) free(mem);
    
    if(copied) {
        color= 0xff00ff30;
        s_printf("Done!!!");
        color= 0xffffffff;
        scr_flip();
        sleep(3);
    }
}


int main(int argc, char **argv)
{
    
    myargc = 0;//argc;
    myargv = NULL;//argv;

    u32 entry = 0;
    u32 segmentcount = 0;
    sysSpuSegment * segments;

    VIRTUAL_WIDTH  = 848;
   // VIRTUAL_HEIGHT = 512;


    //pcscreen = destscreen = memalign(32, SCREENWIDTH * SCREENHEIGHT);
    ps3_scr  = memalign(32, TEXT_W * (TEXT_H + 8) * 4); // for game texture
    ps3_scr2 = memalign(32, TEXT_W * (TEXT_H + 8) * 4); // for menu texture
    
    scr_init();

    is_16_9= aspect==2;

    ioPadInit(7);

    if(sysRegisterCallback(EVENT_SLOT0, sys_callback, NULL)==0) inited |= INITED_CALLBACK;

    // test args to get HEXEN.WAD

    if(argc>=1) {
    
        if(argv && !strncmp((char *) argv[0], "/dev_" , 5)) {
            FILE *fp;
            char *p;
            strcpy(my_wad, (char *) argv[0]);
            p= my_wad; 
            while(*p != 0) p++; // to the end
            while(*p !='/' && p > my_wad) p--; // to the first '/'
            if(*p == '/') {
                strcpy(p+1, "HEXEN.WAD");
             
                fp = fopen(my_wad, "r");

                p[1]=0;

                if(fp) {
                    
                    fclose(fp);

                    basePath = my_wad;

                }   else {
                  
                   basePath = "/dev_usb/hexen/"; // by default use this folder
                }

            }
            
        
        }
    }

    s_printf("Initializing SPUs...");
    s_printf("%08x\n", lv2SpuInitialize(6, 5));
    scr_flip();

    s_printf("Initializing raw SPU... ");
    s_printf("%08x\n", lv2SpuRawCreate(&spu, NULL));
    scr_flip();

    s_printf("Getting ELF information... ");
    s_printf("%08x\n", sysSpuElfGetInformation(spu_soundmodule_bin, &entry, &segmentcount));
    s_printf("\tEntry Point: %08x\n\tSegment Count: %08x\n", entry, segmentcount);
    scr_flip();

    size_t segmentsize = sizeof(sysSpuSegment) * segmentcount;
    segments = (sysSpuSegment*)memalign(128, SPU_SIZE(segmentsize)); // must be aligned to 128 or it break malloc() allocations
    memset(segments, 0, segmentsize);

    s_printf("Getting ELF segments... ");
    s_printf("%08x\n", sysSpuElfGetSegments(spu_soundmodule_bin, segments, segmentcount));
    scr_flip();

    s_printf("Loading ELF image... ");
    s_printf("%08x\n", sysSpuImageImport(&spu_image, spu_soundmodule_bin, 0));
    scr_flip();

    s_printf("Loading image into SPU... ");
    s_printf("%08x\n", sysSpuRawImageLoad(spu, &spu_image));
    scr_flip();

    inited |= INITED_SPU;

    if(SND_Init(spu)==0) inited |= INITED_SOUNDLIB;
    
    cls();

    char title1[]="HHexen for PS3";
    char title2[]="ported by Hermes/www.elotrolado.net";
    char title3[]="Licensed under GPL v2";

    char title4[]="Press CIRCLE to copy from USB";


    PY=32;PX=(VIRTUAL_WIDTH-strlen(title1)*16)/2;
    s_printf ("%s", title1);

    PY=32+64;PX=(VIRTUAL_WIDTH-strlen(title2)*16)/2;
    s_printf("%s", title2);

    PY=32+192;PX=(VIRTUAL_WIDTH-strlen(title3)*16)/2;
    s_printf("%s", title3);

    if(strncmp(my_wad, "/dev_usb", 8)) {
        PY=32+192+100;PX=(VIRTUAL_WIDTH-strlen(title4)*16)/2;
        s_printf("%s", title4);
    }

    scr_flip();
    
    int n;

    for(n=0;n<150;n++) {
        ps3pad_read();
        if((new_pad & BUTTON_CIRCLE) && strncmp(my_wad, "/dev_usb", 8)) {
            copyfiles("/dev_usb/hexen/", my_wad);
            basePath = my_wad; 
            break;
        }
        usleep(20000);
    }


    SND_Pause(0);
    
    atexit (My_Quit);

    H2_Main();

    return 0;
}

void I_StartupNet (void);
void I_ShutdownNet (void);
void I_ReadExternDriver(void);


extern int usemouse, usejoystick;

extern void **lumpcache;

BOOLEAN i_CDMusic;
int i_CDTrack;
int i_CDCurrentTrack;
int i_CDMusicLength;
int oldTic;

int grabMouse;


/*
===============================================================================

        MUSIC & SFX API

===============================================================================
*/


extern sfxinfo_t S_sfx[];
extern musicinfo_t S_music[];

static channel_t Channel[MAX_CHANNELS];
static int RegisteredSong; //the current registered song.
static int NextCleanup;
static BOOLEAN MusicPaused;
static int Mus_Song = -1;
static int Mus_LumpNum;
static void *Mus_SndPtr;
static byte *SoundCurve;

static BOOLEAN UseSndScript;
static char ArchivePath[128];

extern int snd_MusicDevice;
extern int snd_SfxDevice;
extern int snd_MaxVolume;
extern int snd_MusicVolume;
extern int snd_Channels;

extern int startepisode;
extern int startmap;

// int AmbChan;

BOOLEAN S_StopSoundID(int sound_id, int priority);
void I_UpdateCDMusic(void);

//==========================================================================
//
// S_Start
//
//==========================================================================

void S_Start(void)
{
    S_StopAllSound();
    S_StartSong(gamemap, true);
}

//==========================================================================
//
// S_StartSong
//
//==========================================================================

void S_StartSong(int song, BOOLEAN loop)
{
    char *songLump;
    int track;

    if(i_CDMusic)
    { // Play a CD track, instead
        
        if(i_CDTrack)
        { // Default to the player-chosen track
            track = i_CDTrack;
        }
        else
        {
            track = P_GetMapCDTrack(gamemap);
        }
        if(track == i_CDCurrentTrack && i_CDMusicLength > 0)
        {
            return;
        }
        #if 0
        if(!I_CDMusPlay(track))
        {
            if(loop)
            {
                i_CDMusicLength = 35*I_CDMusTrackLength(track);
                oldTic = gametic;
            }
            else
            {
                i_CDMusicLength = -1;
            }
            i_CDCurrentTrack = track;
        }
        #endif
        i_CDMusicLength = -1;
    }
    else
    {
        if(song == Mus_Song)
        { // don't replay an old song
            return;
        }
        if(RegisteredSong)
        {
            I_StopSong(RegisteredSong);
            I_UnRegisterSong(RegisteredSong);
            if(UseSndScript)
            {
                Z_Free(Mus_SndPtr);
            }
            else
            {
                Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);
            }
            #ifdef __WATCOMC__
                _dpmi_unlockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
            #endif
            RegisteredSong = 0;
        }
        songLump = P_GetMapSongLump(song);
        if(!songLump)
        {
            return;
        }
        if(UseSndScript)
        {
            char name[128];
            sprintf(name, "%s%s.lmp", ArchivePath, songLump);
            M_ReadFile(name, (void *)&Mus_SndPtr);
        }
        else
        {
            Mus_LumpNum = W_GetNumForName(songLump);
            Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);
        }
        #ifdef __WATCOMC__
            _dpmi_lockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
        #endif
        RegisteredSong = I_RegisterSong(Mus_SndPtr);
        I_PlaySong(RegisteredSong, loop); // 'true' denotes endless looping.
        Mus_Song = song;
    }
}

//==========================================================================
//
// S_StartSongName
//
//==========================================================================

void S_StartSongName(char *songLump, BOOLEAN loop)
{
    int cdTrack;

    if(!songLump)
    {
        return;
    }
    if(i_CDMusic)
    {
        cdTrack = 0;

        if(!strcmp(songLump, "hexen"))
        {
            cdTrack = P_GetCDTitleTrack();
        }
        else if(!strcmp(songLump, "hub"))
        {
            cdTrack = P_GetCDIntermissionTrack();
        }
        else if(!strcmp(songLump, "hall"))
        {
            cdTrack = P_GetCDEnd1Track();
        }
        else if(!strcmp(songLump, "orb"))
        {
            cdTrack = P_GetCDEnd2Track();
        }
        else if(!strcmp(songLump, "chess") && !i_CDTrack)
        {
            cdTrack = P_GetCDEnd3Track();
        }
/*  Uncomment this, if Kevin writes a specific song for startup
        else if(!strcmp(songLump, "start"))
        {
            cdTrack = P_GetCDStartTrack();
        }
*/
        if(!cdTrack || (cdTrack == i_CDCurrentTrack && i_CDMusicLength > 0))
        {
            return;
        }

        #if 0
        if(!I_CDMusPlay(cdTrack))
        {
            if(loop)
            {
                i_CDMusicLength = 35*I_CDMusTrackLength(cdTrack);
                oldTic = gametic;
            }
            else
            {
                i_CDMusicLength = -1;
            }
            i_CDCurrentTrack = cdTrack;
            i_CDTrack = false;
        }
        #endif
    }
    else
    {
        if(RegisteredSong)
        {
            I_StopSong(RegisteredSong);
            I_UnRegisterSong(RegisteredSong);
            if(UseSndScript)
            {
                Z_Free(Mus_SndPtr);
            }
            else
            {
                Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);
            }
            #ifdef __WATCOMC__
                _dpmi_unlockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
            #endif
            RegisteredSong = 0;
        }
        if(UseSndScript)
        {
            char name[128];
            sprintf(name, "%s%s.lmp", ArchivePath, songLump);
            M_ReadFile(name, (void *)&Mus_SndPtr);
        }
        else
        {
            Mus_LumpNum = W_GetNumForName(songLump);
            Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);
        }
        #ifdef __WATCOMC__
            _dpmi_lockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
        #endif
        RegisteredSong = I_RegisterSong(Mus_SndPtr);
        I_PlaySong(RegisteredSong, loop); // 'true' denotes endless looping.
        Mus_Song = -1;
    }
}

//==========================================================================
//
// S_GetSoundID
//
//==========================================================================

int S_GetSoundID(char *name)
{
    int i;

    for(i = 0; i < NUMSFX; i++)
    {
        if(!strcmp(S_sfx[i].tagName, name))
        {
            return i;
        }
    }
    return 0;
}

//==========================================================================
//
// S_StartSound
//
//==========================================================================

void S_StartSound(mobj_t *origin, int sound_id)
{
    S_StartSoundAtVolume(origin, sound_id, 127);
}

//==========================================================================
//
// S_StartSoundAtVolume
//
//==========================================================================

void S_StartSoundAtVolume(mobj_t *origin, int sound_id, int volume)
{

    int dist, vol;
    int i;
    int priority;
    int sep;
    int angle;
    int absx;
    int absy;

    static int sndcount = 0;
    int chan;

    //printf( "S_StartSoundAtVolume: %d\n", sound_id );

    if(sound_id == 0 || snd_MaxVolume == 0)
        return;
    if(origin == NULL)
    {
        origin = players[displayplayer].mo;
        // How does the DOS version work without this?
        // players[0].mo does not get set until P_SpawnPlayer. - KR
        if( origin == NULL )
            return;

    }
    if(volume == 0)
    {
        return;
    }

    // calculate the distance before other stuff so that we can throw out
    // sounds that are beyond the hearing range.
    absx = abs(origin->x-players[displayplayer].mo->x);
    absy = abs(origin->y-players[displayplayer].mo->y);
    dist = absx+absy-(absx > absy ? absy>>1 : absx>>1);
    dist >>= FRACBITS;
    if(dist >= MAX_SND_DIST)
    {
      return; // sound is beyond the hearing range...
    }
    if(dist < 0)
    {
        dist = 0;
    }
    priority = S_sfx[sound_id].priority;
    priority *= (PRIORITY_MAX_ADJUST-(dist/DIST_ADJUST));
    if(!S_StopSoundID(sound_id, priority))
    {
        return; // other sounds have greater priority
    }
    for(i=0; i<snd_Channels; i++)
    {
        if(origin->player)
        {
            i = snd_Channels;
            break; // let the player have more than one sound.
        }
        if(origin == Channel[i].mo)
        { // only allow other mobjs one sound
            S_StopSound(Channel[i].mo);
            break;
        }
    }
    if(i >= snd_Channels)
    {
        for(i = 0; i < snd_Channels; i++)
        {
            if(Channel[i].mo == NULL)
            {
                break;
            }
        }
        if(i >= snd_Channels)
        {
            // look for a lower priority sound to replace.
            sndcount++;
            if(sndcount >= snd_Channels)
            {
                sndcount = 0;
            }
            for(chan = 0; chan < snd_Channels; chan++)
            {
                i = (sndcount+chan)%snd_Channels;
                if(priority >= Channel[i].priority)
                {
                    chan = -1; //denote that sound should be replaced.
                    break;
                }
            }
            if(chan != -1)
            {
                return; //no free channels.
            }
            else //replace the lower priority sound.
            {
                if(Channel[i].handle)
                {
                    if(I_SoundIsPlaying(Channel[i].handle))
                    {
                        I_StopSound(Channel[i].handle);
                    }
                    if(S_sfx[Channel[i].sound_id].usefulness > 0)
                    {
                        S_sfx[Channel[i].sound_id].usefulness--;
                    }
                }
            }
        }
    }
    if(S_sfx[sound_id].lumpnum == 0)
    {
        S_sfx[sound_id].lumpnum = I_GetSfxLumpNum(&S_sfx[sound_id]);
    }
    if(S_sfx[sound_id].snd_ptr == NULL)
    {
        if(0/*UseSndScript*/)
        {
            char name[128];
            sprintf(name, "%s%s.lmp", ArchivePath, S_sfx[sound_id].lumpname);
            M_ReadFile(name, (void *)&S_sfx[sound_id].snd_ptr);
        }
        else
        {
        int flag_convert=0;

            if (!lumpcache[S_sfx[sound_id].lumpnum]) flag_convert=1;
            S_sfx[sound_id].snd_ptr = W_CacheLumpNum(S_sfx[sound_id].lumpnum,
                PU_SOUND);

        //memset(S_sfx[sound_id].snd_ptr,0, W_LumpLength(S_sfx[sound_id].lumpnum));
            if(flag_convert)
                {
                int n,len;
                u16 *p;

                p=S_sfx[sound_id].snd_ptr;
                
                len=W_LumpLength(S_sfx[sound_id].lumpnum);
               
                for(n=0;n<len;n+=2) {(*p++)-=32767;}

                }
        }
        #ifdef __WATCOMC__
        _dpmi_lockregion(S_sfx[sound_id].snd_ptr,
            lumpinfo[S_sfx[sound_id].lumpnum].size);
        #endif
    }

    vol = (SoundCurve[dist]*(snd_MaxVolume*8)*volume)>>14;
    if(origin == players[displayplayer].mo)
    {
        sep = 128;
//              vol = (volume*(snd_MaxVolume+1)*8)>>7;
    }
    else
    {

        // KR - Channel[i].mo = 0 here!
        if( Channel[i].mo == NULL )
        {
            sep = 128;
            //printf( " Channel[i].mo not set\n" );
        }
        else
        
        {
        angle = R_PointToAngle2(players[displayplayer].mo->x,
            players[displayplayer].mo->y, Channel[i].mo->x, Channel[i].mo->y);
        angle = (angle-viewangle)>>24;
        sep = angle*2-128;
        if(sep < 64)
            sep = -sep;
        if(sep > 192)
            sep = 512-sep;
//              vol = SoundCurve[dist];

        }

    }

    if(S_sfx[sound_id].changePitch)
    {
        Channel[i].pitch = (byte)(127+(M_Random()&7)-(M_Random()&7));
    }
    else
    {
        Channel[i].pitch = 127;
    }
    Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr, vol,
        sep, Channel[i].pitch, 0);
    Channel[i].mo = origin;
    Channel[i].sound_id = sound_id;
    Channel[i].priority = priority;
    Channel[i].volume = volume;
    if(S_sfx[sound_id].usefulness < 0)
    {
        S_sfx[sound_id].usefulness = 1;
    }
    else
    {
        S_sfx[sound_id].usefulness++;
    }

}

//==========================================================================
//
// S_StopSoundID
//
//==========================================================================

BOOLEAN S_StopSoundID(int sound_id, int priority)
{
    int i;
    int lp; //least priority
    int found;

    if(S_sfx[sound_id].numchannels == -1)
    {
        return(true);
    }
    lp = -1; //denote the argument sound_id
    found = 0;
    for(i=0; i<snd_Channels; i++)
    {
        if(Channel[i].sound_id == sound_id && Channel[i].mo)
        {
            found++; //found one.  Now, should we replace it??
            if(priority >= Channel[i].priority)
            { // if we're gonna kill one, then this'll be it
                lp = i;
                priority = Channel[i].priority;
            }
        }
    }
    if(found < S_sfx[sound_id].numchannels)
    {
        return(true);
    }
    else if(lp == -1)
    {
        return(false); // don't replace any sounds
    }
    if(Channel[lp].handle)
    {
        if(I_SoundIsPlaying(Channel[lp].handle))
        {
            I_StopSound(Channel[lp].handle);
        }
        if(S_sfx[Channel[lp].sound_id].usefulness > 0)
        {
            S_sfx[Channel[lp].sound_id].usefulness--;
        }
        Channel[lp].mo = NULL;
    }
    return(true);
}

//==========================================================================
//
// S_StopSound
//
//==========================================================================

void S_StopSound(mobj_t *origin)
{
    int i;

    for(i=0;i<snd_Channels;i++)
    {
        if(Channel[i].mo == origin)
        {
            I_StopSound(Channel[i].handle);
            if(S_sfx[Channel[i].sound_id].usefulness > 0)
            {
                S_sfx[Channel[i].sound_id].usefulness--;
            }
            Channel[i].handle = 0;
            Channel[i].mo = NULL;
        }
    }
}

//==========================================================================
//
// S_StopAllSound
//
//==========================================================================

void S_StopAllSound(void)
{
    int i;

    //stop all sounds
    for(i=0; i < snd_Channels; i++)
    {
        if(Channel[i].handle)
        {
            S_StopSound(Channel[i].mo);
        }
    }
    memset(Channel, 0, 8*sizeof(channel_t));
}

//==========================================================================
//
// S_SoundLink
//
//==========================================================================

void S_SoundLink(mobj_t *oldactor, mobj_t *newactor)
{
    int i;

    for(i=0;i<snd_Channels;i++)
    {
        if(Channel[i].mo == oldactor)
            Channel[i].mo = newactor;
    }
}

//==========================================================================
//
// S_PauseSound
//
//==========================================================================

void S_PauseSound(void)
{
    if(i_CDMusic)
    {
        //I_CDMusStop();
    }
    else
    {
        I_PauseSong(RegisteredSong);
    }
}

//==========================================================================
//
// S_ResumeSound
//
//==========================================================================

void S_ResumeSound(void)
{
    if(i_CDMusic)
    {
        //I_CDMusResume();
    }
    else
    {
        I_ResumeSong(RegisteredSong);
    }
}

//==========================================================================
//
// S_UpdateSounds
//
//==========================================================================

void S_UpdateSounds(mobj_t *listener)
{
    int i, dist, vol;
    int angle;
    int sep;
    int priority;
    int absx;
    int absy;

    if(i_CDMusic)
    {
        //I_UpdateCDMusic();
    }
    if(snd_MaxVolume == 0)
    {
        return;
    }

    // Update any Sequences
    SN_UpdateActiveSequences();

    if(NextCleanup < gametic)
    {
        if(UseSndScript)
        {
            for(i = 0; i < NUMSFX; i++)
            {
                if(S_sfx[i].usefulness == 0 && S_sfx[i].snd_ptr)
                {
                    S_sfx[i].usefulness = -1;
                }
            }
        }
        else
        {
            for(i = 0; i < NUMSFX; i++)
            {
                if(S_sfx[i].usefulness == 0 && S_sfx[i].snd_ptr)
                {
                    if(lumpcache[S_sfx[i].lumpnum])
                    {
                        if(((memblock_t *)((byte*)
                            (lumpcache[S_sfx[i].lumpnum])-
                            sizeof(memblock_t)))->id == 0x1d4a11)
                        { // taken directly from the Z_ChangeTag macro
                            Z_ChangeTag2(lumpcache[S_sfx[i].lumpnum],
                                PU_CACHE);
#ifdef __WATCOMC__
                                _dpmi_unlockregion(S_sfx[i].snd_ptr,
                                    lumpinfo[S_sfx[i].lumpnum].size);
#endif
                        }
                    }
                    S_sfx[i].usefulness = -1;
                    S_sfx[i].snd_ptr = NULL;
                }
            }
        }
        NextCleanup = gametic+35*30; // every 30 seconds
    }
    for(i=0;i<snd_Channels;i++)
    {
        if(!Channel[i].handle || S_sfx[Channel[i].sound_id].usefulness == -1)
        {
            continue;
        }
        if(!I_SoundIsPlaying(Channel[i].handle))
        {
            if(S_sfx[Channel[i].sound_id].usefulness > 0)
            {
                S_sfx[Channel[i].sound_id].usefulness--;
            }
            Channel[i].handle = 0;
            Channel[i].mo = NULL;
            Channel[i].sound_id = 0;
        }
        if(Channel[i].mo == NULL || Channel[i].sound_id == 0
            || Channel[i].mo == listener)
        {
            continue;
        }
        else
        {
            absx = abs(Channel[i].mo->x-listener->x);
            absy = abs(Channel[i].mo->y-listener->y);
            dist = absx+absy-(absx > absy ? absy>>1 : absx>>1);
            dist >>= FRACBITS;

            if(dist >= MAX_SND_DIST)
            {
                S_StopSound(Channel[i].mo);
                continue;
            }
            if(dist < 0)
            {
                dist = 0;
            }
            //vol = SoundCurve[dist];
            vol = (SoundCurve[dist]*(snd_MaxVolume*8)*Channel[i].volume)>>14;
            if(Channel[i].mo == listener)
            {
                sep = 128;
            }
            else
            {
                angle = R_PointToAngle2(listener->x, listener->y,
                                Channel[i].mo->x, Channel[i].mo->y);
                angle = (angle-viewangle)>>24;
                sep = angle*2-128;
                if(sep < 64)
                    sep = -sep;
                if(sep > 192)
                    sep = 512-sep;
            }
            I_UpdateSoundParams(Channel[i].handle, vol, sep,
                Channel[i].pitch);
            priority = S_sfx[Channel[i].sound_id].priority;
            priority *= PRIORITY_MAX_ADJUST-(dist/DIST_ADJUST);
            Channel[i].priority = priority;
        }
    }
}

//==========================================================================
//
// S_Init
//
//==========================================================================

void S_Init(void)
{
    SoundCurve = W_CacheLumpName("SNDCURVE", PU_STATIC);
//      SoundCurve = Z_Malloc(MAX_SND_DIST, PU_STATIC, NULL);
    I_StartupSound();
    if(snd_Channels > 8)
    {
        snd_Channels = 8;
    }
    I_SetChannels(snd_Channels);
    I_SetMusicVolume(snd_MusicVolume);

    // Attempt to setup CD music
    if(snd_MusicDevice == snd_CDMUSIC)
    {
        ST_Message("    Attempting to initialize CD Music: ");
        if(0/*!cdrom*/)
        {
            i_CDMusic = 0;//(I_CDMusInit() != -1);
        }
        else
        { // The user is trying to use the cdrom for both game and music
            i_CDMusic = false;
        }
        if(i_CDMusic)
        {
            ST_Message("initialized.\n");
        }
        else
        {
            ST_Message("failed.\n");
        }
    }
}

//==========================================================================
//
// S_GetChannelInfo
//
//==========================================================================

void S_GetChannelInfo(SoundInfo_t *s)
{
    int i;
    ChanInfo_t *c;

    s->channelCount = snd_Channels;
    s->musicVolume = snd_MusicVolume;
    s->soundVolume = snd_MaxVolume;
    for(i = 0; i < snd_Channels; i++)
    {
        c = &s->chan[i];
        c->id = Channel[i].sound_id;
        c->priority = Channel[i].priority;
        c->name = S_sfx[c->id].lumpname;
        c->mo = Channel[i].mo;
        c->distance = P_AproxDistance(c->mo->x-viewx, c->mo->y-viewy)
            >>FRACBITS;
    }
}

//==========================================================================
//
// S_GetSoundPlayingInfo
//
//==========================================================================

BOOLEAN S_GetSoundPlayingInfo(mobj_t *mobj, int sound_id)
{
    int i;

    for(i = 0; i < snd_Channels; i++)
    {
        if(Channel[i].sound_id == sound_id && Channel[i].mo == mobj)
        {
            if(I_SoundIsPlaying(Channel[i].handle))
            {
                return true;
            }
        }
    }
    return false;
}

//==========================================================================
//
// S_SetMusicVolume
//
//==========================================================================

void S_SetMusicVolume(void)
{
    if(i_CDMusic)
    {
        //I_CDMusSetVolume(snd_MusicVolume*16); // 0-255
    }
    else
    {
        I_SetMusicVolume(snd_MusicVolume);
    }
    if(snd_MusicVolume == 0)
    {
        if(!i_CDMusic)
        {
            //I_PauseSong(RegisteredSong);
        }
        MusicPaused = true;
    }
    else if(MusicPaused)
    {
        if(!i_CDMusic)
        {
            //I_ResumeSong(RegisteredSong);
        }
        MusicPaused = false;
    }
}

//==========================================================================
//
// S_ShutDown
//
//==========================================================================

void S_ShutDown(void)
{
    extern int tsm_ID;
    if(tsm_ID != -1)
    {
        I_StopSong(RegisteredSong);
        I_UnRegisterSong(RegisteredSong);
        I_ShutdownSound();
    }
    if(i_CDMusic)
    {
        //I_CDMusStop();
    }
}
//==========================================================================
//
// S_InitScript
//
//==========================================================================

void S_InitScript(void)
{
    #if 1
    int p;
    int i;

    strcpy(ArchivePath, DEFAULT_ARCHIVEPATH);
    if(!(p = M_CheckParm("-devsnd")))
    {
        UseSndScript = false;
        SC_OpenLump("sndinfo");
    }
    else
    {
        UseSndScript = true;
        SC_OpenFile(myargv[p+1]);
    }
    while(SC_GetString())
    {
        if(*sc_String == '$')
        {
            if(!stricmp(sc_String, "$ARCHIVEPATH"))
            {
                SC_MustGetString();
                strcpy(ArchivePath, sc_String);
            }
            else if(!stricmp(sc_String, "$MAP"))
            {
                SC_MustGetNumber();
                SC_MustGetString();
                if(sc_Number)
                {
                    P_PutMapSongLump(sc_Number, sc_String);
                }
            }
            continue;
        }
        else
        {
            for(i = 0; i < NUMSFX; i++)
            {
                if(!strcmp(S_sfx[i].tagName, sc_String))
                {
                    SC_MustGetString();
                    if(*sc_String != '?')
                    {
                        strcpy(S_sfx[i].lumpname, sc_String);
                    }
                    else
                    {
                        strcpy(S_sfx[i].lumpname, "default");
                    }
                    break;
                }
            }
            if(i == NUMSFX)
            {
                SC_MustGetString();
            }
        }
    }
    SC_Close();

    for(i = 0; i < NUMSFX; i++)
    {
        if(!strcmp(S_sfx[i].lumpname, ""))
        {
            strcpy(S_sfx[i].lumpname, "default");
        }
    }
#endif
}

//==========================================================================
//
// I_UpdateCDMusic
//
// Updates playing time for current track, and restarts the track, if
// needed
//
//==========================================================================

void I_UpdateCDMusic(void)
{
    extern BOOLEAN MenuActive;

    if(MusicPaused || i_CDMusicLength < 0
    || (paused && !MenuActive))
    { // Non-looping song/song paused
        return;
    }
    i_CDMusicLength -= gametic-oldTic;
    oldTic = gametic;
    if(i_CDMusicLength <= 0)
    {
        S_StartSong(gamemap, true);
    }
}
/*
============================================================================

                            CONSTANTS

============================================================================
*/

#define SC_INDEX                0x3C4
#define SC_RESET                0
#define SC_CLOCK                1
#define SC_MAPMASK              2
#define SC_CHARMAP              3
#define SC_MEMMODE              4


#define GC_INDEX                0x3CE
#define GC_SETRESET             0
#define GC_ENABLESETRESET 1
#define GC_COLORCOMPARE 2
#define GC_DATAROTATE   3
#define GC_READMAP              4
#define GC_MODE                 5
#define GC_MISCELLANEOUS 6
#define GC_COLORDONTCARE 7
#define GC_BITMASK              8




//==================================================
//
// joystick vars
//
//==================================================

BOOLEAN joystickpresent;
unsigned joystickx, joysticky;
         

BOOLEAN I_ReadJoystick (void) // returns false if not connected
{
    return false;
}

//==================================================

#define VBLCOUNTER              34000           // hardware tics to a frame

#define TIMERINT 8
#define KEYBOARDINT 9

#define MOUSEB1 1
#define MOUSEB2 2
#define MOUSEB3 4

BOOLEAN mousepresent;
//static  int tsm_ID = -1; // tsm init flag

//===============================

int ticcount;


BOOLEAN novideo; // if true, stay in text mode for debugging


//==========================================================================

//--------------------------------------------------------------------------
//
// FUNC I_GetTime
//
// Returns time in 1/35th second tics.
//
//--------------------------------------------------------------------------
#define ticks_to_msecs(ticks)      ((u32)((ticks)/(TB_TIMER_CLOCK)))

u32 gettick();
int I_GetTime (void)
{

   // ticcount = (SDL_GetTicks()*35)/1000;

    struct timeval tv;
    gettimeofday( &tv, 0 ); 

    //printf( "GT: %lx %lx\n", tv.tv_sec, tv.tv_usec );

    ticcount = ((tv.tv_sec * 1000000) + tv.tv_usec) / 28571;
  //  ticcount = ((tv.tv_sec / 35) + (tv.tv_usec / 28571));

  //  ticcount= (ticks_to_msecs(gettick()) / 35) & 0x7fffffff;
    return( ticcount );
}


/*
============================================================================

                                USER INPUT

============================================================================
*/

//--------------------------------------------------------------------------
//
// PROC I_WaitVBL
//
//--------------------------------------------------------------------------

void I_WaitVBL(int vbls)
{
    if( novideo )
    {
        return;
    }


    while( vbls-- )
    {
     //   usleep( 16667000/1000 );
    }

}

//--------------------------------------------------------------------------
//
// PROC I_SetPalette
//
// Palette source must use 8 bit RGB elements.
//
//--------------------------------------------------------------------------

u32 paleta[256], paleta2[256], paleta_menu[256];


void I_SetPalette(byte *palette)
{
int n;

usegamma=2;
//usegamma=4;
    for(n=0;n<256;n++)
        {
        paleta[n]= gammatable[usegamma][palette[2]]
            | (gammatable[usegamma][palette[1]] << 8) 
            | (gammatable[usegamma][palette[0]] << 16) | 0xff000000;

        paleta_menu[n]= (gammatable[usegamma][palette[2]] * 0xa0 / 0xff )
            | ((gammatable[usegamma][palette[1]] * 0xa0 / 0xff ) << 8) 
            | ((gammatable[usegamma][palette[0]] * 0xa0 / 0xff ) << 16) | 0xff000000;
        
        paleta2[n]=paleta[n]; // menu palette

        palette+=3;
        }
// trick
paleta2[0]=0x00000000;
}

/*
============================================================================

                            GRAPHICS MODE

============================================================================
*/

int ps3_scr_info=0;

static int use_cheat=0;

extern byte *screen2;

extern BOOLEAN MenuActive;

void ps3_scr_update()
{
    int n,m,l,ll;

    static int blink=0;

    I_StartFrame();

    if(novideo) return;
    
    cls();

    l=ll=0;
   
    if(MenuActive) {
        
        for(n=0;n<SCREENHEIGHT; n++) 
        {
            for(m=0;m<SCREENWIDTH; m++)
                {
                ps3_scr[l+m]= (!paleta2[screen2[ll+m]]) ? paleta_menu[screen[ll+m]] : paleta2[screen2[ll+m]]; // used by game
                }
            
            l+=TEXT_W;
            ll+=SCREENWIDTH;
        }

    } else {
    
        for(n=0;n<SCREENHEIGHT; n++) 
        {
            for(m=0;m<SCREENWIDTH; m++)
                {
                ps3_scr[l+m]= (!paleta2[screen2[ll+m]]) ? paleta[screen[ll+m]] : paleta2[screen2[ll+m]]; // used by game
                }
            
            l+=TEXT_W;
            ll+=SCREENWIDTH;
        }
    }

    memset((void *) screen2, 0, SCREENWIDTH*SCREENHEIGHT);

    virtual_rescaler( !is_16_9 ? VIRTUAL_WIDTH : 640, VIRTUAL_HEIGHT, ps3_scr, SCREENWIDTH, SCREENHEIGHT);

    
    if(ps3_scr_info & 1)
        {
        //SetTexture(NULL);
        scr_clear((use_cheat>4) ? 0xf00f8f8f : 0xff0000ff);

        PY=8;
       
        letter_size(16, 32);
        autocenter=1;
        s_printf("PS3 PAD info");
        autocenter=0;
        letter_size(12, 28);

        PY+=64;PX=8;
        s_printf("Stick Left -> To Move, Stick Right -> To Look");
        
        PY+=32;PX=8;
        s_printf("Button L2 -> Run | R2 -> Fire");
        
        PY+=32;PX=8;
        s_printf("Button CROSS -> Jump");
        
        PY+=32;PX=8;
        s_printf("Button CIRCLE -> Open Door");

        PY+=32;PX=8;
        s_printf("Button SQUARE -> Use Item");

        PY+=32;PX=8;
        s_printf("Button TRIANGLE -> Active/Desactive MAP");

        PY+=32;PX=8;
        s_printf("LEFT/UP/RIGHT/DOWN -> Weapon");

        PY+=32;PX=8;
        s_printf("Button L1/R1 -> Select Item");
        
        PY+=32;PX=8;
        s_printf("Hold L2 + LEFT/RIGHT -> ZOOM MAP");

        PY+=32;PX=8;
        s_printf("Hold L2 + UP/DOWN -> for fly. With CIRCLE fly Off");

        PY+=32;PX=8;
        s_printf("SELECT -> Menu | START  -> to see this info or exit");

        autocenter=1;
        PY+=32;PX=8;
        if(!((blink>>4) & 1)) s_printf("Press START to exit");
        blink++;
        autocenter=0;

        }
    

    scr_flip();
}

/*
==============
=
= I_Update
=
==============
*/

int UpdateState;
extern int screenblocks;

void I_Update (void)
{
    int i;
    byte *dest;
    int tics;
    static int lasttic;
    int update=0;


    if(DisplayTicker)
    {
        if(/*screenblocks > 9 ||*/ UpdateState&(I_FULLSCRN|I_MESSAGES))
        {
            //dest = (byte *)screen;
            dest = (byte *) screen;
        }
        else
        {
            dest = (byte *) screen;
        }
        tics = ticcount-lasttic;
        lasttic = ticcount;
    
        if(tics > 20)
        {
            tics = 20;
        }
        for(i = 0; i < tics; i++)
        {
            *dest = 0xff;
            dest += 2;
        }
        for(i = tics; i < 20; i++)
        {
            *dest = 0x00;
            dest += 2;
        }
        
    }

    

    if(UpdateState == I_NOUPDATE)
    {
        return;
    }
    
    if(UpdateState&I_FULLSCRN)
    {
        //memcpy(pcscreen, screen, SCREENWIDTH*SCREENHEIGHT);
        UpdateState = I_NOUPDATE; // clear out all draw types

        update=1;

    }



    if(UpdateState&I_FULLVIEW)
    {
        if(UpdateState&I_MESSAGES && screenblocks > 7)
        {
            /*
            for(i = 0; i <
                (viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
            {
                memcpy(pcscreen+i, screen+i, SCREENWIDTH);
            }*/
            UpdateState &= ~(I_FULLVIEW|I_MESSAGES);
            update=1;

          
        }
        else
        {
            /*
            for(i = viewwindowy*SCREENWIDTH+viewwindowx; i <
                (viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
            {
                memcpy(pcscreen+i, screen+i, viewwidth);
            }
            */
            UpdateState &= ~I_FULLVIEW;

            update=1;

        }
    
    }
    
    if(UpdateState&I_STATBAR)
    {
        /*memcpy(pcscreen+SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
            screen+SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
            SCREENWIDTH*SBARHEIGHT);*/
        UpdateState &= ~I_STATBAR;


        update=1;
    }
    if(UpdateState&I_MESSAGES)
    {
        //memcpy(pcscreen, screen, SCREENWIDTH*28);
        UpdateState &= ~I_MESSAGES;

    update=1;
    }

if(update) ps3_scr_update();
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{

    grabMouse = 0;


        I_SetPalette( W_CacheLumpName("PLAYPAL", PU_CACHE) );
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
}

//===========================================================================


//
// I_StartTic
//
void I_StartTic (void)
{

}


/*
===============
=
= I_StartFrame
=
===============
*/

int last_event[128];

#define MACRO_EVENT(x, y) \
    event.data1 = y; \
    if(x) { \
       if(last_event[nevent]!=1) {event.type = ev_keydown;H2_PostEvent(&event); last_event[nevent]=1;} \
    } \
    else { \
       if(last_event[nevent]==1) {event.type = ev_keyup;H2_PostEvent(&event); last_event[nevent]=-1;} \
    }\
    nevent++;

#define J_DEATHZ 48 // death zone for sticks

extern void set_my_cheat(int indx);
BOOLEAN usergame;

extern BOOLEAN askforquit;

void I_StartFrame (void)
{
    // hermes
    
    static int counter=0;
    event_t ev; // joystick event
    event_t event; // keyboard event

    int k_up=0,k_down=0,k_left=0,k_right=0,k_esc=0,k_enter=0,k_tab=0, k_del=0, k_pag_down=0;
    int k_jump=0,k_leftsel=0,k_rightsel=0,k_alt=0, k_space=0, k_plus=0, k_minus=0;
    int k_1=0,k_2=0,k_3=0,k_4=0,k_fly=0;

    static int w_jx=1,w_jy=1;

    int nevent=0;

    counter++;


    ps3pad_read();
    ev.type = ev_joystick;
    ev.data1 =  0;
    ev.data2 =  0;
    ev.data3 =  0;

    // menu

//    if(new_pad & BUTTON_START) k_enter = 1;

    
    if(!ps3_scr_info && (MenuActive || askforquit))
        {
        if(new_pad & (BUTTON_CROSS | BUTTON_CIRCLE))
            {
            new_pad &= ~(BUTTON_CROSS | BUTTON_CIRCLE);
            k_enter = 1;
            }

        if(new_pad & BUTTON_LEFT)  {k_left=1; new_pad &= ~BUTTON_LEFT; }
        if(new_pad & BUTTON_UP)    {k_up=1;   new_pad &= ~BUTTON_UP;   }
        if(new_pad & BUTTON_RIGHT) {k_right=1;new_pad &= ~BUTTON_RIGHT;}
        if(new_pad & BUTTON_DOWN)  {k_down=1; new_pad &= ~BUTTON_DOWN; }
        
        }
    // stick
    if(!ps3_scr_info) {
        if(pad_alive)
            {

            if(paddata.ANA_L_V < (127-J_DEATHZ))
                {if(w_jy & 1) k_up=1; w_jy&= ~1; ev.data3 = -1;} else w_jy|=1;
            if(paddata.ANA_L_V > (127+J_DEATHZ))
                {if(w_jy & 2) k_down=1; w_jy&= ~2; ev.data3 = 1;} else w_jy|=2;

            
            if(paddata.ANA_L_H > (127+100))
                {if(usergame) {k_right=1; k_alt=1;} else {if(w_jx & 1) {k_right=1;k_alt=1;} w_jx&= ~1;}} else w_jx|=1;
           
            if(paddata.ANA_L_H < (127-100))
                {if(usergame) {k_left=1; k_alt=1;} else {if(w_jx & 2) {k_left=1;k_alt=1;} w_jx&= ~2;}} else w_jx|=2;
                

            if(paddata.ANA_R_H > (127+J_DEATHZ))
                {if(counter & 1) ev.data2 = 1;k_right=1;k_left=0;k_alt=0;} 
            if(paddata.ANA_R_H < (127-J_DEATHZ))
                {if(counter & 1) ev.data2 = -1;k_left=1;k_right=0;k_alt=0;}

             // fly
        
            if(old_pad & BUTTON_L2)
                {
                if(old_pad & BUTTON_UP)   {k_fly = -1; new_pad &= ~ BUTTON_UP;}
                if(old_pad & BUTTON_DOWN) {k_fly =  1; new_pad &= ~ BUTTON_DOWN;}
               
                if(new_pad & BUTTON_CIRCLE) {k_fly=-2;new_pad &= ~ BUTTON_CIRCLE;}
                
                }
            
            if(paddata.ANA_R_V > (127+J_DEATHZ)) k_del=1;
            if(paddata.ANA_R_V < (127-J_DEATHZ)) k_pag_down=1; 
            
            }

        if(new_pad & BUTTON_CROSS) k_jump=1;  // jump
        if(old_pad & BUTTON_L2) ev.data1|=4; // run

        // map
        if(new_pad & BUTTON_TRIANGLE) k_tab=1;

        if(old_pad & BUTTON_L2)
            {
            if(old_pad & BUTTON_LEFT)  {k_minus=1; new_pad &= ~ BUTTON_LEFT;}
            if(old_pad & BUTTON_RIGHT) {k_plus=1; new_pad &= ~ BUTTON_RIGHT;}
            }

        // menu
        if(new_pad & BUTTON_SELECT) k_esc=1;

        // change weapon
        
        if(new_pad & BUTTON_LEFT)  k_1=1;
        if(new_pad & BUTTON_UP)    k_2=1;
        if(new_pad & BUTTON_RIGHT) k_3=1;
        if(new_pad & BUTTON_DOWN)  k_4=1;
       

        if(new_pad & BUTTON_SQUARE)    
            {k_enter=1;}  // use object
       
        if(old_pad & BUTTON_CIRCLE) {k_space=1;}  // open
        if(old_pad & BUTTON_R2) ev.data1|=1; // fire

        if(new_pad & BUTTON_L1) k_leftsel=1; // sel left object
        if(new_pad & BUTTON_R1) k_rightsel=1; // sel right object

    }


    if((new_pad & BUTTON_L1) && (ps3_scr_info & 1)) use_cheat++;

    if((ps3_scr_info & 1) && (old_pad & BUTTON_L1) && use_cheat>4 && usergame)
        { 
        if(new_pad & BUTTON_UP) {new_pad &=~ BUTTON_UP; set_my_cheat(4);ps3_scr_info&=2;}
        if(new_pad & BUTTON_RIGHT) {new_pad &=~ BUTTON_RIGHT; set_my_cheat(2);ps3_scr_info&=2;}
        if(new_pad & BUTTON_DOWN) {new_pad &=~ BUTTON_DOWN; set_my_cheat(5);ps3_scr_info&=2;}
        if(new_pad & BUTTON_LEFT) {new_pad &=~ BUTTON_LEFT; set_my_cheat(6);ps3_scr_info&=2;}
        if(new_pad & BUTTON_TRIANGLE) {new_pad &=~ BUTTON_TRIANGLE; set_my_cheat(9);ps3_scr_info&=2;}
        }
    else
        if((new_pad & BUTTON_START))
        {
        ps3_scr_info^=1;    
        }


    H2_PostEvent (&ev);

    nevent=0; // first event
    
    // zoom map
    MACRO_EVENT(k_minus, '-');

    MACRO_EVENT(k_plus, '=');

    // jump
    MACRO_EVENT(k_jump, '/');

    // open
    MACRO_EVENT(k_space, ' ');

    MACRO_EVENT(k_up, KEY_UPARROW);
    
    MACRO_EVENT(k_down, KEY_DOWNARROW);

    MACRO_EVENT(k_left, KEY_LEFTARROW);
    
    MACRO_EVENT(k_right, KEY_RIGHTARROW);

    // strafe
    MACRO_EVENT(k_alt, KEY_ALT);

    // enter
    MACRO_EVENT(k_enter, KEY_ENTER);

    // menu
    MACRO_EVENT(k_esc, KEY_ESCAPE);

    // mapa
    MACRO_EVENT(k_tab, KEY_TAB);

    MACRO_EVENT(k_leftsel, KEY_LEFTBRACKET);

    MACRO_EVENT(k_rightsel, KEY_RIGHTBRACKET);

    //  mira abajo
    MACRO_EVENT(k_del, KEY_DEL);

    // mira arriba
    MACRO_EVENT(k_pag_down, KEY_PGDN);
    
    // weapon
    MACRO_EVENT(k_1, '1');

    MACRO_EVENT(k_2, '2');

    MACRO_EVENT(k_3, '3');

    MACRO_EVENT(k_4, '4');
    
    // fly up
    MACRO_EVENT(k_fly==-1, KEY_PGUP);
    
    // fly down
    MACRO_EVENT(k_fly==1, KEY_INS);
    
    // fly drop
    MACRO_EVENT(k_fly==-2, KEY_HOME);  

}


/*
============================================================================

                            MOUSE

============================================================================
*/


/*
================
=
= StartupMouse
=
================
*/

void I_StartupCyberMan(void);

void I_StartupMouse (void)
{
    mousepresent = 0;

//  I_StartupCyberMan();
}




/*
===============
=
= I_StartupJoystick
=
===============
*/



void I_StartupJoystick (void)
{
// hermes

joystickpresent = true;

centerx = 0;
centery = 0;

}

/*
===============
=
= I_Init
=
= hook interrupts and set graphics mode
=
===============
*/

void I_Init (void)
{
    extern void I_StartupTimer(void);

    novideo = 0;
    
    I_StartupMouse();

    I_StartupJoystick();
    ST_Message("  S_Init... ");

    S_Init();
    S_Start();
}


/*
===============
=
= I_Shutdown
=
= return to default system state
=
===============
*/

void I_Shutdown (void)
{
    I_ShutdownGraphics ();
    S_ShutDown ();
}


/*
================
=
= I_Error
=
================
*/
char scr_str[256];

void I_Error (char *error, ...)
{
    va_list argptr;

    D_QuitNetGame ();
    I_Shutdown ();
    va_start (argptr,error);
    vsprintf (scr_str,error, argptr);
    va_end (argptr);
    
    cls();
    PX=32;
    PY=16;
    letter_size(12, 24);
    s_printf ("%s\n", scr_str);
    scr_flip();
    sleep(10);
    exit (1);
}

//--------------------------------------------------------------------------
//
// I_Quit
//
// Shuts down net game, saves defaults, prints the exit text message,
// goes to text mode, and exits.
//
//--------------------------------------------------------------------------

void I_Quit(void)
{
    D_QuitNetGame();
    M_SaveDefaults();
    I_Shutdown();

    exit(0);
}

/*
===============
=
= I_ZoneBase
=
===============
*/

byte *I_ZoneBase (int *size)
{
    static byte *ptr=NULL;
    int heap = 0x800000*2;

  
    if(!ptr)
        {
        ptr =memalign(32, heap);

        ST_Message ("  0x%x allocated for zone, ", heap);
        ST_Message ("ZoneBase: 0x%lX\n", (long)ptr);

        if ( ! ptr )
            I_Error ("  Insufficient DPMI memory!");

        memset(ptr, 255, heap);
        }

    *size = heap;
    
    return ptr;
}

/*
=============
=
= I_AllocLow
=
=============
*/

byte *I_AllocLow (int length)
{
    return malloc( length );
}

/*
============================================================================

                        NETWORKING

============================================================================
*/

/* // FUCKED LINES
typedef struct
{
    char    priv[508];
} doomdata_t;
*/ // FUCKED LINES

#define DOOMCOM_ID              0x12345678l

/* // FUCKED LINES
typedef struct
{
    long    id;
    short   intnum;                 // DOOM executes an int to execute commands

// communication between DOOM and the driver
    short   command;                // CMD_SEND or CMD_GET
    short   remotenode;             // dest for send, set by get (-1 = no packet)
    short   datalength;             // bytes in doomdata to be sent

// info common to all nodes
    short   numnodes;               // console is allways node 0
    short   ticdup;                 // 1 = no duplication, 2-5 = dup for slow nets
    short   extratics;              // 1 = send a backup tic in every packet
    short   deathmatch;             // 1 = deathmatch
    short   savegame;               // -1 = new game, 0-5 = load savegame
    short   episode;                // 1-3
    short   map;                    // 1-9
    short   skill;                  // 1-5

// info specific to this node
    short   consoleplayer;
    short   numplayers;
    short   angleoffset;    // 1 = left, 0 = center, -1 = right
    short   drone;                  // 1 = drone

// packet data to be sent
    doomdata_t      data;
} doomcom_t;
*/ // FUCKED LINES

extern  doomcom_t               *doomcom;

/*
====================
=
= I_InitNetwork
=
====================
*/

void I_InitNetwork (void)
{
    int             i;

    i = M_CheckParm ("-net");
    if (!i)
    {
    //
    // single player game
    //
        doomcom = malloc (sizeof (*doomcom) );
        memset (doomcom, 0, sizeof(*doomcom) );
        netgame = false;
        doomcom->id = DOOMCOM_ID;
        doomcom->numplayers = doomcom->numnodes = 1;
        doomcom->deathmatch = false;
        doomcom->consoleplayer = 0;
        doomcom->ticdup = 1;
        doomcom->extratics = 0;
        return;
    }

    netgame = true;
    doomcom = (doomcom_t *) (long) atoi((char *) myargv[i+1]);
//DEBUG
doomcom->skill = startskill;
doomcom->episode = startepisode;
doomcom->map = startmap;
doomcom->deathmatch = deathmatch;

}

void I_NetCmd (void)
{
    if (!netgame)
        I_Error ("I_NetCmd when not in netgame");
}


//==========================================================================
//
//
// I_StartupReadKeys
//
//
//==========================================================================

void I_StartupReadKeys(void)
{
   //if( KEY_ESCAPE pressed )
   //    I_Quit ();
}


//EOF
