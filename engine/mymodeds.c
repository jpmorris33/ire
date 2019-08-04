//
//      Customised video mode selection, taken from Allegro
//

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <allegro.h>
#include "tdgui.h"

#define GFX_CANCEL         3
#define GFX_DRIVER_LIST    4
#define GFX_MODE_LIST      5
#define GFX_DEPTH_LIST     6

static _DRIVER_INFO *driver_list;
static int *gfx_card_list;
static int gfx_card_count;

typedef struct GFX_MODE_DATA
{
   int bpp;
   char *s;
} GFX_MODE_DATA;


/* gfx_card_cmp:
 *  qsort() callback for sorting the graphics driver list.
 */
static int gfx_card_cmp(const void *e1, const void *e2)
{
   /* note: drivers don't have to be in this array in order to be listed
    * by the dialog. This is just to control how the list is sorted.
    */
   static int driver_order[] = 
   { 
      #ifdef GFX_AUTODETECT
	 GFX_AUTODETECT,
      #endif
      #ifdef GFX_VESA1
	 GFX_VESA1,
      #endif
      #ifdef GFX_VESA2B
	 GFX_VESA2B,
      #endif
      #ifdef GFX_VESA2L
	 GFX_VESA2L,
      #endif
      #ifdef GFX_VESA3
	 GFX_VESA3,
      #endif
      #ifdef GFX_VBEAF
	 GFX_VBEAF,
      #endif
      #ifdef GFX_VGA
	 GFX_VGA,
      #endif
      #ifdef GFX_MODEX
	 GFX_MODEX,
      #endif
      #ifdef GFX_XTENDED
	 GFX_XTENDED
      #endif
   };

   int d1 = *((int *)e1);
   int d2 = *((int *)e2);
   char *n1, *n2;
   int i;

   if (d1 < 0)
      return -1;
   else if (d2 < 0)
      return 1;

   n1 = ((GFX_DRIVER *)driver_list[d1].driver)->ascii_name;
   n2 = ((GFX_DRIVER *)driver_list[d2].driver)->ascii_name;

   d1 = driver_list[d1].id;
   d2 = driver_list[d2].id;

   for (i=0; i<(int)(sizeof(driver_order)/sizeof(int)); i++) {

      if (d1 == driver_order[i])
	 return -1;
      else if (d2 == driver_order[i])
	 return 1;
   }

//   return stricmp(n1, n2);
   return strcmp(n1, n2);
}



/* setup_card_list:
 *  Fills the list of video cards with info about the available drivers.
 */
static void setup_card_list(int *list)
{
   if (system_driver->gfx_drivers)
      driver_list = system_driver->gfx_drivers();
   else
      driver_list = _gfx_driver_list;

   gfx_card_list = list;
   gfx_card_count = 0;

   while (driver_list[gfx_card_count].driver) {
      gfx_card_list[gfx_card_count] = gfx_card_count;
      gfx_card_count++;
   }

   gfx_card_list[gfx_card_count++] = -1;

   qsort(gfx_card_list, gfx_card_count, sizeof(int), gfx_card_cmp);
}

static GFX_MODE_DATA gfx_mode_data[] =
{
   { 8,  "8 bpp"   },
   { 15, "15 bpp"   },
   { 16, "16 bpp"   },
//   { 24, "24 bpp"   },
   { 32, "32 bpp"   },
   { 0,    NULL     }
};

static char *gfx_mode_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(gfx_mode_data) / sizeof(GFX_MODE_DATA) - 1;
      return NULL;
   }

   return gfx_mode_data[index].s;
}

/* gfx_card_getter:
 *  Listbox data getter routine for the graphics card list.
 */

static char *gfx_card_getter(int index, int *list_size)
{
   int i;

   if (index < 0) {
      if (list_size)
	 *list_size = gfx_card_count;
      return NULL;
   }

   i = gfx_card_list[index];

   if (i >= 0)
      return get_config_text(((GFX_DRIVER *)driver_list[i].driver)->ascii_name);
   else
      return get_config_text("Autodetect");
}

static DIALOG gfx_mode_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
   { d_shadow_box_proc, 0,    8,    304,  166,  0,    0,    0,    0,       0,    0,    "" },
   { d_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    "" },
   { d_button_proc,     196,  113,  100,  16,   0,    0,    0,    D_EXIT,  0,    0,    "OK" },
   { d_button_proc,     196,  135,  100,  16,   0,    0,    27,   D_EXIT,  0,    0,    "Cancel" },
   { d_list_proc,       16,   28,   164,  123,  0,    0,    0,    D_EXIT,  0,    0,    gfx_card_getter },
   { d_list_proc,       196,  28,   100,  75,   0,    0,    0,    D_EXIT,  3,    0,    gfx_mode_getter },
   { NULL }
};

int gfx_mode_select_ire(int *card, int *bpp)
{
   int card_list[256];
   int ret,i;

   clear_keybuf();
   setup_card_list(card_list);

   do {
   } while (mouse_b);

   centre_dialog(gfx_mode_dialog);
	set_dialog_color(gfx_mode_dialog,makecol(0,0,0),makecol(255,255,255));
   ret = do_dialog(gfx_mode_dialog, /*GFX_DRIVER_LIST*/-1);

   i = card_list[gfx_mode_dialog[GFX_DRIVER_LIST].d1];
   if (i >= 0)
      *card = driver_list[i].id;
   else
      *card = GFX_AUTODETECT;

   *bpp = gfx_mode_data[gfx_mode_dialog[GFX_MODE_LIST].d1].bpp;

   if (ret == GFX_CANCEL)
      return FALSE;
   else 
      return TRUE;
}


void ire_alert_mode()
{
alert("Cannot set graphics mode","Please try a different mode",NULL,"OK",NULL,KEY_ENTER,0);
}

