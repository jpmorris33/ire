//
//      Sound engine wrappers for FMOD v4
//
//	This is a bare-bones implementation, it could be improved to use
//	submixing and various other goodies provided by the API
//

#ifdef USE_FMOD4

#ifdef _WIN32
#include "c:/fmodex/api/inc/fmod.h"
#include "c:/fmodex/api/inc/fmod_errors.h"
#else
#include <fmodex/fmod.h>
#include <fmodex/fmod_errors.h>
#endif

//#include <fmodex/fmod.h>
#include <allegro.h>
#include <string.h>
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../loadfile.hpp"
#include "../sound.h"

// defines

// variables

extern int  driftlevel;                    // Amount of frequency variation

int sf_volume = 63;
int mu_volume = 90;

int Songs=0;
int Waves=0;
static FMOD_SYSTEM *SoundOn=NULL;
static FMOD_CHANNEL *StrChan=NULL;

struct SMTab *wavtab;       // Tables of data for each sound and song.
struct SMTab *mustab;       // Filled by script.cc

static char *IsPlaying=NULL;

// functions

void S_Load();              // Load the sound and music
void S_SoundVolume();       // Set the Sound volume
void S_MusicVolume();       // Set the Music volume
void S_PlaySample(char *s,int v);   // Play a sound sample
void S_PlayMusic(char *s);  // Play a song
void S_StopMusic();         // Stop the music

static void LoadMusic();
static void LoadWavs();

static unsigned int StreamSong_left();

// Callbacks for Fmod's filesystem

/*
static void * F_CALLBACKAPI Fsound_iFileOpen(char *name);
static void F_CALLBACKAPI Fsound_iFileClose(unsigned int handle);
static int F_CALLBACKAPI  Fsound_iFileRead(void *buffer, int size, unsigned int handle);
static int F_CALLBACKAPI  Fsound_iFileSeek(unsigned int handle, int pos, signed char whence);
static int F_CALLBACKAPI  Fsound_iFileTell(unsigned int handle);
*/

static FMOD_RESULT Fsound_iFileOpen(char *name, int unicode, unsigned int *filesize, void **handle, void **userdata);
static FMOD_RESULT Fsound_iFileClose(void *handle, void *userdata);
static FMOD_RESULT Fsound_iFileRead(void *handle, void *buffer, unsigned int size, unsigned int *sizeread, void *userdata);
static FMOD_RESULT Fsound_iFileSeek(void *handle, unsigned int pos, void *userdata);

// Public code


int S_Init()
{
unsigned int version;

if(!SoundOn)
	{
	FMOD_System_Create(&SoundOn);
	if(SoundOn)
		{
		FMOD_System_GetVersion(SoundOn,&version);
		if(version < FMOD_VERSION)
			{
			FMOD_System_Release(SoundOn);
			SoundOn=NULL;
			return 0;
			}

		FMOD_System_Init(SoundOn,32,FMOD_INIT_NORMAL,NULL);

		// Set filesystem wrappers to use
//		FMOD_System_SetFileSystem(SoundOn,Fsound_iFileOpen,Fsound_iFileClose,Fsound_iFileRead,Fsound_iFileSeek,0);
		// Reserve channel 0 for streaming
//		FSOUND_SetReserved(0,TRUE);
		return 1;
		}
	return 0; // Failed
	}
return 1; // Already on, say yes
}

void S_Term()
{
int ctr;
if(SoundOn)
	{
	// Free the samples
	for(ctr=0;ctr<Songs;ctr++)
		if(wavtab[ctr].sample)
			FMOD_Sound_Release(wavtab[ctr].sample);

	FMOD_System_Close(SoundOn);
	FMOD_System_Release(SoundOn);
	}
SoundOn=NULL;
}

/*
 *      S_Load - Load in the music and samples.
 */

void S_Load()
{
LoadMusic();
LoadWavs();
}


/*
 *      S_MusicVolume - Set the music level
 */

