//
//      Sound engine wrappers for SDLmixer
//

#ifdef USE_SDLSOUND

#include <stdlib.h>     // atexit(), random
#include <string.h>     // String splicing routines

#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../loadfile.hpp"
#include "../sound.h"
#include "../oscli.hpp"

// defines

#ifdef USE_SDL12
#define NO_SDL_QUIT	// Do NOT call SDL_Quit if we're using it for graphics too!
#endif

// variables

extern char mus_chan;                      // Number of music channels
extern char sfx_chan;                      // Number of effects channels
extern int  driftlevel;                    // Amount of frequency variation

int sf_volume = 63;
int mu_volume = 90;

int Songs=0;
int Waves=0;
static int SoundOn=0;
//static char *IsPlaying=NULL;
static Mix_Music *IsPlaying=NULL;
static int MusicChannel=-1;

struct SMTab *wavtab;       // Tables of data for each sound and song.
struct SMTab *mustab;       // Filled by script.cc

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

// Public code


int S_Init()
{
int r;

if(SoundOn)
	return 0;

if(SDL_Init(SDL_INIT_AUDIO) < 0) {
	ilog_quiet("SDL: init failed\n");
	return 0;
}

// stereo, 4k buffer
r=Mix_OpenAudio(44100,AUDIO_S16,2,4096);
if(r<0) {
	ilog_quiet("SDL: Mix Audio failed, returned %d\n",r);
	exit(1);
	#ifndef NO_SDL_QUIT
	SDL_Quit();
	#endif
	return 0;
}

SoundOn=1;

// Okay
return 1;
}

void S_Term()
{
if(!SoundOn) {
	return;
}

Mix_CloseAudio();
#ifndef NO_SDL_QUIT
SDL_Quit();
#endif

SoundOn=0;
}

/*
 *      S_Load - Load in the music and samples.
 */

void S_Load()
{
if(!SoundOn) {
	return;
}
LoadMusic();
LoadWavs();
}


/*
 *      S_MusicVolume - Set the music level
 */

void S_MusicVolume(int vol)
{
if(vol < 0) {
	vol=0;
}
if(vol > 100) {
	vol=100;
}
mu_volume = vol;
SetMusicVol(mu_volume);
return;
}


/*
 *      S_SoundVolume - Set the effects level
 */

void S_SoundVolume(int vol)
{
if(vol < 0) {
	vol=0;
}
if(vol > 100) {
	vol=100;
}
sf_volume = vol;
SetSoundVol(sf_volume);
return;
}

/*
 *   S_PlayMusic - play an ogg stream
 */


void S_PlayMusic(char *name)
{
char filename[1024];
char filename2[1024];
long ctr;

if(!SoundOn)
	return;

// Is the music playing?  If not, this will clear IsPlaying
S_IsPlaying();

for(ctr=0;ctr<Songs;ctr++) {
	if(!istricmp(mustab[ctr].name,name)) {
		// If the music is marked Not Present, ignore it (music is optional)
		if(mustab[ctr].fname[0] == 0) {
			return;
		}

		// Is something else playing?
		if(IsPlaying) {
			S_StopMusic();
		}

		// Otherwise, no excuses
//		IsPlaying = mustab[ctr].name; // This is playing now

		if(!loadfile(mustab[ctr].fname,filename)) {
			Bug("S_PlayMusic - could play song %s, could not find file '%s'\n",name,mustab[ctr].fname);
			return;
		}

		IsPlaying=Mix_LoadMUS(filename);
		if(!IsPlaying) {
			// SDL can't access things inside files so we use a bodge
			strcpy(filename2,projectdir);
			strcat(filename2,filename);
			IsPlaying=Mix_LoadMUS(filename2);
			if(!IsPlaying) {
				Bug("S_PlayMusic - could not load song '%s'\n",name);
				return;
			}
		}
		MusicChannel=Mix_PlayMusic(IsPlaying,0);
		if(MusicChannel < 0) {
			Bug("S_PlayMusic - could not play song '%s'\n",name);
			IsPlaying=NULL;
			MusicChannel=-1;
		}
		return;
	}
}

Bug("S_PlayMusic - could not find song '%s'\n",name);
}


/*
 *   S_StopMusic - Stop the current song (if any)
 */

void S_StopMusic()
{
if(IsPlaying) {
	Mix_HaltMusic();
	Mix_FreeMusic(IsPlaying);
	IsPlaying=NULL;
	MusicChannel=-1;
}
}


/*
 *   S_PlaySample - play a sound sample.
 */

void S_PlaySample(char *name, int volume)
{
int freq,num,ctr,chn;

if(!SoundOn) {
	return;
}

for(ctr=0;ctr<Waves;ctr++) {
	if(!istricmp(wavtab[ctr].name,name)) {
		freq=1000;
		if(!wavtab[ctr].nodrift) {
			num = 10 - (rand()%20);
			freq+=(num*driftlevel)/10;
		}
		chn=Mix_PlayChannel(-1,wavtab[ctr].sample,0);
		if(chn>-1) {
			Mix_Volume(chn,volume/2);
//			Mix_SetFreq(chn,freq);
		}
		return;
	}
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

if(!SoundOn) {
	return;
}

ilog_printf("  Checking music.. ");     // This line is not terminated!

for(ctr=0;ctr<Songs;ctr++) {
	if(!loadfile(mustab[ctr].fname,filename)) {
		ilog_quiet("Warning: Could not load music file '%s'\n",mustab[ctr].fname);
		mustab[ctr].fname[0]=0;	// Erase the filename to mark N/A
	}
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

if(!SoundOn) {
	return;
}

// load in each sound

ilog_printf("  Loading sounds");     // This line is not terminated, for the dots

Plot(Waves);    // Work out how many dots to print

for(ctr=0;ctr<Waves;ctr++) {
	if(!loadfile(wavtab[ctr].fname,filename)) {
		ithe_panic("LoadWavs: Cannot open WAV file",wavtab[ctr].fname);
	}
	wavtab[ctr].sample=Mix_LoadWAV(filename);
	if(!wavtab[ctr].sample) {
		// SDL can't access things inside files so we use a bodge
		strcpy(filename2,projectdir);
		strcat(filename2,filename);
		wavtab[ctr].sample=Mix_LoadWAV(filename2);
		if(!wavtab[ctr].sample) {
			ithe_panic("LoadWavs: Invalid WAV file",filename);
		}
		Plot(0);                                        // Print a dot
	}
}

ilog_printf("\n");  // End the line of dots
}



void SetSoundVol(int vol)
{
int ctr;

vol *= 127;
vol /= 100;
for(ctr=0;ctr<256;ctr++) {
	if(ctr != MusicChannel) {
		if(Mix_GetChunk(ctr)) {
			Mix_Volume(ctr,vol);
		}
	}
}
Mix_VolumeMusic((mu_volume*127)/100);
}


void SetMusicVol(int vol)
{
vol *= 127;
Mix_VolumeMusic(vol/100);
}


// Stubs for the music engine

int S_IsPlaying()
{
if(!IsPlaying) {
	return 0; // Definitely not
}

// Not sure, need to ask
return Mix_PlayingMusic();
}


/*
 *   Streaming code
 */


int StreamSong_start(char *filename)
{
IsPlaying=Mix_LoadMUS(filename);
if(IsPlaying) {
	return 1;
}
return 0;
}

/*
 *  Stop the music
 */

void StreamSong_stop()
{
S_StopMusic();
}


#endif
