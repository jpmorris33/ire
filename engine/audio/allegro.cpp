//
//      Sound engine wrappers for Allegro
//

#ifdef USE_ALSOUND

#include <stdlib.h>     // atexit(), random
#include <string.h>     // String splicing routines

#include <allegro.h>
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../loadfile.hpp"
#include "../sound.h"
#include "../types.h"

#ifdef USE_ALOGG
	#include <alogg/alogg.h>
	#include <alogg/aloggpth.h>
	#include <vorbis/vorbisfile.h>
#endif

// defines

// variables

extern char mus_chan;                      // Number of music channels
extern char sfx_chan;                      // Number of effects channels
extern int  driftlevel;                    // Amount of frequency variation

VMINT sf_volume = 63;
VMINT mu_volume = 90;

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

static int StreamSong_GetVoice();

#ifdef USE_ALOGG
	static int ov_iFileClose(void *datasource);
	static size_t ov_iFileRead(void *ptr, size_t size, size_t num, void *datasource);
	static int ov_iFileSeek(void *datasource, ogg_int64_t pos, int whence);
	static long ov_iFileTell(void *datasource);
#endif

// Public code


int S_Init()
{
if(SoundOn) {
	return 0;
}

if(install_sound(DIGI_AUTODETECT,MIDI_NONE,NULL)) {
	return 0;	// Zero success for install_sound
}
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
if(!SoundOn) {
	return;
}

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
long ctr;

if(!SoundOn) {
	return;
}

// Is the music playing?  If not, this will clear IsPlaying
S_IsPlaying();

for(ctr=0;ctr<Songs;ctr++) {
	if(!istricmp(mustab[ctr].name,name)) {
		// If the music is marked Not Present, ignore it (music is optional)
		if(mustab[ctr].fname[0] == 0) {
			return;
		}

		// Is it already playing?
		if(IsPlaying == mustab[ctr].name) {
			return;
		}

		// Is something else playing?
		if(IsPlaying) {
			StreamSong_stop();
		}

		// Otherwise, no excuses
		IsPlaying = mustab[ctr].name; // This is playing now

		if(!loadfile(mustab[ctr].fname,filename)) {
			Bug("S_PlayMusic - could play song %s, could not find file '%s'\n",name,mustab[ctr].fname);
			return;
		}

		if(!StreamSong_start(filename)) {
			Bug("S_PlayMusic - could not play song '%s'\n",name);
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
StreamSong_stop();
}


/*
 *   S_PlaySample - play a sound sample.
 */

void S_PlaySample(char *name, int volume)
{
int freq,num,ctr;

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
		play_sample(wavtab[ctr].sample,volume,128,freq,0); // vol,pan,freq,loop
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
	wavtab[ctr].sample=iload_wav(filename);
	if(!wavtab[ctr].sample) {
		ithe_panic("LoadWavs: Invalid WAV file",filename);
	}
	Plot(0);                                        // Print a dot
}

ilog_printf("\n");  // End the line of dots

}



void SetSoundVol(int vol)
{
int ctr,musvoice;

vol = (vol*255)/100;

// Get channel used by streamer because we don't want to touch that one
musvoice = StreamSong_GetVoice();

for(ctr=0;ctr<256;ctr++) {
	if(ctr != musvoice) {
		voice_set_volume(ctr,vol);
	}
}
}


void SetMusicVol(int vol)
{
int musvoice;

vol = (vol*255)/100;

// Get channel used by streamer
musvoice = StreamSong_GetVoice();
if(musvoice>=0) {
	voice_set_volume(musvoice,vol);
}
}


// Stubs for the music engine

int S_IsPlaying()
{
#ifdef USE_ALOGG
if(!stream || !thread || !alogg_is_thread_alive(thread)) {
	IsPlaying = NULL;
	return 0;
}
#endif

return 1;
}

int StreamSong_start(char *filename)
{
IFILE *ifile;

if(!SoundOn) {
	return 0;
}

#ifdef USE_ALOGG
	ifile = iopen(filename);

	stream = alogg_start_streaming_callbacks((void *)ifile,(struct ov_callbacks *)&ogg_vfs,40960,NULL);
	if(!stream) {
		return 0;
	}
	thread = alogg_create_thread(stream);

	SetMusicVol(mu_volume);

#endif

return 1;
}

void StreamSong_stop()
{
if(!SoundOn) {
	return;
}

IsPlaying = NULL;

#ifdef USE_ALOGG
	if(!stream) {
		return;
	}

	if(thread) {
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

if(!SoundOn) {
	return -1;
}

#ifdef USE_ALOGG
	if(!stream) {
		return  -1;
	}

	if(!thread) {
		return  -1;
	}

	if(!alogg_is_thread_alive(thread)) {
		return  -1;
	}

	// Tranquilise the thread while we mess around with it
	i=alogg_lock_thread(thread);
	if(!i) {
		// Get the underlying audio stream structure
		audiostream=alogg_get_audio_stream(stream);
		if(audiostream) {
			// Set the volume level
			voice = audiostream->voice;
			free_audio_stream_buffer(audiostream);
		}
		// Leave the thread to wake up
		alogg_unlock_thread(thread);
	} else {
		printf("Alogg thread lock error %d\n",i);
	}
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


/* load_wav: Taken from Allegro, made to use my filesystem API
 *  Reads a RIFF WAV format sample file, returning a SAMPLE structure, 
 *  or NULL on error.
 */
SAMPLE *iload_wav(char *filename)
{
IFILE *f;
char buffer[25];
int i;
int length, len;
int freq = 22050;
int bits = 8;
int channels = 1;
signed short s;
SAMPLE *spl = NULL;

*allegro_errno=0;

f = iopen(filename);
if (!f) {
	return NULL;
}

iread((unsigned char *)buffer, 12, f);          /* check RIFF header */
if (memcmp(buffer, "RIFF", 4) || memcmp(buffer+8, "WAVE", 4)) {
	goto getout;
}

while (!ieof(f)) {
	if (iread((unsigned char *)buffer, 4, f) != 4) {
		break;
	}

	length = igetl_i(f);          /* read chunk length */

	if (memcmp(buffer, "fmt ", 4) == 0) {
		i = igetsh_i(f);            /* should be 1 for PCM data */
		length -= 2;
		if (i != 1)  {
			goto getout;
		}

		channels = igetsh_i(f);     /* mono or stereo data */
		length -= 2;
		if ((channels != 1) && (channels != 2)) {
			goto getout;
		}

		freq = igetl_i(f);         /* sample frequency */
		length -= 4;

		igetl_i(f);                /* skip six bytes */
		igetsh_i(f);
		length -= 6;

		bits = igetsh_i(f);         /* 8 or 16 bit data? */
		length -= 2;
		if ((bits != 8) && (bits != 16)) {
			goto getout;
		}
	} else {
		if (memcmp(buffer, "data", 4) == 0) {
			len = length / channels;

			if (bits == 16) {
				len /= 2;
			}

			spl = create_sample(bits, ((channels == 2) ? TRUE : FALSE), freq, len);

			if (spl) { 
				if (bits == 8) {
					iread((unsigned char *)spl->data, length, f);
				} else {
					for (i=0; i<len*channels; i++) {
				  		s = igetsh_i(f);
						((signed short *)spl->data)[i] = s^0x8000;
					}
				}
				length = 0;
			}
		}
	}

	while (length > 0) {             /* skip the remainder of the chunk */
		if (igetc(f) == (unsigned char)EOF) {
			break;
		}
		length--;
	}
}

getout:
iclose(f);
return spl;
}



#endif
