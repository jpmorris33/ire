//
//  Core media initialisation
//

#include <allegro.h>
#include "../../ithelib.h"
#include "../../oscli.hpp"
#include "../iregraph.hpp"

extern "C" int gfx_mode_select_ire(int *card, int *bpp);
extern "C" void ire_alert_mode();


static int ChooseMode();
static char BackendVersion[256];

const char *IRE_Backend()
{
return "Allegro4";
}

const char *IRE_InitBackend()
{
allegro_init();
sprintf(BackendVersion,"Allegro library: %s",ALLEGRO_VERSION_STR);
return BackendVersion;
}


int IRE_Startup(int allow_break, int videomode)
{
int bpp=0;
  
ilog_printf("  Keyboard: ");
if(!install_keyboard())
	ilog_printf("OK\n");
else
	{
	ilog_printf("FAILED\n");
	ilog_printf("%s\n",allegro_error);
	exit(1);
	}

// Set allegro panic button

if(allow_break)
	three_finger_flag=1;
else
	three_finger_flag=0;

ilog_printf("  Timer: ");
if(install_timer() != -1)
	ilog_printf("OK\n");
else
	{
	ilog_printf("FAILED\n");
	ilog_printf("%s\n",allegro_error);
	exit(1);
	}

ilog_printf("  Mouse: ");
if(install_mouse() != -1)
	ilog_printf("OK\n");
else
	{
	ilog_printf("FAILED\n");
	ilog_printf("%s\n",allegro_error);
	exit(1);
	}

ilog_printf("  Video:\n");

switch(videomode)
	{

	// Windowed mode

	case -2:

	// Get colour depth
	bpp = desktop_color_depth();

	// Set it up..
	set_color_depth(bpp);
	set_color_conversion(COLORCONV_TOTAL);

	if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0))
		ChooseMode();
	break;

	// Supported video depths (fullscreen)

	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
	bpp=videomode;

	set_color_depth(bpp);
	set_color_conversion(COLORCONV_TOTAL);
	if(set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0))
		bpp=ChooseMode();
	break;

	// Anything we don't understand

	default:
	bpp=ChooseMode();
	break;
	}

#ifdef WINDOWS_IS_BROKEN
	ilog_quiet("  Attempting to fix windows bug\n");
	screen = create_video_bitmap(640,480);
#endif
	
	IRE_StartGFX(bpp);

return bpp;
}


void IRE_Shutdown()
{
set_gfx_mode(GFX_TEXT,80,25,0,0);
}


void *IRE_SetTimer(void (*timer)(), int clockhz)
{
if(install_int_ex(timer,BPS_TO_TIMER(clockhz)) == 0)
	return (void *)1L; // Hack, since it doesn't return a handle in this implementation
return NULL;
}

void IRE_StopTimer(void *handle, void (*timer)())
{
remove_int(timer);
}


int ChooseMode()
{
int card,ok;
RGB pal[768];

// Choose sensible default BPP

int bpp = desktop_color_depth();

set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);
generate_332_palette(pal);
set_palette(pal);

do  {
	ok=0;
	if (!gfx_mode_select_ire(&card, &bpp))
		{
		allegro_exit();
		exit(1);
		}

	set_color_depth(bpp);
	set_color_conversion(COLORCONV_TOTAL);

	if(!set_gfx_mode(card, 640, 480, 0, 0))
		ok=1;
	else
		{
		set_color_depth(8);
		set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);
		generate_332_palette(pal);
		set_palette(pal);
		ire_alert_mode();
		}
	} while(!ok);
	
return bpp;
}
