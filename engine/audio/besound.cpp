//
//      Sound engine wrappers for Allegro (BEOS)
//

#ifdef USE_ALSOUND

#ifndef _WIN32
	#include <unistd.h>     // Fork
	#include <signal.h>
#else
	#include <process.h>
#endif

#include <stdlib.h>     // atexit(), random
#include <string.h>     // String splicing routines

#include <allegro.h>
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../loadfile.hpp"
#include "sound.h"

// defines

// variables

extern char mus_chan;                      // Number of music channels
extern char sfx_chan;                      // Number of effects channels
extern int  driftlevel;                    // Amount of frequency variation

int sf_volume = 63;
int mu_volume = 63;

int Songs=0;
int Waves=0;
static int SoundOn=0;
static char *IsPlaying=NULL;

struct SMTab *wavtab;       // Tables of data for each sound and song.
struct SMTab *mustab;       // Filled by script.cc

#ifdef USE_ALOGG
	static struct alogg_stream *stream=NULL;
	static struct alogg_thread *thread=NULL;
	static ov_callbacks ogg_vfs;
#endif

// functions

void S_Load();              // Load the sound and music
void S_SoundVolume();       // Set the Sound volume
void S_MusicVolume();       // Set the Music volume
void S_PlaySample(char *s,int v);   // Play a sound sample
void S_PlayMusic(char *s);  // Play a song
void S_StopMusic();         // Stop the music

static void LoadMusic();
static void LoadWavs();

static void SetSoundVol(int vol);
static void SetMusicVol(int vol);

#ifdef USE_ALOGG
	static int StreamSong_GetVoice();
	static int ov_iFileClose(void *datasource);
	static size_t ov_iFileRead(void *ptr, size_t size, size_t num, void *datasource);
	static int ov_iFileSeek(void *datasource, ogg_int64_t pos, int whence);
	static long ov_iFileTell(void *datasource);
#endif

// Public code


int S_Init()
{
if(SoundOn)
	return 0;

if(install_sound(DIGI_AUTODETECT,MIDI_NONE,NULL))
	return 0;	// Zero success for install_sound

#ifdef USE_ALOGG
	alogg_init();
	memset(&ogg_vfs,0,sizeof(ogg_vfs));
	ogg_vfs.read_func = ov_iFileRead;
	ogg_vfs.close_func = ov_iFileClose;
	ogg_vfs.seek_func = ov_iFileSeek;
	ogg_vfs.tell_func = ov_iFileTell;
#endif

SoundOn=1;

// Okay
return 1;
}

void S_Term()
{
if(!SoundOn)
	return;

StreamSong_stop();
#ifdef USE_ALOGG
	alogg_exit();
#endif

SoundOn=0;
}

/*
 *      S_Load - Load in the music and samples.
 */

void S_Load()
{
if(!SoundOn)
	return;
LoadMusic();
LoadWavs();
}


/*
 *      S_MusicVolume - Set the music level
 */

void S_MusicVolume(int vol)
{
mu_volume = vol;
SetMusicVol(mu_volume);
return;
}


/*
 *      S_SoundVolume - Set the effects level
 */

void S_SoundVolume(int vol)
{
sf_volume = vol;
SetSoundVol(sf_volume);
return;
}

/*
 *   S_PlayMusic - play an ogg stream
 */


void S_PlayMusic(char *name)
{
IFILE *ifp;
char filename[1024];
long offset,ctr;

if(!SoundOn)
	return;

// Is the music playing?  If not, this will clear IsPlaying
S_IsPlaying();

for(ctr=0;ctr<Songs;ctr++)
	if(!istricmp(mustab[ctr].name,name))
		{
		// If the music is marked Not Present, ignore it (music is optional)
		if(mustab[ctr].fname[0] == 0)
			return;
		// Otherwise, no excuses

		if(!loadfile(mustab[ctr].fname,filename))
			{
			Bug("S_PlayMusic - could play song %s, could not find file '%s'\n",name,mustab[ctr].fname);
			return;
			}

		ifp = iopen(filename);
		offset = ifp->origin;
		len = ifp->length;
		strcpy(filename,ifp->truename);
		iclose(ifp);

		if(!ForkPlayer_start(filename,offset,len))
			{
			// Try again, with system prefix
			ifp = iopen(filename);
			offset = ifp->origin;
			len = ifp->length;
			strcpy(filename,ifile_prefix);
			strcat(filename,ifp->truename);
			strslash(filename);
			iclose(ifp);
			if(!ForkPlayer_start(filename,offset,len))
				Bug("S_PlayMusic - could not play song '%s'\n",name);
			}
		return;
		}

Bug("S_PlayMusic - could not find song '%s'\n",name);
}


/*
 *   S_StopMusic - Stop the current song (if any)
 */

void S_StopMusic()
{
StreamSong_stop();
}


/*
 *   S_PlaySample - play a sound sample.
 */

void S_PlaySample(char *name, int volume)
{
int freq,num,ctr;

if(!SoundOn)
	return;

for(ctr=0;ctr<Waves;ctr++)
	if(!istricmp(wavtab[ctr].name,name))
		{
		freq=1000;
		if(!wavtab[ctr].nodrift)
			{
			num = 10 - (rand()%20);
			freq+=(num*driftlevel)/10;
			}
		play_sample(wavtab[ctr].sample,volume,128,freq,0); // vol,pan,freq,loop
		return;
		}

Bug("S_PlaySample- could not find sound '%s'\n",name);
}


//
//   Private code hereon
//

/*
 *   LoadMusic - Make sure the music files are there, else mark as Not Present
 */

