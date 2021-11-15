
// I_SOUND.C
//
// Sound for SDL taken from Sam Lantinga's SDL Doom i_sound.c file.


#include <stdio.h>
#include <math.h>       // pow()

/*#include <SDL/SDL_audio.h>
#include <SDL/SDL_mutex.h>
#include <SDL/SDL_byteorder.h>
#include <SDL/SDL_version.h>*/

#include "h2def.h"
#include "sounds.h"
#include "i_sound.h"


#include <soundlib/spu_soundlib.h>

int tsm_ID = -1;

volatile int snd_blocked=0;
int _updateSound( void* unused, u8* stream, int len );
/*
 *
 *                           SOUND HEADER & DATA
 *
 *
 */

// sound information

const char snd_prefixen[] = {'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S'};

int snd_Channels;
int snd_DesiredMusicDevice, snd_DesiredSfxDevice;
int snd_MusicDevice,    // current music card # (index to dmxCodes)
	snd_SfxDevice,      // current sfx card # (index to dmxCodes)
	snd_MaxVolume,      // maximum volume for sound
	snd_MusicVolume;    // maximum volume for music
//int dmxCodes[NUM_SCARDS]; // the dmx code for a given card

int     snd_SBport, snd_SBirq, snd_SBdma;       // sound blaster variables
int     snd_Mport;                              // midi variables

extern BOOLEAN  snd_MusicAvail, // whether music is available
                snd_SfxAvail;   // whether sfx are available


void I_PauseSong(int handle)
{
}

void I_ResumeSong(int handle)
{
}

void I_SetMusicVolume(int volume)
{
}

void I_SetSfxVolume(int volume)
{
    snd_MaxVolume = volume; // THROW AWAY?
}

/*
 *
 *                              SONG API
 *
 */

int I_RegisterSong(void *data)
{
  return 0;
}

void I_UnRegisterSong(int handle)
{
}

int I_QrySongPlaying(int handle)
{
  return 0;
}

// Stops a song.  MUST be called before I_UnregisterSong().

void I_StopSong(int handle)
{
}

void I_PlaySong(int handle, BOOLEAN looping)
{
}

/*
 *
 *                                 SOUND FX API
 *
 */


struct Sample
{
    short a;       // always 0x2b11
    short b;       // always 3
    long length;   // sample length?
    char firstSample;
};




// Gets lump nums of the named sound.  Returns pointer which will be
// passed to I_StartSound() when you want to start an SFX.  Must be
// sure to pass this to UngetSoundEffect() so that they can be
// freed!


int I_GetSfxLumpNum(sfxinfo_t *sound)
{
  return W_GetNumForName(sound->lumpname);
}


int I_StartSound (int id, void *data, int vol, int sep, int pitch, int priority)
{
int len;
sfxinfo_t *sfx;
//int n;

int leftvol,rightvol;

	if (!data)
	{
		return 0;
	}

	priority = priority/10;
	if (priority > 127)
	{
		priority = 127;
	}
	if (vol > 127)
	{
		vol = 127;
	}

	// Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    sep += 1;

    // Per left/right channel.
    //  x^2 sep,
    //  adjust volume properly.
    //volume *= 8;  // puts rightvol/leftvol out of range - KR
    //volume *= 2;
    leftvol = vol - ((vol*sep*sep) >> 16); // (256*256);
    sep = sep - 257;
    rightvol = vol - ((vol*sep*sep) >> 16);    

	if( rightvol < 0) rightvol = 0;
    if( rightvol > 127 ) rightvol = 127;

	if( leftvol < 0 ) leftvol = 0;
    if( leftvol > 127 ) leftvol = 127;
       

	extern sfxinfo_t S_sfx[];
	sfx = &S_sfx[id];
	len = W_LumpLength(sfx->lumpnum);
	//if(len>10025*3) len=11025*3;


	id=SND_GetFirstUnusedVoice();
	if(id<0) {id=0;SND_StopVoice(id);}//return -1;
	
	// alignment control (32)
   // n=(32- (((long) data) & 31)) & 31;
   // data+=n;
	//len=(len-n) & ~31;

	SND_SetVoice(id, VOICE_MONO_16BIT, (pitch*11025)/128, 0, data, len, leftvol>>1, rightvol>>1, NULL);

    return id;
}

void I_StopSound(int handle)
{
	if(handle<0) return;
	//channels[ handle ]=0;
   // SFX_StopPatch(handle);

   SND_StopVoice(handle & 15);
}

int I_SoundIsPlaying(int handle)
{
	if(handle<0) return 0;
    //return SFX_Playing(handle);
	return SND_StatusVoice(handle)>0;
    return 0;
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
int leftvol,rightvol;

if(handle<0) return;

	if (vol > 127)
	{
		vol = 127;
	}

	// Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    sep += 1;

    // Per left/right channel.
    //  x^2 sep,
    //  adjust volume properly.
    //volume *= 8;  // puts rightvol/leftvol out of range - KR
    //volume *= 2;
    leftvol = vol - ((vol*sep*sep) >> 16); // (256*256);
    sep = sep - 257;
    rightvol = vol - ((vol*sep*sep) >> 16);    

	if( rightvol < 0) rightvol = 0;
    if( rightvol > 127 ) rightvol = 127;

	if( leftvol < 0 ) leftvol = 0;
    if( leftvol > 127 ) leftvol = 127;

	SND_ChangeVolumeVoice(handle & 15, leftvol, rightvol);
	SND_ChangeFreqVoice(handle & 15, (pitch*11025)/128);
}

/*
 *
 *                                                      SOUND STARTUP STUFF
 *
 *
 */

// inits all sound stuff

void I_StartupSound (void)
{

SND_Pause(0);

}


// shuts down all sound stuff

void I_ShutdownSound (void)
{
SND_Pause(1);
}


void I_SetChannels(int channels)
{
   
}