void S_MusicVolume(int vol)
{
if(vol < 0)
	vol=0;
if(vol > 100)
	vol=100;
mu_volume = vol;
if(StrChan)
	FMOD_Channel_SetVolume(StrChan,(double)mu_volume/100.0);
return;
}


/*
 *      S_SoundVolume - Set the effects level
 */

void S_SoundVolume(int vol)
{
if(vol < 0)
	vol=0;
if(vol > 100)
	vol=100;
sf_volume = vol;
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

		// Is it already playing?
		if(IsPlaying == mustab[ctr].name)
			return;

		// Is something else playing?
		if(IsPlaying)
			StreamSong_stop();

		// Otherwise, no excuses
		IsPlaying = mustab[ctr].name; // This is playing now

		if(!loadfile(mustab[ctr].fname,filename))
			{
			Bug("S_PlayMusic - could not play song %s, could not find file '%s'\n",name,mustab[ctr].fname);
			return;
			}
		if(!StreamSong_start(filename))
			Bug("S_PlayMusic - could not play song '%s'\n",name);
		return;
		}

Bug("S_PlayMusic - could not find song '%s'\n",name);
}


/*
 *   S_PlaySample - play a sound sample.
 */

void S_PlaySample(char *name, int volume)
{
int freq,num,ctr,ret;
FMOD_CHANNEL *chn;

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
		chn=NULL;
		ret=FMOD_System_PlaySound(SoundOn,FMOD_CHANNEL_FREE,wavtab[ctr].sample,0,&chn);
		if(chn)
			{
			//FSOUND_SetVolume(chn,volume*2);
			FMOD_Channel_SetVolume(chn,(double)volume/100.0);
			}

//		play_sample(wavtab[ctr].sample,volume,128,freq,0); // vol,pan,freq,loop

//        if(!wavtab[ctr].handle)
//                    MIDASerror();
		return;
		}

Bug("S_PlaySample- could not find sound '%s'\n",name);
}

//
//  Is is still playing?
//

int S_IsPlaying()
{
if(!SoundOn)
	return 0;
if(StreamSong_left()>0)
	return 1;

IsPlaying = NULL;
StrChan=NULL;
return 0;
}

/*
 *  Stop the music from playing
 */

void S_StopMusic()
{
StreamSong_stop();
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
char filename2[1024];
int ctr;

//return;

if(!SoundOn)
	return;

// load in each sound

ilog_printf("  Loading sounds");     // This line is not terminated, for the dots

Plot(Waves);    // Work out how many dots to print

for(ctr=0;ctr<Waves;ctr++)
	{
	if(!loadfile(wavtab[ctr].fname,filename))
		ithe_panic("LoadWavs: Cannot open WAV file",wavtab[ctr].fname);
	strcpy(filename2,ifile_prefix);
	strcat(filename2,filename);
	wavtab[ctr].sample=NULL;
	FMOD_System_CreateSound(SoundOn,filename2,FMOD_DEFAULT,NULL,&wavtab[ctr].sample);
//	wavtab[ctr].sample=FSOUND_Sample_Load(FSOUND_FREE,filename2,FSOUND_NORMAL,0,0);
	if(!wavtab[ctr].sample)
		ithe_panic("LoadWavs: Invalid WAV file",filename2);
	Plot(0);                                        // Print a dot
	}

ilog_printf("\n");  // End the line of dots
}

/*
 *   Streaming code
 */

static FMOD_SOUND *Stream=NULL;
static unsigned int SongLen=0; // Song length in bytes

