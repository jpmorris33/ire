//
//      Sound engine wrappers for FMOD
//

#ifdef USE_FMOD3

#include <allegro.h>
#include <string.h>
#include "../types.h"
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../loadfile.hpp"
#include "../sound.h"

// defines

// variables

extern int  driftlevel;                    // Amount of frequency variation

VMINT sf_volume = 63;
VMINT mu_volume = 90;

int Songs=0;
int Waves=0;
static int SoundOn=0;

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

static void * F_CALLBACKAPI Fsound_iFileOpen(const char *name);
static void F_CALLBACKAPI Fsound_iFileClose(void *handle);
static int F_CALLBACKAPI  Fsound_iFileRead(void *buffer, int size, void *handle);
static int F_CALLBACKAPI  Fsound_iFileSeek(void *handle, int pos, signed char whence);
static int F_CALLBACKAPI  Fsound_iFileTell(void *handle);

// Public code


int S_Init()
{
if(!SoundOn)
	{
	SoundOn=FSOUND_Init(44100,32,FSOUND_INIT_ACCURATEVULEVELS);
	if(SoundOn)
		{
		// Set filesystem wrappers to use
		FSOUND_File_SetCallbacks(Fsound_iFileOpen,Fsound_iFileClose,Fsound_iFileRead,Fsound_iFileSeek,Fsound_iFileTell);
		// Reserve channel 0 for streaming
		FSOUND_SetReserved(0,TRUE);
		return 1;
		}
	return 0; // Failed
	}
return 1; // Already on, say yes
}

void S_Term()
{
if(SoundOn)
	FSOUND_Close();
SoundOn=0;
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
vol *=255;
FSOUND_SetVolumeAbsolute(0,vol/100); // Set music level
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
char filename[1024];
long ctr;

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
			Bug("S_PlayMusic - could play song %s, could not find file '%s'\n",name,mustab[ctr].fname);
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
int freq,num,ctr,chn;

if(!SoundOn)
	return;

volume=(volume*255)/100;

for(ctr=0;ctr<Waves;ctr++)
	if(!istricmp(wavtab[ctr].name,name))
		{
		freq=1000;
		if(!wavtab[ctr].nodrift)
			{
			num = 10 - (rand()%20);
			freq+=(num*driftlevel)/10;
			}

		chn=FSOUND_PlaySound(FSOUND_FREE,wavtab[ctr].sample);
		if(chn>0)
			{
			FSOUND_SetVolume(chn,volume);
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
if(StreamSong_left()>100)	// Hack as it seems to return 58 always
	return 1;

IsPlaying = NULL;
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
	wavtab[ctr].sample=FSOUND_Sample_Load(FSOUND_FREE,filename,FSOUND_NORMAL,0,0);
	if(!wavtab[ctr].sample)
		ithe_panic("LoadWavs: Invalid WAV file",filename);
	Plot(0);                                        // Print a dot
	}

ilog_printf("\n");  // End the line of dots
}

/*
 *   Streaming code
 */

static FSOUND_STREAM *Stream=NULL;
static unsigned int SongLen=0; // Song length in bytes

int StreamSong_start(char *filename)
{
IFILE *ifp;
char buffer[1024];

if(Stream)
	{
	FSOUND_Stream_Close(Stream);
	Stream=NULL;
	SongLen=0;
	}

ifp=iopen(filename);
if(!ifp)
	{
	ilog_quiet("Could not iopen file\n");
	Stream=NULL;
	SongLen=0;
	return 0;
	}

strcpy(buffer,ifile_prefix);
strcat(buffer,ifp->truename);
#ifdef _WIN32
strwinslash(buffer);
#endif
iclose(ifp);

ilog_quiet("try to open '%s'\n",buffer);

Stream=FSOUND_Stream_Open(filename,FSOUND_NORMAL,0,0);	// This should work, but doesn't
if(!Stream)
	Stream=FSOUND_Stream_Open(buffer,FSOUND_NORMAL,0,0);
if(Stream)
	{
	FSOUND_Stream_Play(0,Stream); // Channel 0 reserved for streams
	FSOUND_SetVolumeAbsolute(0,(mu_volume*255)/100); // Make sure volume is as requested
	SongLen=FSOUND_Stream_GetLength(Stream);
	return 1;
	}
Bug("S_PlayMusic - could not find file '%s'\n",filename);
SongLen=0;
return 0;
}

/*
 *  Stop the music
 */

void StreamSong_stop()
{
if(Stream)
	{
	FSOUND_Stream_Stop(Stream);
	FSOUND_Stream_Close(Stream);
	Stream = NULL;
	}
IsPlaying = NULL;
}


/*
 *  Get bytes left to play
 */

unsigned int StreamSong_left()
{
int l;
if(Stream)
	{
	l=FSOUND_Stream_GetPosition(Stream);
//	ilog_printf("song position: %d/%d = %d\n",l,SongLen,SongLen-l);
	if(l>0)
		return (unsigned int)(SongLen-l);
	}
return 0;
}



/*
 * Filesystem callbacks for FMOD to use
 */

void * F_CALLBACKAPI Fsound_iFileOpen(const char *name)
{
return(unsigned int*)iopen(name);
}

void F_CALLBACKAPI Fsound_iFileClose(void *handle)
{
iclose((IFILE *)handle);
}

int F_CALLBACKAPI Fsound_iFileRead(void *buffer, int size, void *handle)
{
return iread((unsigned char *)buffer,size,(IFILE *)handle);
}

int F_CALLBACKAPI Fsound_iFileSeek(void *handle, int pos, signed char whence)
{
iseek((IFILE *)handle,pos,whence);
return 0;
}

int F_CALLBACKAPI Fsound_iFileTell(void *handle)
{
return (signed)itell((IFILE *)handle);
}


#endif
