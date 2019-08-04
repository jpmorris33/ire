//
//	No sound driver
//

#ifdef USE_NOSOUND

#include <string.h>
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../loadfile.hpp"
#include "../sound.h"

// defines

// variables

int sf_volume = 63; // Max 255
int mu_volume = 200; // Max 255

struct SMTab *wavtab;       // Tables of data for each sound and song.
struct SMTab *mustab;       // Filled by script.cc

int Songs=0;
int Waves=0;

// functions

void S_Load();              // Load the sound and music
void S_SoundVolume();       // Set the Sound volume
void S_MusicVolume();       // Set the Music volume
void S_PlaySample(char *s,int v);   // Play a sound sample
void S_PlayMusic(char *s);  // Play a song
void S_StopMusic();         // Stop the music

// Public code


int S_Init()
{
return 1; // Already on, say yes
}

void S_Term()
{
}

/*
 *      S_Load - Load in the music and samples.
 */

void S_Load()
{
}


/*
 *      S_MusicVolume - Set the music level
 */

void S_MusicVolume(int vol)
{
return;
}


/*
 *      S_SoundVolume - Set the effects level
 */

void S_SoundVolume(int vol)
{
return;
}


/*
 *   S_PlayMusic - play an ogg stream
 */

void S_PlayMusic(char *name)
{
}


/*
 *   S_PlaySample - play a sound sample.
 */

void S_PlaySample(char *name, int volume)
{
}

//
//  Is is still playing?
//

int S_IsPlaying()
{
return 0;
}

/*
 *  Stop the music from playing
 */

void S_StopMusic()
{
}

int StreamSong_start(char *name)
{
return 0;
}

#endif
