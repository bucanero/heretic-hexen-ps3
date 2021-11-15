// soundst.h
// $Revision: 505 $
// $Date: 2009-06-03 22:41:58 +0300 (Wed, 03 Jun 2009) $

#ifndef __SOUNDSTH__
#define __SOUNDSTH__

extern	int		snd_MaxVolume;
extern	int		snd_MusicVolume;

void S_Init(void);
void S_ShutDown(void);
void S_Start(void);
void S_StartSound(mobj_t *origin, int sound_id);
void S_StartSoundAtVolume(mobj_t *origin, int sound_id, int volume);
void S_StopSound(mobj_t *origin);
void S_PauseSound(void);
void S_ResumeSound(void);
void S_UpdateSounds(mobj_t *listener);
void S_StartSong(int song, BOOLEAN loop);
void S_GetChannelInfo(SoundInfo_t *s);
void S_SetMaxVolume(BOOLEAN fullprocess);
void S_SetMusicVolume(void);

#endif	/* __SOUNDSTH__ */