void LoadMusic()
{
char filename[1024];
int ctr;

if(!SoundOn)
	return;

ilog_printf("  Checking music.. ");     // This line is not terminated!

for(ctr=0;ctr<Songs;ctr++)
	if(!loadfile(mustab[ctr].fname,filename))
		{
		ilog_quiet("Warning: Could not load music file '%s'\n",mustab[ctr].fname);
		mustab[ctr].fname[0]=0;	// Erase the filename to mark N/A
		}

ilog_printf("done.\n");  // This terminates the line above

}


/*
 *   LoadWavs - Load in the sounds
 */

void LoadWavs()
{
char filename[1024];
int ctr;

if(!SoundOn)
	return;

// load in each sound

ilog_printf("  Loading sounds");     // This line is not terminated, for the dots

Plot(Waves);    // Work out how many dots to print

for(ctr=0;ctr<Waves;ctr++)
	{
	if(!loadfile(wavtab[ctr].fname,filename))
		ithe_panic("LoadWavs: Cannot open WAV file",wavtab[ctr].fname);
	wavtab[ctr].sample=iload_wav(filename);
	if(!wavtab[ctr].sample)
		ithe_panic("LoadWavs: Invalid WAV file",filename);
	Plot(0);                                        // Print a dot
	}

ilog_printf("\n");  // End the line of dots

}



void SetSoundVol(int vol)
{
int ctr,musvoice;

// Get channel used by streamer because we don't want to touch that one
musvoice = StreamSong_GetVoice();

for(ctr=0;ctr<256;ctr++)
	if(ctr != musvoice)
		voice_set_volume(ctr,vol);
}


void SetMusicVol(int vol)
{
int musvoice;

// Get channel used by streamer
musvoice = StreamSong_GetVoice();
if(musvoice>=0)
	voice_set_volume(musvoice,vol);
}


// Stubs for the music engine

int S_IsPlaying()
{
#ifdef USE_ALOGG
if(!stream || !thread || !alogg_is_thread_alive(thread))
	{
	IsPlaying = NULL;
	return 0;
	}
#endif

return 1;
}

int StreamSong_start(char *filename)
{
IFILE *ifile;

if(!SoundOn)
	return;

#ifdef USE_ALOGG
	ifile = iopen(filename);

	stream = alogg_start_streaming_callbacks((void *)ifile,&ogg_vfs,40960,NULL);
	if(!stream)
		return 0;
	thread = alogg_create_thread(stream);

	SetMusicVol(mu_volume);

#endif

return 1;
}

void StreamSong_stop()
{
if(!SoundOn)
	return;

IsPlaying = NULL;

#ifdef USE_ALOGG
	if(!stream)
		return;

	if(thread)
		{
		alogg_stop_thread(thread);
		while(alogg_is_thread_alive(thread));
		alogg_destroy_thread(thread);
		thread=NULL;
		}

	stream=NULL;

#endif

return;
}


int StreamSong_GetVoice()
{
int i,voice=-1;
#ifdef USE_ALOGG
AUDIOSTREAM *audiostream=NULL;
#endif

if(!SoundOn)
	return -1;

#ifdef USE_ALOGG
	if(!stream)
		return  -1;

	if(!thread)
		return  -1;

	if(!alogg_is_thread_alive(thread))
		return  -1;

	// Tranquilise the thread while we mess around with it
	i=alogg_lock_thread(thread);
	if(!i)
		{
		// Get the underlying audio stream structure
		audiostream=alogg_get_audio_stream(stream);
		if(audiostream)
			{
			// Set the volume level
			voice = audiostream->voice;
			free_audio_stream_buffer(audiostream);
			}
		// Leave the thread to wake up
		alogg_unlock_thread(thread);
		}
	else
		printf("Alogg thread lock error %d\n",i);
#endif

return voice;
}


/*
 * Filesystem callbacks for ALOGG/Vorbisfile to use
 */

#ifdef USE_ALOGG

int ov_iFileClose(void *datasource)
{
iclose((IFILE *)datasource);
return 1;
}

size_t ov_iFileRead(void *ptr, size_t size, size_t num, void *datasource)
{
return (size_t)iread((char *)ptr,size*num,(IFILE *)datasource);
}

int ov_iFileSeek(void *datasource, ogg_int64_t pos, int whence)
{
iseek((IFILE *)datasource,(unsigned int)pos,whence);
return 1;
}

long ov_iFileTell(void *datasource)
{
return (long)itell((IFILE *)datasource);
}
#endif

#endif


static pid_t ForkID=0;
static FILE *oggfile;

// Create a fork of the program and run the player.
// Then self-destruct.

int ForkPlayer_start(char *songfile, long offset, long length)
{
int r;
if(ForkID)
	{
	kill(ForkID,SIGKILL); // SANCTIFY
	unload_vorbis_stream();
	oggfile=NULL;
	}

// Open the file up first
oggfile=fopen(songfile,"rb");
if(!oggfile)
	return 0; // Failed

// Now try playing it
fseek(oggfile,offset,SEEK_SET);

ForkID=fork();
if(ForkID)
	return 1;

if(!load_vorbis_stream_offset(oggfile))
		{
		fclose(oggfile);
		raise(SIGKILL); // Ouch!
//		return 0;
		}

while(poll_vorbis_stream() != -1);
unload_vorbis_stream();
oggfile=NULL;
ForkID=0;       // Kinda pointless
raise(SIGKILL); // Suicide
return 0;
}

// Stop any music

void ForkPlayer_stop()
{
if(ForkID)
	{
	kill(ForkID,SIGKILL); // SANCTIFY
	unload_vorbis_stream();
	oggfile=NULL;
	}
ForkID=0;
}

