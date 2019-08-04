/*
 *  Which sound library are we going to use?
 */

#ifdef USE_FMOD4
	// FMOD sound library, with full multithreaded Ogg streaming
	// This code isn't stable yet
	#include "audio/fmod4.h"
#endif

#ifdef USE_FMOD3
	// FMOD sound library, with full multithreaded Ogg streaming
	#include "audio/fmod3.h"
#endif

#ifdef USE_ALSOUND
	// Allegro sound library, with optional ALOGG music support
	// For DOS, BeOS and other non-fmod systems.
	#include "audio/allegro.h"
#endif

#ifdef USE_SDLSOUND
	// SDL Mixer
	// For Linux, BeOS, Windows.  Almost as good as FMOD but Free
	#include "audio/sdl.h"
#endif

#ifdef USE_SDL2SOUND
	// SDL Mixer
	// For Linux, BeOS, Windows.  Almost as good as FMOD but Free
	#include "audio/sdl2.h"
#endif

#ifdef USE_NOSOUND
	#include "audio/nosound.h"
#endif