int StreamSong_start(char *filename)
{
int r;
IFILE *ifp;
char buffer[1024];

if(!SoundOn)
	{
	ilog_quiet("No sound!\n");
	return;
	}

ilog_quiet("startsong, check original\n");

if(Stream)
	{
	ilog_quiet("Rel\n");
	FMOD_Sound_Release(Stream);
	Stream=NULL;
	SongLen=0;
	}

Stream=NULL;
StrChan=NULL;
ilog_quiet("CreateSound '%s'\n",filename);

ifp=iopen(filename);
if(!ifp)
	{
	ilog_quiet("Could not iopen file\n");
	SongLen=0;
	StrChan=NULL;
	return 0;
	}

strcpy(buffer,ifile_prefix);
strcat(buffer,ifp->truename);
#ifdef _WIN32
strwinslash(buffer);
#endif
iclose(ifp);

//r=FMOD_System_CreateSound(SoundOn,filename,FMOD_SOFTWARE|FMOD_CREATESTREAM|FMOD_2D,0,&Stream);
ilog_quiet("try to open '%s'\n",buffer);
r=FMOD_System_CreateSound(SoundOn,buffer,FMOD_SOFTWARE|FMOD_CREATESTREAM|FMOD_2D,0,&Stream);
if(Stream)
	{
	ilog_quiet("Created\n");
	FMOD_System_PlaySound(SoundOn,FMOD_CHANNEL_FREE,Stream,0,&StrChan);
	FMOD_Channel_SetVolume(StrChan,(double)mu_volume/100.0);
	ilog_quiet("GetLen\n");
	SongLen=0;
	FMOD_Sound_GetLength(Stream,&SongLen,FMOD_TIMEUNIT_MS);
	ilog_quiet("GetLen ok\n");
	return 1;
	}

SongLen=0;
StrChan=NULL;
return 0;
}

/*
 *  Stop the music
 */

void StreamSong_stop()
{
ilog_quiet("StopSong\n");
if(Stream)
	{
	FMOD_Sound_Release(Stream);
	Stream = NULL;
	}
StrChan=NULL;
IsPlaying = NULL;
}


/*
 *  Get bytes left to play
 */

unsigned int StreamSong_left()
{
unsigned int pos;
if(Stream && StrChan)
	{
	pos=0;
	ilog_quiet("IsPlaying\n");
	FMOD_Channel_IsPlaying(StrChan,&pos);
	if(!pos)
		{
		StrChan=NULL;
		return 0;
		}
	
	ilog_quiet("Getpos\n");
	pos=SongLen;
	FMOD_Channel_GetPosition(StrChan,&pos,FMOD_TIMEUNIT_MS);
	ilog_quiet("Getpos ok\n");
	return SongLen-pos;
	}

StrChan=NULL;
return 0;
}



/*
 * Filesystem callbacks for FMOD to use
 */

/*
FMOD_RESULT Fsound_iFileOpen(char *name, int unicode, unsigned int *filesize, void **handle, void **userdata)
{
IFILE *ifile;

ilog_quiet("FM: open '%s'\n",name);

ifile = iopen(name);
if(ifile)
	{
	*filesize = ifile->length;
	*handle=(void *)ifile;
	*userdata=NULL;

	ilog_quiet("And got fp = %x\n",ifile->fp);

	return FMOD_OK;
	}
return FMOD_ERR_FILE_NOTFOUND;
}

FMOD_RESULT Fsound_iFileClose(void *handle, void *userdata)
{
ilog_quiet("FM: close\n");
if(handle)
	iclose((IFILE *)handle);
return FMOD_OK;
}

FMOD_RESULT Fsound_iFileRead(void *handle, void *buffer, unsigned int size, unsigned int *sizeread, void *userdata)
{
//if(ifeof((IFILE *)handle))
//	return FMOD_FILE_EOF;
ilog_quiet("FM: Read %d\n",size);
*sizeread=iread(buffer,size,(IFILE *)handle);
return FMOD_OK;
}

FMOD_RESULT Fsound_iFileSeek(void *handle, unsigned int pos, void *userdata)
{
ilog_quiet("FM: Seek %d\n",pos);
//iseek((IFILE *)handle,pos,SEEK_CUR);
iseek((IFILE *)handle,pos,SEEK_SET);
return FMOD_OK;
}
*/

#endif
