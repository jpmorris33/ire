/*
 *      IRE Game resource loader (script language is in the 'pe' directory)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ithelib.h"
#include "oscli.hpp"
#include "console.hpp"
#include "media.hpp"
#include "core.hpp"
#include "textfile.h"
#include "gamedata.hpp"
#include "sound.h"
#include "init.hpp"
#include "object.hpp"
#include "linklist.hpp"

#include "pe/pe_api.hpp" // Compiler API

// defines

//#define TILELINK_OUTPUT       // Not used at present
#define NO_OF_SECTIONS 64       // Up to 64 separate blocks.  Arbitrary.

// Variables

extern int Songs;
extern int Waves;

static struct SectionMap_S              // Map of the blocks in the script
{
	int start;
	int end;
	char name[128];
} sect_map[NO_OF_SECTIONS];

static int no_of_sections;              // Number of blocks in the file
static char Rbuffer[MAX_LINE_LEN];      // Text buffer for LiSP-alikes

static struct TF_S script;              // TextFile loader
static char *parsename;                 // Name of file being parsed

// Functions

void LoadResources(char *fn);           // High-level function, parse a file

static void Purify();                   // Clean the text for parser
static void SeekSect();                 // Map out the blocks

// LiSP and support functions

static void Dump(long lineno,const char *error,const char *help);  // Report script error

// Process the sections

static void Section(char *name);                   // Section dispatcher
static void PreScan(char *name);                   // pre-parse a section

static void sprites(long start,long finish);       // Parse sprites
static void sequences(long start,long finish);     // Parse animation sequences
static void characters(long start,long finish);    // Parse characters
static void tiles(long start,long finish);         // Parse map tile sequences
static void tilelink(long start,long finish);      // Parse map tile connects
static void code(long start,long finish);         // Parse PE code
static void sounds(long start,long finish);        // Parse sounds
static void music(long start,long finish);         // Parse music
static void rooftiles(long start,long finish);     // Parse roof tiles
static void lightmaps(long start,long finish);     // Parse lights
static void tables(long start,long finish);        // Parse tables
static void strings(long start,long finish);        // Parse strings

static void pre_sprites(long start,long finish);       // Parse sprites
static void pre_sequences(long start,long finish);     // Parse animation sequences
static void pre_characters(long start,long finish);    // Parse characters
static void pre_tables(long start,long finish);     	// Parse tables
static void pre_tilelink(long start,long finish);     	// Parse connects
static void pre_strings(long start,long finish);     	// Parse tables

static int CMP_sort_DTI(const void *a,const void *b);
static int CMP_sort_DTS(const void *a,const void *b);
static int CMP_sort_ST(const void *a,const void *b);
static int FindWord(const char *str, const char *find);

/*
 *      Parse - Parse the script
 */

void LoadResources(char *fn)
{
int ctr,gotinclude;

ilog_printf("Reading game resource file..\n");

TF_init(&script);

ilog_printf("  Loading.. ");
TF_load(&script,fn);
ilog_printf("done.  %ld lines\n",script.lines);

parsename = fn;

ilog_quiet("Parsing resource file %s\n",fn);

ilog_printf("  Preprocessing.. ");
ilog_quiet("\n");
do {
	Purify();
	gotinclude=0;
	for(ctr=0;ctr<script.lines;ctr++) {
		char *word;
		word = strfirst(script.line[ctr]);
		if(!stricmp(word, "!include")) {
			gotinclude=1;
			script.line[ctr][0]='#'; // Comment out the inclusion to prevent it being re-run forever
			ilog_quiet("!include %s\n",strrest(script.line[ctr]));
			TF_merge(&script,strrest(script.line[ctr]),ctr);
			break;
		}
	}
} while(gotinclude);
	
ilog_printf("done.  Now %ld lines\n",script.lines);

ilog_printf("  Searching sections.. ");
SeekSect();
ilog_printf("done.  %d sections\n",no_of_sections-1);

ilog_printf("  Parsing.. \n");

// Keep searching for the right sections until we have parsed them all.

PreScan("sprites");
PreScan("sequences");
PreScan("characters");
PreScan("tables");
PreScan("tilelink");
PreScan("strings");

// In order of compilation
Section("sounds");
Section("music");
Section("rooftiles");
Section("lightmaps");
Section("strings");	// These will be needed for the script compiler

Section("scripts");
// Now we have enough to build the fast lookup tables
Init_Lookups();

Section("sprites");
Section("sequences");
Section("tiles");
Section("characters");
Section("tables");
Section("tilelink");

ilog_printf("  Compilation finished.\n");

//script.term();

return;
}

/*
 *      Section - Parse appropriate section, return the number done so far
 */

void Section(char *name)
{
int firstline=0,lastline=0;
char found=0;
int ctr;

for(ctr=0;ctr<no_of_sections;ctr++)
	if(!istricmp(name,sect_map[ctr].name))
		{
		firstline = sect_map[ctr].start;
		lastline = sect_map[ctr].end;
		found=1;
		}

if(!found)
	return;

firstline++;

if(!istricmp(name,"sprites"))
	{
	sprites(firstline,lastline);
	return;
	}

if(!istricmp(name,"sequences"))
	{
	sequences(firstline,lastline);
	return;
	}

if(!istricmp(name,"characters"))
	{
	characters(firstline,lastline);
	return;
	}

if(!istricmp(name,"tiles"))
	{
	tiles(firstline,lastline);
	return;
	}

if(!istricmp(name,"sounds"))
	{
	sounds(firstline,lastline);
	return;
	}

if(!istricmp(name,"music"))
	{
	music(firstline,lastline);
	return;
	}

if(!istricmp(name,"rooftiles"))
	{
	rooftiles(firstline,lastline);
	return;
	}

if(!istricmp(name,"lightmaps"))
	{
	lightmaps(firstline,lastline);
	return;
	}

if(!istricmp(name,"scripts"))
	{
	code(firstline,lastline);
	if(ire_showfuncs)
		{
		FILE *fp;
		fp=fopen("funcs.txt","w");
		if(fp)
			{
			for(ctr=0;ctr<PEtot;ctr++)
				fprintf(fp,"%s\n",PElist[ctr].name);
			fclose(fp);
			KillGFX();
			printf("Function names written to funcs.txt as requested.\n");
			exit(1);
			}
		}
	return;
	}

if(!istricmp(name,"tables"))
	{
	tables(firstline,lastline);
	return;
	}

if(!istricmp(name,"strings"))
	{
	strings(firstline,lastline);
	return;
	}


// Obsolete
if(!istricmp(name,"code"))
	return;

if(!istricmp(name,"tilelink"))
	{
	tilelink(firstline,lastline);
	return;
	}

ilog_quiet("SC_name='%s'\n",name);
Dump(firstline,"Unknown SECTION type.","Valid types are:\r\n SPRITES  SEQUENCES  TILES  CHARACTERS  CODE  TILES\r\n SOUNDS  MUSIC  ROOFTILES  LIGHTMAPS  SCRIPT  TABLES  TILELINK  STRINGS");
return;
}

/*
 *      PreScan - gather names for all sections
 */

void PreScan(char *name)
{
int firstline=0,lastline=0;
char found=0;
int ctr;

for(ctr=0;ctr<no_of_sections;ctr++)
    if(!istricmp(name,sect_map[ctr].name))
        {
        firstline = sect_map[ctr].start;
        lastline = sect_map[ctr].end;
        found=1;
        }

if(!found)
	{
	ilog_quiet("PreScan didn't find '%s'\n",name);
	return;
	}

firstline++;

if(!istricmp(name,"sprites"))
        {
	pre_sprites(firstline,lastline);
        return;
        }

if(!istricmp(name,"sequences"))
	{
	pre_sequences(firstline,lastline);
	return;
	}

if(!istricmp(name,"characters"))
        {
	pre_characters(firstline,lastline);
        return;
        }

if(!istricmp(name,"tiles"))
        {
//	pre_tiles(firstline,lastline);
        return;
        }

if(!istricmp(name,"sounds"))
        {
//	pre_sounds(firstline,lastline);
        return;
        }

if(!istricmp(name,"music"))
        {
        return;
        }

if(!istricmp(name,"rooftiles"))
        {
//	pre_rooftiles(firstline,lastline);
        return;
        }

if(!istricmp(name,"lightmaps"))
        {
//	pre_lightmaps(firstline,lastline);
        return;
        }

if(!istricmp(name,"tables"))
	{
	pre_tables(firstline,lastline);
	return;
	}

if(!istricmp(name,"tilelink"))
	{
	pre_tilelink(firstline,lastline);
	return;
	}

if(!istricmp(name,"strings"))
	{
	pre_strings(firstline,lastline);
	return;
	}

ilog_quiet("PR_name='%s'\n",name);
Dump(firstline,"Unknown SECTION type.","Valid types are:\r\nSPRITES  SEQUENCES  TILES  CHARACTERS  CODE  TILES\r\nSOUNDS  MUSIC  ROOFTILES  LIGHTMAPS  SCRIPT  TABLES  TILELINK  STRINGS");
return;
}

/* ======================== Section parsers =========================== */

// The sprite list is just a simple list, of format:
//
// IDENTIFIER, FILENAME
// IDENTIFIER, FILENAME
//      :    ,    :
//      :    ,    :
//
// Therefore it is sufficient to just get the first two terms and store them.
//

/*
 *      Pre_Sprites - Get the names for the sprites and allocate space
 */

void pre_sprites(long start,long finish)
{
long ctr,pos,imagedata;
const char *Rptr;
char *line;

SPtot = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];        // Get the line
        Rptr = strgetword(line,2);      // If the second word exists,
                                        // then the first word has to as well
        if(Rptr != NOTHING)             // and we assume we have the data
            {
            SPtot++;
            }
        }

// Allocate room on this basis

if(in_editor != 2)
    spr_alloc = SPtot;

ilog_printf("    Creating %d sprite entries ", spr_alloc);

SPlist = (S_POOL *)M_get(sizeof(S_POOL),spr_alloc+1);
ilog_printf("using %d bytes\n", sizeof(S_POOL)*(spr_alloc+1));

// Gather the names and store them

pos = 0;
imagedata=0;

for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // If it exists..
		{
		Rptr=strgetword(line,2);        // Get second term
		if(Rptr!=NOTHING)               // If it exists
			{
			SPlist[pos].fname = (char *)strrest(line);   // Get filename
			strstrip(SPlist[pos].fname);         // Kill whitespc

			// Now get the name as a pointer in the text Block

			SPlist[pos].name=hardfirst(line);
			pos++;  // Go onto the next entry in the sprite list
			}
		}
	}
}

/*
 *      Sprites - Parse the sprites
 */

void sprites(long start,long finish)
{
long ctr,imagedata;

//
//      Most of the grunt-work has already been done, we just need to
//      load in the sprite data for each entry.
//

if(bookview[0]) // Not needed for the book viewer
	return;

ilog_printf("    Load sprites..");

Plot(SPtot);
imagedata=0;

for(ctr=0;ctr<SPtot;ctr++)
	imagedata+=Init_Sprite(ctr);

ilog_printf("\n");
ilog_printf("    Done.  %d bytes of image data allocated\n",imagedata);
}

//      Sequences are a more complex format, but we can count the number
//      of sequences in the script by looking for the word 'name:'

/*
 *      Get the names for all the animation sequences
 */

void pre_sequences(long start,long finish)
{
long ctr,pos;
char *line;
const char *Rptr;

SQtot = 0;
for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);          // First see if it's blank
        if(!istricmp(Rptr,"name"))       // then look for the keyword 'Name'
            SQtot++;                    // Got one
        if(!istricmp(Rptr,"quickname"))  // then look for keyword 'QuickName'
            SQtot++;                    // Got one
        if(!istricmp(Rptr,"oversprite"))  // then look for keyword 'QuickName'
            SQtot++;                    // Got one
        }

// Allocate room on this basis

if(in_editor != 2)
    seq_alloc = SQtot;  // If not in scripter, use original value of seq_alloc

ilog_printf("    Creating %d animation entries ", seq_alloc);

SQlist = (SEQ_POOL *)M_get(sizeof(SEQ_POOL),seq_alloc+1);

ilog_printf("using %d bytes\n", sizeof(SEQ_POOL)*(seq_alloc+1));

// Now, get the name for each entry

pos = 0;
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{
		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Find the offset in the block

				SQlist[pos].name = (char *) strrest(line);   // Get name label
				strstrip(SQlist[pos].name);         // Kill whitespace
				pos++;
				}
			}
		if(!istricmp(Rptr,"quickname"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Grab the name
				SQlist[pos].name = stradd(NULL,Rptr);
				pos++;
				}
			}
		if(!istricmp(Rptr,"oversprite"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Grab the name
				SQlist[pos].name = stradd(NULL,Rptr);
				pos++;
				}
			}
		}
	}
}

/*
 *      Sequences - Parse the animation sequences
 */

void sequences(long start,long finish)
{
long ctr,pos,spos,tmp,list;
char *line;
const char *Rptr;
long liststart=0,listend=0;

if(bookview[0]) // Not needed for the book viewer
	return;

ilog_printf("    Animation sequences");

Plot(SQtot);
pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{

        // The 'NAME' clause.  Big.

                if(!istricmp(Rptr,"name"))       // Got the first one
                    {
		    Rptr=strgetword(line,2);        // Get second term
	            if(Rptr!=NOTHING)           // Make sure its there
                        {
                        // Find the offset in the block

//                        SQlist[pos].name = strrest(line);   // Get name label
//                        strstrip(SQlist[pos].name);         // Kill whitespace
                        }
                    else
                        Dump(ctr,"NA: This sequence has no name",NULL);

                    SQlist[pos].frames=0;
		    SQlist[pos].speed=0;
		    SQlist[pos].flags=0;
		    SQlist[pos].x=0;
		    SQlist[pos].y=0;
		    SQlist[pos].ox=0;
		    SQlist[pos].oy=0;
		    SQlist[pos].translucency=128;
                    SQlist[pos].overlay=NULL;

                    // First, find the boundary of the framelist
                    // so we can calculate the right number of frames

                    liststart=0;
                    listend=0;

                    list = ctr+1;  // Skip the NAME: line
                    do
                        {
                        Rptr = strfirst(script.line[list]);
                        if(!liststart)                  // Don't have start
                            if(!istricmp(Rptr,"framelist:"))
                                liststart = list+1;

                        if(!listend)                    // Don't have end
                            if(!istricmp(Rptr,"end"))
                                listend = list-1;

                        if(!istricmp(Rptr,"name") || list == finish)
                            list = -1;                  // Reached the end

                        list++;
                        } while(list>0);                // list = 0 on exit

                    // Now we should have the boundaries.

                    if(!liststart)
                        Dump(ctr,"NA: This sequence has no FRAMELIST: entry!",SQlist[pos].name);

                    if(!listend)
                        Dump(ctr,"NA: This sequence has no END entry!",SQlist[pos].name);

                    // Good.  Now, we'll count the frames

                    for(list = liststart;list<=listend;list++)
                        {
                        Rptr = strfirst(script.line[list]);
                        if(Rptr != NOTHING)
                            SQlist[pos].frames++;
                        }

                    if(!SQlist[pos].frames)
                        Dump(ctr,"NA: This sequence has no frames at all!",SQlist[pos].name);

                    // Now we have the number of frames in the list

                    SQlist[pos].seq = (S_POOL **)M_get(sizeof(S_POOL *),SQlist[pos].frames+1);

                    // Now we've allocated the space.  We're done I think
                    }

        // The 'FRAMELIST' clause.

                if(!istricmp(Rptr,"framelist:"))       // Got the first one
                    {
                    if(!SQlist[pos].name)
                        Dump(pos,"FL: This sequence has no name",NULL);

                    if(!liststart)
                        Dump(ctr,"FL: This sequence has no FRAMELIST: entry!",SQlist[pos].name);

                    if(!listend)
                        Dump(ctr,"FL: This sequence has no END entry!",SQlist[pos].name);

                    spos=0;

                    for(long list = liststart;list<=listend;list++)
                        {
                        Rptr = strfirst(script.line[list]);
                        if(Rptr != NOTHING)
                            {
                            SQlist[pos].seq[spos] = find_spr(Rptr);
                            if(!SQlist[pos].seq[spos++])
                                Dump(ctr,"FL: Could not find this sprite",Rptr);
                            }
                        }
                    }


        // The 'END' clause.

                if(!istricmp(Rptr,"END"))       // End of the sequence
                    {
                    pos++;
                    Plot(0);
                    }

        // The 'QUICKNAME' clause.

        // QN is a fast-track sequence definition for just one frame.
        // Quicknames are of the form:
        //
        //      quickname  sequence_name sprite_name
        //
        // ..and are therefore much easier for things like map tiles
        // which do not usually have a second frame.

				if(!istricmp(Rptr,"quickname")) // Got the first one
					{
					// For a Quick sequence there is just one frame.
					SQlist[pos].frames=1;
					SQlist[pos].seq = (S_POOL **)M_get(sizeof(S_POOL *),SQlist[pos].frames+1);
						
					// Now we've allocated the frame, find the sprite
					Rptr=strfirst(strrest(strrest(line)));     // Get Third term
					if(Rptr!=NOTHING)                 // Make sure its there
						{
						SQlist[pos].seq[0] = find_spr(Rptr);

						// Did it work?
						if(!SQlist[pos].seq[0])
							Dump(ctr,"QN: Could not find this sprite",Rptr);
						}
					else
						{
						// If there's only one argument, the sprite and sequence are the same name
						Rptr=strfirst(strrest(line));     // Get Second term
						if(Rptr!=NOTHING)                 // Make sure its there
							{
							SQlist[pos].seq[0] = find_spr(Rptr);

							// Did it work?
							if(!SQlist[pos].seq[0])
								Dump(ctr,"QN: Could not find this sprite",Rptr);
							}
						else
							Dump(ctr,"QN: QuickName sequences must be defined as:\r\n QUICKNAME [sequence_name] sprite_name",NULL);
						}

					SQlist[pos].speed=0;
					SQlist[pos].flags=0;
					SQlist[pos].x=0;
					SQlist[pos].y=0;
					SQlist[pos].ox=0;
					SQlist[pos].oy=0;
					SQlist[pos].translucency=128;
					pos++;
					Plot(0);
					Rptr=NOTHING;
					}

        // OverSprite is like quickname, but sprite is always-on-top

				if(!istricmp(Rptr,"oversprite")) // Got the first one
					{
					Rptr=strfirst(strrest(strrest(line)));     // Get Third term
					if(Rptr!=NOTHING)                 // Make sure its there
						{
						// For a Quick sequence there is just one frame.
						SQlist[pos].frames=1;
						SQlist[pos].seq = (S_POOL **)M_get(sizeof(S_POOL *),SQlist[pos].frames+1);

						// Now we've allocated the frame, find the sprite
						SQlist[pos].seq[0] = find_spr(Rptr);
						// Set the overlay the same
						SQlist[pos].overlay = find_spr(Rptr);
						if(!SQlist[pos].overlay)
							Dump(ctr,"OSP: Could not find this sprite",Rptr);

						// Did it work?
						if(!SQlist[pos].seq[0])
							Dump(ctr,"OSP: Could not find this sprite",Rptr);
						}
					else
						Dump(ctr,"OSP: OverSprite sequences must be defined as:\r\n OVERSPRITE sequence_name sprite_name",NULL);

					SQlist[pos].speed=0;
					SQlist[pos].flags=SEQFLAG_POSTOVERLAY;
					SQlist[pos].x=0;
					SQlist[pos].y=0;
					SQlist[pos].ox=0;
					SQlist[pos].oy=0;
					SQlist[pos].translucency=128;

					pos++;
					Plot(0);
					Rptr=NOTHING;
					}

        // Flags

                if(!istricmp(Rptr,"pingpong"))       // Animation will rewind
                    {
                    SQlist[pos].flags|=SEQFLAG_PINGPONG;
		    if(SQlist[pos].frames<2)
			Dump(ctr,"You must not have a pingpong sequence with only one frame!",NULL);
                    }

                if(!istricmp(Rptr,"loop"))             // Animation repeats
                    SQlist[pos].flags|=SEQFLAG_LOOP;
                if(!istricmp(Rptr,"looped"))
                    SQlist[pos].flags|=SEQFLAG_LOOP;
                if(!istricmp(Rptr,"loops"))
                    SQlist[pos].flags|=SEQFLAG_LOOP;

                if(!istricmp(Rptr,"stepped"))          // Animation is stepped
                    SQlist[pos].flags|=SEQFLAG_STEPPED;

				if(!istricmp(Rptr,"random"))           // Animation is random
					{
					SQlist[pos].flags&=~SEQFLAG_ANIMCODE;
					SQlist[pos].flags|=SEQFLAG_RANDOM;
					}

				if(!istricmp(Rptr,"unisync"))          // Animation hardlocked
					{
					SQlist[pos].flags&=~SEQFLAG_ANIMCODE;
					SQlist[pos].flags|=SEQFLAG_UNISYNC;
					SQlist[pos].flags|=SEQFLAG_ASYNC;
					}

                if(!istricmp(Rptr,"onroof"))           // Rooftop
                    SQlist[pos].flags|=SEQFLAG_CHIMNEY;
                if(!istricmp(Rptr,"chimney"))           // Rooftop
                    SQlist[pos].flags|=SEQFLAG_CHIMNEY;

                if(!istricmp(Rptr,"post_overlay"))     // Rooftop
                    SQlist[pos].flags|=SEQFLAG_POSTOVERLAY;

                if(!istricmp(Rptr,"on_top"))           // Rooftop
                    SQlist[pos].flags|=SEQFLAG_POSTOVERLAY;

                if(!istricmp(Rptr,"wall"))     // Wall
                    SQlist[pos].flags|=SEQFLAG_WALL;

        // The 'X' clause.

                if(!istricmp(Rptr,"x"))       // X offset
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].x=strgetnumber(Rbuffer);
                    if(!(strisnumber(Rbuffer)))
			Dump(ctr,"The X offset should be a number.",NULL);
                    }

        // The 'Y' clause.

                if(!istricmp(Rptr,"y"))       // Y offset
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].y=strgetnumber(Rbuffer);
                    if(!(strisnumber(Rbuffer)))
			Dump(ctr,"The Y offset should be a number.",NULL);
                    }

        // The 'OX' clause.

                if(!istricmp(Rptr,"ox"))       // Overlay X offset
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].ox=strgetnumber(Rbuffer);
                    if(!(strisnumber(Rbuffer)))
			Dump(ctr,"The X offset should be a number.",NULL);
                    }

        // The 'OY' clause.

                if(!istricmp(Rptr,"oy"))       // Overlay Y offset
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].oy=strgetnumber(Rbuffer);
                    if(!(strisnumber(Rbuffer)))
			Dump(ctr,"The Y offset should be a number.",NULL);
                    }

        // The 'SPEED' clause.

                if(!istricmp(Rptr,"speed"))       // Animation rate
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].speed=strgetnumber(Rbuffer);
                    if(!(strisnumber(Rbuffer)))
			Dump(ctr,"The speed should be a number.",NULL);
                    }

        // The 'OVERLAY' clause.

                if(!istricmp(Rptr,"overlay"))       // Overlaid frame
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].overlay = find_spr(Rbuffer);
                    // Did it work?
                    if(!SQlist[pos].overlay)
                            Dump(ctr,"OV: Could not find this sprite",Rbuffer);
                    }

        // The 'JUMP_TO' clause, a sequence to switch to when this one ends

                if(!istricmp(Rptr,"jump_to"))       // Sequence to switch to
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
			// Mark that we have a jump
                    SQlist[pos].flags|=SEQFLAG_JUMPTO;
                    // Store the line no we're going to use for post-process
                    // after all sequence definitions have been read in
                    SQlist[pos].jumpaddr = ctr;
                    SQlist[pos].jumpto = NULL;
                    }

        // The 'call' clause, a script to call when the sequence ends

				if(!istricmp(Rptr,"call"))       // Sequence to switch to
					{
					strcpy(Rbuffer,strfirst(strrest(line)));
					SQlist[pos].hookfunc = getnum4PE(Rbuffer);
					if(SQlist[pos].hookfunc < 1 && !editarea[0]) // Don't care for shape editor
						Dump(ctr,"call: Could not find this function",Rbuffer);
					}

        // The 'Translucency' clause.

                if(!istricmp(Rptr,"translucency"))
                    {
                    strcpy(Rbuffer,strfirst(strrest(line)));
                    SQlist[pos].translucency=strgetnumber(Rbuffer);
					if(SQlist[pos].translucency < 0 || SQlist[pos].translucency > 255)
						Dump(ctr,"The translucency should be a number between 0 and 255",NULL);
                    if(!(strisnumber(Rbuffer)))
						Dump(ctr,"The translucency should be a number between 0 and 255",NULL);
                    }

                }
        }

ilog_printf("\n");
ilog_printf("      Aftertouches...\n");

for(ctr=0;ctr<pos;ctr++)
    if(SQlist[ctr].flags&SEQFLAG_JUMPTO)
        {
        strcpy(Rbuffer,strfirst(strrest(script.line[SQlist[ctr].jumpaddr])));
        tmp=getnum4sequence(Rbuffer);
            if(tmp==-1)
                Dump(SQlist[ctr].jumpaddr,"SQJT: Could not find sequence:",Rbuffer);
        SQlist[ctr].jumpto = &SQlist[tmp];
        }
}

/*
 *      Pre-Characters - Get the character names
 */

void pre_characters(long start,long finish)
{
int ctr,pos;
char *line;
const char *Rptr;

// Values for SETSOLID


CHtot = 0;

//      Count the instances of 'Name' to get the quantity of characters.
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);             // First see if it's blank
	if(!istricmp(Rptr,"name"))       // then look for the word 'Name'
		CHtot++;                    // Got one
	if(!istricmp(Rptr,"alias"))       // also allow for aliases
		CHtot++;                    // Got one
	}

ilog_printf("    Creating %d characters ", CHtot);

if(in_editor != 2)
    chr_alloc = CHtot;  // If not in scripter, use original value

CHlist = (OBJECT *)M_get(sizeof(OBJECT),chr_alloc+1);
ilog_printf("using %d bytes\n", sizeof(OBJECT)*(chr_alloc+1));

// Extract the names

pos = -1;
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{
		if(!istricmp(Rptr,"name") || !istricmp(Rptr,"alias"))       // Got the first one
			{
			Rptr=strfirst(strrest(line));     // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				pos ++;                             // Pre-increment
				CHlist[pos].name = (char *)strrest(line);      // Get name label
				strstrip(CHlist[pos].name);         // Kill whitespace
				if(strlen(CHlist[pos].name) > 31)   // Check name length
					if(istricmp(strfirst(line),"alias"))	// Ignore if it's an alias
						Dump(ctr,"This name exceeds 31 letters",CHlist[pos].name);
				}
			}
		}
	}
}

/*
 *      Characters - Parse the characters
 */

void characters(long start,long finish)
{
int ctr,tmp,pos;
char *line;
const char *Rptr;
char *ptr;
unsigned char polarity,blockx,blocky,blockw,blockh;
int hour,minute;
// Values for SETSOLID

if(bookview[0]) // Not needed for the book viewer
	return;

ilog_printf("    Setting up characters");

Plot(CHtot);
pos = -1;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{
        // Get the name

		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strfirst(strrest(line));     // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Find the offset in the block

				Plot(0);
				pos ++;                             // Pre-increment
				CHlist[pos].personalname= (char *)M_get(32,1);
				CHlist[pos].maxstats = (STATS *)M_get(sizeof(STATS),1);
				CHlist[pos].stats = (STATS *)M_get(sizeof(STATS),1);
				CHlist[pos].funcs = (FUNCS *)M_get(sizeof(FUNCS),1);
				CHlist[pos].user = (USEDATA *)M_get(sizeof(USEDATA),1);
				CHlist[pos].labels = (CHAR_LABELS *)M_get(1,sizeof(CHAR_LABELS));
				CHlist[pos].labels->rank=NOTHING;
				CHlist[pos].labels->race=NOTHING;
				CHlist[pos].labels->party=NOTHING;
				CHlist[pos].labels->location=NOTHING;

				CHlist[pos].funcs->ucache=-1;
				CHlist[pos].funcs->tcache=-1;
				CHlist[pos].funcs->kcache=-1;
				CHlist[pos].funcs->lcache=-1;
				CHlist[pos].funcs->scache=-1;
				CHlist[pos].funcs->hcache=-1;
				CHlist[pos].funcs->icache=-1;
				CHlist[pos].funcs->wcache=-1;
				CHlist[pos].funcs->hrcache=-1;
				CHlist[pos].funcs->qcache=-1;
				CHlist[pos].funcs->gcache=-1;

				CHlist[pos].vblock[BLK_X]=0;   // Partly-solid object
				CHlist[pos].vblock[BLK_Y]=0;   // Solidity offset
				CHlist[pos].vblock[BLK_W]=0;   // and size
				CHlist[pos].vblock[BLK_H]=0;
				CHlist[pos].hblock[BLK_X]=0;   // Partly-solid object
				CHlist[pos].hblock[BLK_Y]=0;   // Solidity offset
				CHlist[pos].hblock[BLK_W]=0;   // and size
				CHlist[pos].hblock[BLK_H]=0;
//				CHlist[pos].behave = -1;            // No behaviour
//				CHlist[pos].stats->oldbehave = -1;  // No behaviour
				CHlist[pos].stats->tick=0;
				CHlist[pos].stats->owner.objptr=NULL;
				CHlist[pos].user->npctalk=NULL;
				CHlist[pos].target.objptr=NULL;
				CHlist[pos].activity=-1;

				strcpy(CHlist[pos].personalname,"-");
				CHlist[pos].desc = "No description";
				CHlist[pos].shortdesc = CHlist[pos].desc;
				// By default the thing is resurrected to be itself
				strcpy(CHlist[pos].funcs->resurrect,CHlist[pos].name);

				OB_Funcs(&CHlist[pos]);				// Set up the strings
				continue;
				}
			else
				Dump(ctr,"NA: This character has no name",NULL);
			}

		// Allow copies/inherticance from existing objects

		if(!istricmp(Rptr,"alias"))       // Copy existing object
			{
			Rptr=strgetword(line,3);     //  Check equals is there
			if(istricmp(Rptr,"="))
				Dump(ctr,"Syntax is: ALIAS newname = srcname",NULL);

			Rptr=strgetword(line,4);     // Get oldname
			if(Rptr==NOTHING)           // Make sure its there
				Dump(ctr,"Syntax is: ALIAS newname = srcname",NULL);

			// Find the source object
			OBJECT *src=NULL;
			for(int i=0;i<=pos;i++)
				if(!stricmp(CHlist[i].name,Rptr))	{
					src=&CHlist[i];
					break;
					}

			if(!src)
				Dump(ctr,"NA: Didn't find source character:",Rptr);
			Rptr=strgetword(line,1);	// Got the name
			
			Plot(0);
			pos ++;                             // Pre-increment

			memcpy(&CHlist[pos],src,sizeof(CHlist[pos]));
			// Now cut up the line so we can get just the first term
			char *cutptr = (char *)strrest(line); // Dodgy, but safe in this case
			strstrip(cutptr);
			CHlist[pos].name = hardfirst(cutptr);	// Truncates original string
			strstrip(CHlist[pos].name);

			// Now redo the pointers
			CHlist[pos].personalname= (char *)M_get(32,1);
			CHlist[pos].maxstats = (STATS *)M_get(sizeof(STATS),1);
			CHlist[pos].stats = (STATS *)M_get(sizeof(STATS),1);
			CHlist[pos].funcs = (FUNCS *)M_get(sizeof(FUNCS),1);
			CHlist[pos].user = (USEDATA *)M_get(sizeof(USEDATA),1);
			CHlist[pos].labels = (CHAR_LABELS *)M_get(1,sizeof(CHAR_LABELS));

			strcpy(CHlist[pos].personalname,src->personalname);
			memcpy(CHlist[pos].maxstats,src->maxstats,sizeof(STATS));
			memcpy(CHlist[pos].stats,src->stats,sizeof(STATS));
			memcpy(CHlist[pos].funcs,src->funcs,sizeof(FUNCS));
			memcpy(CHlist[pos].user,src->user,sizeof(USEDATA));
			memcpy(CHlist[pos].labels,src->labels,sizeof(CHAR_LABELS));

			OB_Funcs(&CHlist[pos]);				// Set up the strings
			continue;
			}

        // Sanity Check
		if(pos == -1)
			Dump(ctr,"NA: The character's name must come first",NULL);

        // Get the vital statistics

		if(!istricmp(Rptr,"hp"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's health must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->hp=tmp;
//			CHlist[pos].stats->oldhp=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_hp"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's maximum health must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->hp=tmp;
			continue;
			}

		if(!istricmp(Rptr,"dexterity"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's dexterity must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->dex=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_dexterity"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's maximum dexterity must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->dex=tmp;
			continue;
			}

		if(!istricmp(Rptr,"strength"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's strength must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->str=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_strength"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's maximum strength must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->str=tmp;
			continue;
			}

		if(!istricmp(Rptr,"intelligence"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's intelligence must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->intel=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_intelligence"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's maximum intelligence must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->intel=tmp;
			continue;
			}

		if(!istricmp(Rptr,"weight"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's weight must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->weight=tmp;
			continue;
			}

		if(!istricmp(Rptr,"unit_weight") || !istricmp(Rptr,"max_weight"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's unit weight must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->weight=tmp;
			continue;
			}

		if(!istricmp(Rptr,"damage"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The damage it causes must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->damage=tmp;
			continue;
			}

		if(!istricmp(Rptr,"armour") || !istricmp(Rptr,"armor"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The armour value must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->armour=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_damage")) // is this needed?
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The maximum damage it causes must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->damage=tmp;
			continue;
			}

		// is this needed
		if(!istricmp(Rptr,"max_armour") || !istricmp(Rptr,"max_armor"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The maximum damage it causes must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->armour=tmp;
			continue;
			}

		if(!istricmp(Rptr,"karma"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The karma it adds/subtracts must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->karma=tmp;
			continue;
			}

		if(!istricmp(Rptr,"bulk"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The object's bulk must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->bulk=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_bulk"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The object's bulk capacity must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->bulk=tmp;
			continue;
			}

		if(!istricmp(Rptr,"range"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The weapon's range must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->range=tmp;
			continue;
			}

		if(!istricmp(Rptr,"max_range"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The weapon's range must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->range=tmp;
			continue;
			}

		if(!istricmp(Rptr,"speed"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The object's speed must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].maxstats->speed=tmp; // Speed
			continue;
			}

		if(!istricmp(Rptr,"level"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The object's level must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->level=tmp; // Speed
			continue;
			}

		if(!istricmp(Rptr,"proximity"))
			{
			// Set default egg distance
			CHlist[pos].stats->radius = EggDistance;
			continue;
			}

		if(!istricmp(Rptr,"radius"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The object's radius must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->radius = tmp;
			continue;
			}

		if(!istricmp(Rptr,"alignment"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The character's alignment should be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			CHlist[pos].stats->alignment=tmp;
			continue;
			}

		if(!istricmp(Rptr,"frame"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=getnum4sequence(Rbuffer);
			if(tmp==-1)
				Dump(ctr,"Could not find sequence:",Rbuffer);
			CHlist[pos].dir[CHAR_L]=tmp;
			CHlist[pos].dir[CHAR_R]=tmp;
			CHlist[pos].dir[CHAR_U]=tmp;
			CHlist[pos].dir[CHAR_D]=tmp;
			continue;
			}

		if(!istricmp(Rptr,"left"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=getnum4sequence(Rbuffer);
			if(tmp==-1)
				Dump(ctr,"Could not find sequence:",Rbuffer);
			CHlist[pos].dir[CHAR_L]=tmp;
			continue;
			}

		if(!istricmp(Rptr,"right"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=getnum4sequence(Rbuffer);
			if(tmp==-1)
				Dump(ctr,"Could not find sequence:",Rbuffer);
			CHlist[pos].dir[CHAR_R]=tmp;
			continue;
			}

		if(!istricmp(Rptr,"up"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=getnum4sequence(Rbuffer);
			if(tmp==-1)
				Dump(ctr,"Could not find sequence:",Rbuffer);
			CHlist[pos].dir[CHAR_U]=tmp;
			continue;
			}

		if(!istricmp(Rptr,"down"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=getnum4sequence(Rbuffer);
			if(tmp==-1)
				Dump(ctr,"Could not find sequence:",Rbuffer);
			CHlist[pos].dir[CHAR_D]=tmp;
			continue;
			}

        // Get a text description

		if(!istricmp(Rptr,"description"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));

			if(Rbuffer[0] == '\"')  // Quoted text is direct
				{
				CHlist[pos].desc = strchr(line,'\"'); // start
				CHlist[pos].desc++;            // skip the quote
				ptr = strrchr(line,'\"');     // Find last quote
				if(ptr)
					*ptr = 0;                 // blow quote away
				}
			else                    // Look up unquoted text
				Dump(ctr,"Invalid string",Rbuffer);
			continue;
			}

        // Get the short description

		if(!istricmp(Rptr,"short"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(Rbuffer[0] == '\"')  // Quoted text is direct
				{
				CHlist[pos].shortdesc = strchr(line,'\"'); // start
				CHlist[pos].shortdesc++;       // skip the quote
				ptr = strrchr(line,'\"');     // Find last quote
				if(ptr)
					*ptr = 0;                 // blow quote away
				}
			else                    // Look up unquoted text
				Dump(ctr,"Invalid string",Rbuffer);
			continue;
			}

		if(!istricmp(Rptr,"race"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(Rbuffer[0] == '\"')  // Quoted text
				{
				CHlist[pos].labels->race = strchr(line,'\"'); // start
				CHlist[pos].labels->race++;       // skip the quote
				ptr = strrchr(line,'\"');     // Find last quote
				if(ptr)
					*ptr = 0;                 // blow quote away
				}
			else                    // unquoted text
				CHlist[pos].labels->race = (char *)strrest(line);
			continue;
			}

		if(!istricmp(Rptr,"rank"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(Rbuffer[0] == '\"')  // Quoted text
				{
				CHlist[pos].labels->rank = strchr(line,'\"'); // start
				CHlist[pos].labels->rank++;       // skip the quote
				ptr = strrchr(line,'\"');     // Find last quote
				if(ptr)
					*ptr = 0;                 // blow quote away
				}
			else                    // unquoted text
				CHlist[pos].labels->rank = (char *)strrest(line);
			continue;
			}

		if(!istricmp(Rptr,"party"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(Rbuffer[0] == '\"')  // Quoted text
				{
				CHlist[pos].labels->party = strchr(line,'\"'); // start
				CHlist[pos].labels->party++;       // skip the quote
				ptr = strrchr(line,'\"');     // Find last quote
				if(ptr)
					*ptr = 0;                 // blow quote away
				}
			else                    // unquoted text
				CHlist[pos].labels->party = (char *)strrest(line);
			continue;
			}

		if(!istricmp(Rptr,"location"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(Rbuffer[0] == '\"')  // Quoted text
				{
				CHlist[pos].labels->location = strchr(line,'\"'); // start
				CHlist[pos].labels->location++;       // skip the quote
				ptr = strrchr(line,'\"');     // Find last quote
				if(ptr)
					*ptr = 0;                 // blow quote away
				}
			else                    // unquoted text
				CHlist[pos].labels->location = (char *)strrest(line);
			continue;
			}

        // Get the character's USE function

		if(!istricmp(Rptr,"IfUsed"))
			{
			strcpy(CHlist[pos].funcs->use,strfirst(strrest(line)));
			continue;
			}

        // Get the character's behaviour function

		if(!istricmp(Rptr,"behave") || !istricmp(Rptr,"behaviour")
		|| !istricmp(Rptr,"behavior"))
			{
			// If we have a behaviour already, reject the second one
			// unless it is the dummy one to make ifTriggered work
			if(CHlist[pos].activity != -1)
				Dump(ctr,"Multiple initial behaviours not supported",NULL);
			CHlist[pos].activity = getnum4PE(strfirst(strrest(line)));
			if(CHlist[pos].activity == -1)
				Dump(ctr,"Cannot find script function",strfirst(strrest(line)));
			continue;
			}

        // Get the character's STAND function

		if(!istricmp(Rptr,"IfTriggered"))
			{
			strcpy(CHlist[pos].funcs->stand,strfirst(strrest(line)));
			CHlist[pos].flags |= IS_TRIGGER;
			continue;
			}

        // Get the character's KILLED function

		if(!istricmp(Rptr,"IfKilled") || !istricmp(Rptr,"IfDead"))
			{
			strcpy(CHlist[pos].funcs->kill,strfirst(strrest(line)));
			continue;
			}

                // Get the character's LOOK function

		if(!istricmp(Rptr,"IfLooked") || !istricmp(Rptr,"IfLookedAt")
		||!istricmp(Rptr,"IfLook"))
			{
			strcpy(CHlist[pos].funcs->look,strfirst(strrest(line)));
			continue;
			}

        // Get the character's HURT function

		if(!istricmp(Rptr,"IfHurt") || !istricmp(Rptr,"IfDamaged"))
			{
			strcpy(CHlist[pos].funcs->hurt,strfirst(strrest(line)));
			continue;
			}

        // Get the character's INIT function

		if(!istricmp(Rptr,"Init") || !istricmp(Rptr,"OnInit"))
			{
			strcpy(CHlist[pos].funcs->init,strfirst(strrest(line)));
			continue;
			}

        // Get the character's WIELD function

		if(!istricmp(Rptr,"IfWield") || !istricmp(Rptr,"IfWielded"))
			{
			strcpy(CHlist[pos].funcs->wield,strfirst(strrest(line)));
			continue;
			}

        // Get the character's Horrified function

		if(!istricmp(Rptr,"IfHorrified"))
			{
			strcpy(CHlist[pos].funcs->horror,strfirst(strrest(line)));
			continue;
			}

        // Get the character's Quantity-Change function

		if(!istricmp(Rptr,"qchange") || !istricmp(Rptr,"QuantityChange"))
			{
			strcpy(CHlist[pos].funcs->quantity,strfirst(strrest(line)));
			continue;
			}

        // Get the character's Get function

		if(!istricmp(Rptr,"IfGet") || !istricmp(Rptr,"IfGot")
		||!istricmp(Rptr,"IfTaken"))
			{
			strcpy(CHlist[pos].funcs->get,strfirst(strrest(line)));
			continue;
			}

        // Get the character's resurrection object, if special

		if(!istricmp(Rptr,"resurrect") || !istricmp(Rptr,"resurrect_to")
		|| !istricmp(Rptr,"resurrect_as"))
			{
			strcpy(CHlist[pos].funcs->resurrect,strfirst(strrest(line)));
			continue;
			}

		if(!istricmp(Rptr,"no_resurrect"))
			{
			strcpy(CHlist[pos].funcs->resurrect,"-");
			continue;
			}

        // Get the character's ATTACK function

		if(!istricmp(Rptr,"Attack"))
			{
			strcpy(CHlist[pos].funcs->attack,strfirst(strrest(line)));
			continue;
			}

		if(!istricmp(Rptr,"user1"))
			{
			strcpy(CHlist[pos].funcs->user1,strfirst(strrest(line)));
			continue;
			}

		if(!istricmp(Rptr,"user2"))
			{
			strcpy(CHlist[pos].funcs->user2,strfirst(strrest(line)));
			continue;
			}

		// Get the character's light level
		// Invert it to bring in line with my bizarre light physics

		if(!istricmp(Rptr,"light"))
			{
			CHlist[pos].light = strgetnumber(strfirst(strrest(line)));
			continue;
			}

		// Is the character solid only in places?
		// If so, find out which part is solid and how big it is

		if(!istricmp(Rptr,"setsolid")
		||!istricmp(Rptr,"subsolid"))
			{
//                        CHlist[pos].flags.solid = 1;  // Make sure it's solid
			polarity=0;
			strcpy(Rbuffer,strgetword(line,2));
			if(!istricmp(Rbuffer,"H"))
				polarity=1;
			if(!istricmp(Rbuffer,"V"))
				polarity=2;
			if(!polarity)
				Dump(ctr,"\r\nPartly-solid objects must be declared with a polarity and four numbers:\r\nSETSOLID [H or V] <x offset> <y offset> <width> <height>",Rbuffer);

			strcpy(Rbuffer,strgetword(line,3));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nPartly-solid objects must be declared with a polarity and four numbers:\r\nSETSOLID [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blockx= strgetnumber(Rbuffer);
			strcpy(Rbuffer,strgetword(line,4));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nPartly-solid objects must be declared with a polarity and four numbers:\r\nSETSOLID [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blocky= strgetnumber(Rbuffer);

			strcpy(Rbuffer,strgetword(line,5));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nPartly-solid objects must be declared with a polarity and four numbers:\r\nSETSOLID [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blockw= strgetnumber(Rbuffer);

			strcpy(Rbuffer,strgetword(line,6));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nPartly-solid objects must be declared with a polarity and four numbers:\r\nSETSOLID [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blockh= strgetnumber(Rbuffer);

			if(polarity == 1)
				{
				CHlist[pos].hblock[BLK_X] = blockx;
				CHlist[pos].hblock[BLK_Y] = blocky;
				CHlist[pos].hblock[BLK_W] = blockw;
				CHlist[pos].hblock[BLK_H] = blockh;
				}
			else
				{
				CHlist[pos].vblock[BLK_X] = blockx;
				CHlist[pos].vblock[BLK_Y] = blocky;
				CHlist[pos].vblock[BLK_W] = blockw;
				CHlist[pos].vblock[BLK_H] = blockh;
				}
			continue;
			}

		// Is the character smaller than it's physical appearance?
		// If so, find out which part is active and how big it is

		if(!istricmp(Rptr,"setarea")
		||!istricmp(Rptr,"setactive")
		||!istricmp(Rptr,"setactivearea"))
			{
			polarity=0;
			strcpy(Rbuffer,strgetword(line,2));
			if(!istricmp(Rbuffer,"H"))
				polarity=1;
			if(!istricmp(Rbuffer,"V"))
				polarity=2;
			if(!polarity)
				Dump(ctr,"\r\nObjects with Active Areas must be declared with a polarity and four numbers:\r\nSetActiveArea [H or V] <x offset> <y offset> <width> <height>",Rbuffer);

			strcpy(Rbuffer,strgetword(line,3));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nObjects with Active Areas must be declared with a polarity and four numbers:\r\nSetActiveArea [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blockx= strgetnumber(Rbuffer);

			strcpy(Rbuffer,strgetword(line,4));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nObjects with Active Areas must be declared with a polarity and four numbers:\r\nSetActiveArea [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blocky= strgetnumber(Rbuffer);

			strcpy(Rbuffer,strgetword(line,5));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nObjects with Active Areas must be declared with a polarity and four numbers:\r\nSetActiveArea [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blockw= strgetnumber(Rbuffer);

			strcpy(Rbuffer,strgetword(line,6));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"\r\nObjects with Active Areas must be declared with a polarity and four numbers:\r\nSetActiveArea [H or V] <x offset> <y offset> <width> <height>",Rbuffer);
			blockh= strgetnumber(Rbuffer);

			if(polarity == 1)
				{
				CHlist[pos].harea[BLK_X] = blockx;
				CHlist[pos].harea[BLK_Y] = blocky;
				CHlist[pos].harea[BLK_W] = blockw;
				CHlist[pos].harea[BLK_H] = blockh;
				}
			else
				{
				CHlist[pos].varea[BLK_X] = blockx;
				CHlist[pos].varea[BLK_Y] = blocky;
				CHlist[pos].varea[BLK_W] = blockw;
				CHlist[pos].varea[BLK_H] = blockh;
				}
			continue;
			}

		// Set a default schedule item (useful for timed eggs)

		if(!istricmp(Rptr,"schedule"))
			{
			SCHEDULE *sched;
			if(in_editor == 1)  // Not in the world editor or they will accrue
				continue;
			if(!CHlist[pos].schedule)
				{
				// Initialise schedule if necessary
				CHlist[pos].schedule = (SCHEDULE *)M_get(24,sizeof(SCHEDULE));
				}

			// This makes things less annoying 
			sched = CHlist[pos].schedule;

			// Get time
			strcpy(Rbuffer,strfirst(strrest(line)));
			Rbuffer[5]=0; // Should be NN:NN\0 - this will make sure
			if(Rbuffer[2] != ':')
				Dump(ctr,"Schedule syntax:  schedule <hh:mm> <script function>",NULL);
			sscanf(Rbuffer,"%02d:%02d",&hour,&minute);

			tmp = getnum4PE(strfirst(strrest(strrest(line))));
			if(tmp == -1)
				Dump(ctr,"Cannot find script function",strfirst(strrest(strrest(line))));

			blockh=0;
			for(blockw=0;blockw<24;blockw++)
				if(sched[blockw].active == 0)
					{
					sched[blockw].hour=hour;
					sched[blockw].minute=minute;
					sched[blockw].call=tmp;
					SAFE_STRCPY(sched[blockw].vrm,PElist[tmp].name);
					sched[blockw].target.objptr=NULL;
					sched[blockw].active=1;
					blockh=1;
					break;
					}

			if(!blockh)
				Dump(ctr,"Too many schedule items (max 24)",NULL);
			continue;
			}


		// Get the flags

		if(!istricmp(Rptr,"solid"))             // Is it solid?
			{
			CHlist[pos].flags |= IS_SOLID;
			continue;
			}

		if(!istricmp(Rptr,"will_open"))         // Isn't always solid
			{
			CHlist[pos].flags |= CAN_OPEN;
			continue;
			}

		if(!istricmp(Rptr,"blocklight") ||
		!istricmp(Rptr,"blockslight"))        //Is it solid?
			{
			CHlist[pos].flags |= DOES_BLOCKLIGHT;
			continue;
			}

		if(!istricmp(Rptr,"invisible"))             // Is it invisible?
			{
			CHlist[pos].flags |= IS_INVISIBLE;
			continue;
			}

		if(!istricmp(Rptr,"semivisible")) { // Player can see even if invisible
			CHlist[pos].flags |= IS_SEMIVISIBLE;
			continue;
		}

		if(!istricmp(Rptr,"system"))               // Is it a system object?
			{
			CHlist[pos].flags |= IS_SYSTEM;
			continue;
			}

		if(!istricmp(Rptr,"hidden"))             // Is it Hidden?
			{
			CHlist[pos].flags |= IS_SHADOW|IS_INVISIBLE;
			continue;
			}

		if(!istricmp(Rptr,"translucent"))           // Is it translucent?
			{
			CHlist[pos].flags |= IS_TRANSLUCENT;
			continue;
			}

		if(!istricmp(Rptr,"post_overlay"))     // Are overlays postprojected?
			Dump(ctr,"Post Overlay is moved to Section: Sequences",Rptr);

		if(!istricmp(Rptr,"window"))           // Is it a window?
			{
			CHlist[pos].flags |= IS_WINDOW|DOES_BLOCKLIGHT;
			continue;
			}

		if(!istricmp(Rptr,"fixed"))             // Can't move/get it?
			{
			CHlist[pos].flags |= IS_FIXED;
			continue;
			}

		if(!istricmp(Rptr,"container"))         // Is a bag or chest?
			{
			CHlist[pos].flags |= IS_CONTAINER;
			continue;
			}

		if(!istricmp(Rptr,"wielded"))           // Can it be wielded?
			{
			CHlist[pos].flags |= CAN_WIELD;
			continue;
			}

		if(!istricmp(Rptr,"tabletop"))           // Can you drop things
			{                                   // On this solid object
			CHlist[pos].flags |= IS_TABLETOP|IS_SOLID;
			continue;
			}

		if(!istricmp(Rptr,"spikeproof"))         // unaffected by triggers
			{
			CHlist[pos].flags |= IS_SPIKEPROOF;
			continue;
			}

		if(!istricmp(Rptr,"person"))          // can't be stuffed in sacks etc
			{
			CHlist[pos].flags |= IS_PERSON;
			continue;
			}

		if(!istricmp(Rptr,"quantity"))        // Can it be a pile?
			{
			CHlist[pos].flags |= IS_QUANTITY;
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=strgetnumber(Rbuffer);
			if(strisnumber(Rbuffer))
				CHlist[pos].stats->quantity=tmp;
			else
				CHlist[pos].stats->quantity=1;
			continue;
			}

		if(!istricmp(Rptr,"boat"))            // Is it a boat?
			{
			CHlist[pos].flags |= IS_WATER;
			continue;
			}

		if(!istricmp(Rptr,"shadow"))           // Is it like a shadow?
			{
			CHlist[pos].flags |= IS_SHADOW;
			continue;
			}

		if(!istricmp(Rptr,"decorative"))       // Is it a decor object?
			{
			// For the world editor, pretend it is NOT a decor object.
			if(in_editor != 1)
				CHlist[pos].flags |= IS_DECOR;
			CHlist[pos].user->edecor=1; // Mark as decorative for editor
			continue;
			}

		if(!istricmp(Rptr,"horrific"))   // Does it makes NPCs shit themselves?
			{
			CHlist[pos].flags |= IS_HORRIBLE;
			continue;
			}

		// Get the character's speech file

		if(!istricmp(Rptr,"Conversation"))
			{
			strcpy(CHlist[pos].funcs->talk,strfirst(strrest(line)));
			CHlist[pos].funcs->tcache=1;
			continue;
			}

		// Get the character's default contents

		if(!istricmp(Rptr,"contains"))
			{
			if(CHlist[pos].funcs->contents<8)
				strcpy(CHlist[pos].funcs->contains[CHlist[pos].funcs->contents++],strfirst(strrest(line)));
			else
				Dump(ctr,"This character has more than 8 initial items",Rbuffer);
			continue;
			}

		if(!istricmp(Rptr,"fragile"))             // Is it fragile?
			{
			CHlist[pos].flags |= IS_FRAGILE;
			continue;
			}

		if(!istricmp(Rptr,"onroof")||!istricmp(Rptr,"on_roof")||
		!istricmp(Rptr,"chimney"))             // On the roof?
			Dump(ctr,"OnRoof/Chimney is moved to Section: Sequences",Rptr);

		// NPC flags

		if(!istricmp(Rptr,"male"))           // Is it male? (default)
			{
			CHlist[pos].stats->npcflags &= ~IS_FEMALE;
			continue;
			}

		if(!istricmp(Rptr,"female"))           // Is it female?
			{
			CHlist[pos].stats->npcflags |= (IS_FEMALE & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"know_name"))      // Player know their name
			{
			CHlist[pos].stats->npcflags |= (KNOW_NAME & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"is_hero"))      // Player know their name
			{
			CHlist[pos].stats->npcflags |= (IS_HERO & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"cant_eat"))      // Can't eat or drink
			{
			CHlist[pos].stats->npcflags |= (CANT_EAT & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"cant_drink"))    // Can't eat or drink
			{
			CHlist[pos].stats->npcflags |= (CANT_EAT & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"wont_shut_doors")) // Can't shut doors
			{
			CHlist[pos].stats->npcflags |= (NOT_CLOSE_DOOR & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"wont_open_doors")) // Can't open doors
			{
			CHlist[pos].stats->npcflags |= (NOT_OPEN_DOOR & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"no_schedule")) // Can't open doors
			{
			CHlist[pos].stats->npcflags |= (NO_SCHEDULE & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"link_to_npc")
		|| !istricmp(Rptr,"link_to_other_npc")
		|| !istricmp(Rptr,"link_to_another_npc")
		|| !istricmp(Rptr,"symlink"))
			{
			CHlist[pos].stats->npcflags |= (IS_SYMLINK & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"biological")) // Flesh and blood
			{
			CHlist[pos].stats->npcflags |= (IS_BIOLOGICAL & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"robot")	// Robotic
		|| !istricmp(Rptr,"robotic")
		|| !istricmp(Rptr,"is_robot")
		|| !istricmp(Rptr,"is_robotic"))
			{
			CHlist[pos].stats->npcflags |= (IS_ROBOT & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"guard")) // It's the cops!
			{
			CHlist[pos].stats->npcflags |= (IS_GUARD & STRIPNPC);
			continue;
			}

		if(!istricmp(Rptr,"spawned"))
			{
			CHlist[pos].stats->npcflags |= (IS_SPAWNED & STRIPNPC);
			continue;
			}

		// If we get this far, there was no recognised keyword

		if(!skip_unk)
			Dump(ctr,"Unknown keyword:",Rptr);

		}
	}

ilog_printf("\n");
}

/*
 *      Tiles - Parse the tiles
 *              This parser is based on characters()
 */

void tiles(long start,long finish)
{
int ctr,tmp,pos,seqno,altcode;
char *line,*Rptr,*p;

if(bookview[0]) // Not needed for the book viewer
	return;

TItot = 0;

//      Count the instances of 'Name' to get the quantity of tiles.

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);             // First see if it's blank
	if(!istricmp(Rptr,"name"))       // then look for the word 'Name'
		TItot++;                    // Got one
	}

ilog_printf("    Creating %d tiles ", TItot);

// Allocate room on this basis
if(in_editor != 2)
	til_alloc = TItot;  // If not in scripter, use original value

TIlist = (TILE *)M_get(sizeof(TILE),til_alloc+1);

ilog_printf("using %d bytes\n", sizeof(TILE)*(til_alloc+1));

// Now, process the data.  We will never parse this again, so it can be done
// destructively.

ilog_printf("    Loading tiles..");

Plot(TItot);
pos = -1;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{

		// Get the name
		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strfirst(strrest(line));     // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Find the offset in the block

				pos ++;									// Pre-increment
				Plot(0);
				TIlist[pos].name = (char *)strrest(line);		// Get name label
				TIlist[pos].alternate=NULL;				// No alternates
				TIlist[pos].cost = 1;					// Cost defaults to 1
				TIlist[pos].standfunc = -1;				// No stand function

				strstrip(TIlist[pos].name);				// Kill whitespace
				if(strlen(TIlist[pos].name) > 31)		// Check name length
					Dump(ctr,"This name exceeds 31 letters",TIlist[pos].name);
				continue;
				}
			else
				Dump(ctr,"NA: This tile has no name",NULL);
			}

		// Sanity Check

		if(pos == -1)
			Dump(ctr,"NA: The tile's name must come first",NULL);

		// Get the sequence of the tile

		if(!istricmp(Rptr,"sequence"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			tmp=getnum4sequence(Rbuffer);
			if(tmp==-1)
				Dump(ctr,"Could not find sequence:",Rbuffer);

			// Get seq. directly
			TIlist[pos].form=&SQlist[tmp];
			// Get name of sequence too
			strcpy(TIlist[pos].seqname,strfirst(strrest(line)));
			TIlist[pos].sdir = 1;
			TIlist[pos].sptr = 0;

			continue;
			}

        // Get the description

		if(!istricmp(Rptr,"description"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(Rbuffer[0] == '\"')  // Quoted text is direct
				{
				TIlist[pos].desc = strchr(line,'\"'); // start
				TIlist[pos].desc++;            // skip the quote
				Rptr = strrchr(line,'\"');     // Find last quote
				if(Rptr)
					*Rptr = 0;                 // blow quote away
				}
			else                    // Look up unquoted text
				Dump(ctr,"Invalid string",Rbuffer);
			continue;
			}

        // Get the flags

		if(!istricmp(Rptr,"solid"))             // Is it solid?
			{
			TIlist[pos].flags |= IS_SOLID;
			continue;
			}

		if(!istricmp(Rptr,"blocklight") ||
		!istricmp(Rptr,"blockslight"))        //Is it solid?
			{
			TIlist[pos].flags |= DOES_BLOCKLIGHT;
			continue;
			}

		if(!istricmp(Rptr,"water"))             // Is it water?
			{
			TIlist[pos].flags |= IS_WATER;
			continue;
			}

		if(!istricmp(Rptr,"cost"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The movement cost must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			TIlist[pos].cost=tmp;
			continue;
			}

		if(!istricmp(Rptr,"scroll"))
			{
			strcpy(Rbuffer,strfirst(strrest(line)));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The scrolling X coordinate must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			if(!in_editor)                  // Only for game
				if(tmp<0)
					tmp=32+tmp;
			TIlist[pos].sdx=tmp;
			TIlist[pos].odx=tmp;

			strcpy(Rbuffer,strfirst(strrest(strrest(line))));
			if(!strisnumber(Rbuffer))
				Dump(ctr,"The scrolling Y coordinate must be a number.",NULL);
			tmp=strgetnumber(Rbuffer);
			if(!in_editor)                  // Only for game
				if(tmp<0)
					tmp=32+tmp;
			TIlist[pos].sdy=tmp;
			TIlist[pos].ody=tmp;

			continue;
			}

		if(!istricmp(Rptr,"function"))
			{
			TIlist[pos].standfunc = getnum4PE(strfirst(strrest(line)));
			if(TIlist[pos].standfunc == -1)
				Dump(ctr,"Cannot find script function",strfirst(strrest(line)));
			continue;
			}

// Tile Classes

		if(!istricmp(Rptr,"blank"))
			{
			if(!fixwalls)
				continue;
			if(!TIlist[pos].alternate)
				{
				TIlist[pos].alternate=(SEQ_POOL **)M_get(16,sizeof(SEQ_POOL *)); // Alloc space
				for(tmp=0;tmp<16;tmp++)
					TIlist[pos].alternate[tmp]=TIlist[pos].form;
				}

			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: BLANK [directions that are blanked] [sequence name]","e.g. BLANK LU corner3");
			if(strfirst(strrest(strrest(line))) == NOTHING)
				Dump(ctr,"Usage: BLANK [directions that are blanked] [sequence name]","e.g. BLANK LU corner3");

			// Find the sequence that this Blank command uses
			p = (char *)strrest(strrest(line));
			seqno=getnum4sequence(p);
			if(seqno==-1)
				Dump(ctr,"Could not find sequence:",p);

			// Find the direction codes and add them to a unique ID
			p = strfirst(strrest(line));

			altcode=0;
			if(strchr(p,'L'))
				altcode|=1;
			if(strchr(p,'R'))
				altcode|=2;
			if(strchr(p,'U'))
				altcode|=4;
			if(strchr(p,'D'))
				altcode|=8;

			if(strchr(p,'l'))
				altcode|=1;
			if(strchr(p,'r'))
				altcode|=2;
			if(strchr(p,'u'))
				altcode|=4;
			if(strchr(p,'d'))
				altcode|=8;

/*
			ilog_quiet("blank %s (%d): adding rule %02d (",TIlist[pos].name,pos,altcode);
			if(altcode&1) ilog_quiet("L");
			if(altcode&2) ilog_quiet("R");
			if(altcode&4) ilog_quiet("U");
			if(altcode&8) ilog_quiet("D");
			ilog_quiet(")\n");
*/
			// Put the sequence number in the appropriate slot
			TIlist[pos].alternate[altcode]=&SQlist[seqno];
			continue;
			}

// Finish.  Shouldn't get here unless there was a problem

		if(!skip_unk)
			Dump(ctr,"Unknown keyword:",Rbuffer);

		}
	}
ilog_printf("\n");
}

/*
 *      PE Code - Load in the PEscript files
 */

void code(long start,long finish)
{
long ctr,pos;
char *Rptr;
char *line;
OBJCODE *pe;
char msg[1024];

// Temporary structures for the PEscript compiler
int numfiles,func;
int *filefuncs;

// The PE list is just a straight list of files.
// We just get the first term and store it.

if(bookview[0]) // Not needed for the book viewer
	return;

if(editarea[0]) // Not needed for the area editor
	return;


// First, see how many PE files we have.  This != functions

ilog_printf("    Parse scripts...");

numfiles = 0;

for(ctr= start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	strupr(line);                   // convert to upper-case
	if(strfirst(line) != NOTHING)  // Anything there?
		numfiles++;
	}

// Allocate room for the list of files to be compiled

if(in_editor != 2)
	pef_alloc = numfiles;  // If not in scripter, use original value

pe_files = (char **)M_get(sizeof(char *),pef_alloc+1);
filefuncs = (int *)M_get(sizeof(int),numfiles+1);

// Now, process the data.  We will never parse this again, so it can be done
// destructively.

pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // If it exists..
		{
		pe_files[pos] = hardfirst(line);  // Get filename
		strstrip(pe_files[pos]);
		pos++;  // Go onto the next entry in the VRM list
		}
	}


//ilog_quiet("Used %d/%d entries\n",pos,pef_alloc+1);

//compiler_init();

// Do we care about compiling the script properly?
if(in_editor)
	PE_FastBuild=1; // No

// Gather number of functions so we can allocate space

Plot(numfiles);

PEtot=0;
for(pos=0;pos<numfiles;pos++)
	{
	compiler_openfile(pe_files[pos]);
	ctr = compiler_parse();
	filefuncs[pos]=ctr;
	PEtot+=ctr;
	Plot(0);
	compiler_closefile();
	}

// Sort the keywords so we can do a fast lookup
compiler_sortkeywords();

ilog_printf("\n");
ilog_printf("    Build scripts...");

PElist = (PEM *)M_get(PEtot+1,sizeof(PEM));

Plot(numfiles);

func=0;

for(pos=0;pos<numfiles;pos++)
	{
	compiler_openfile(pe_files[pos]);

    // Allocate space for compiled code, length of code, function names

    // (Function names are pointers to the keyword structure and are released
    //  from memory when compiler_term is called)

	if(filefuncs[pos])
		{
		pe = (OBJCODE *)M_get(filefuncs[pos],sizeof(OBJCODE));
		// Compile this source file
		compiler_build(pe);

		// Deal with all the functions in each file

		for(ctr=0;ctr<filefuncs[pos];ctr++)
			{
			// Build all the functions, fill out the output data

			if(strlen(pe[ctr].name)>31)
				Dump(start,"Function name exceeds 31 characters",pe[ctr].name);

			if(pe[ctr].size == 0)
				{
				sprintf(msg,"%s in file %s",pe[ctr].name,pe_files[pos]);
				Dump(start,"Function probably missing END statement",msg);
				}

			// Store the compiled stuff for later
			PElist[func].size = pe[ctr].size;
			PElist[func].codelen = pe[ctr].codelen;
			
			PElist[func].name = stradd(NULL,pe[ctr].name);
			PElist[func].file = pe_files[pos];
			PElist[func].hidden = pe[ctr].hidden; // hide local functions
			PElist[func].Class = pe[ctr].Class; // function type

			PElist[func].code = (char *)M_get(pe[ctr].size,sizeof(unsigned char));
			memcpy(PElist[func].code,pe[ctr].code,pe[ctr].size);
			Init_PE(func); // Register any system functions
			func++;
			M_free(pe[ctr].code);
			}
		Plot(0);
		M_free(pe);
		}

	// Ok, we're done with that file
	compiler_closefile();
	}

// We're finished

M_free(filefuncs);

compiler_term();
ilog_printf("\n");
}


/*
 *      Sounds - Load in the Sounds
 *               this parser is based on sprites()
 */

void sounds(long start,long finish)
{
long ctr,pos;
char *Rptr;
char *line;

// The WAVE list is just a simple list, of format:
//
// NAME, FILENAME
// NAME, FILENAME
//   : ,    :
//   : ,    :
//
// Therefore it is sufficient to just get the first two terms and store them.
//

// First, see how many sounds we have.

Waves = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(strrest(line)); // If the second word exists,
                                    // then the first word has to as well
	if(Rptr != NOTHING)             // and we assume we have the data
		Waves++;
	}

ilog_printf("    Creating %d sound entries", Waves);

// Allocate room on this basis

if(in_editor != 2)
    wav_alloc = Waves;  // If not in scripter, use original value

//wavtab = (SMTab *)M_get(sizeof(SMTab),Waves+1);

wavtab = (SMTab *)M_get(sizeof(SMTab),wav_alloc+1);
ilog_printf(" %d bytes allocated\n", sizeof(SMTab)*(wav_alloc+1));

// Now, process the data.  We will never parse this again, so it can be done
// destructively.

pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];
	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // If it exists..
		{
		Rptr=strfirst(strrest(strrest(line)));   // Get third term
		if(Rptr!=NOTHING)               // If it exists
			if(!istricmp("nodrift",strfirst(Rptr)))
				wavtab[pos].nodrift = 1;

		Rptr=strfirst(strrest(line));         // Get second term
		if(Rptr!=NOTHING)               // If it exists
			{
                        wavtab[pos].fname = hardfirst((char *)strrest(line));      // Get filename
                        strstrip(wavtab[pos].fname);            // Kill whitespc

                        // Now get the name as a pointer in the text Block
                        wavtab[pos].name= hardfirst(line);
			pos++;  // Go onto the next entry in the WAV list
			}
		}
	}
}

/*
 *      Music - Load in the music
 *              this parser is based on sprites()
 */

void music(long start,long finish)
{
long ctr,pos;
char *Rptr;
char *line;

// The MUSIC list is just a simple list, of format:
//
// NAME, FILENAME
// NAME, FILENAME
//   : ,    :
//   : ,    :
//
// Therefore it is sufficient to just get the first two terms and store them.
//

// First, see how many mods we have.

//if(in_editor) return;

Songs = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];        // Get the line
	Rptr = strfirst(strrest(line));       // If the second word exists,
                                        // then the first word has to as well
        if(Rptr != NOTHING)             // and we assume we have the data
            Songs++;
        }

ilog_printf("    Creating %d music entries", Songs);

// Allocate room on this basis

if(in_editor != 2)
    mus_alloc = Songs;  // If not in scripter, use original value
mustab = (SMTab *)M_get(sizeof(SMTab),mus_alloc+1);

ilog_printf(" using %d bytes\n", sizeof(SMTab)*(mus_alloc+1));

// Now, process the data.  We will never parse this again, so it can be done
// destructively.

pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
        line = script.line[ctr];
	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // If it exists..
		{
		Rptr=strfirst(strrest(strrest(line)));   // Get third term
		if(Rptr!=NOTHING)               // If it exists
			if(!istricmp("loop",strfirst(Rptr)))
				mustab[pos].loopsong = 1;
		Rptr=strfirst(strrest(line));         // Get second term
		if(Rptr!=NOTHING)               // If it exists
			{
                        mustab[pos].fname = hardfirst((char *)strrest(line));      // Get filename
                        strstrip(mustab[pos].fname);            // Kill whitespc
			strlwr(mustab[pos].fname);

                        // Now get the name as a pointer in the text Block

                        mustab[pos].name= hardfirst(line);
			pos++;  // Go onto the next entry in the MOD list
			}
		}
	}
}

/*
 *      rooftiles - Load in sprites to be used as roof tiles
 */

void rooftiles(long start,long finish)
{
int dp;
long ctr,pos,imagedata;
char *Rptr;
char *line;

if(bookview[0]) // Not needed for the book viewer
	return;

// The rooftiles list is just a single list of sprite files:
//
// FILENAME
// FILENAME
//      :
//      :
//
// Therefore it is sufficient to just get the first term and store it.
//

// First, see how many sprites we have.

RTtot = 1; // 0 is special (no roof tile)

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);             // If the second word exists,
                                        // then the first word has to as well
	if(Rptr != NOTHING)             // and we assume we have the data
		RTtot++;
	}

ilog_printf("    Rooftops..", RTtot);

// Allocate room on this basis

if(in_editor != 2)
    rft_alloc = RTtot;
/*
else
    ilog_printf("      Allocating space for %d entries\n", rft_alloc);
*/

RTlist = (S_POOL *)M_get(sizeof(S_POOL),rft_alloc+1);
//ilog_printf("      %d bytes allocated\n", sizeof(S_POOL)*(rft_alloc+1));

// Now, process the data.  We will never parse this again, so it can be done
// destructively.

Plot(RTtot);
pos = 1; // 0 is special (no roof tile)
imagedata=0;

for(ctr=start;ctr<=finish;ctr++)
	{
    // Assume the roof vanishes when you stand under it
    RTlist[pos].flags=KILLROOF;
    line = script.line[ctr];
/*
    Rptr=strfirst(strrest(line));
    if(!istricmp(Rptr,"stand_under"))
    	RTlist[pos].flags&=~KILLROOF; // oh, it doesn't
    if(!istricmp(Rptr,"no_map"))
    	RTlist[pos].flags|=KILLMAP; // oh, it doesn't

	// It makes things dark?
	if(!istricmp(Rptr,"dark"))
		{
		RTlist[pos].darkness=64;	// Default to 64

		// If we have a sensible replacement, use that
		strcpy(Rbuffer,strfirst(strrest(strrest(line))));
		if(strisnumber(Rbuffer))
			RTlist[pos].darkness=strgetnumber(Rbuffer);
		}
*/
    Rptr=(char *)strrest(line);
	if(FindWord(Rptr,"stand_under"))
		RTlist[pos].flags&=~KILLROOF; // Stop it taking the roof away
	if(FindWord(Rptr,"no_map"))
		RTlist[pos].flags|=KILLMAP; // Stops the map from being available

	// It makes things dark?
	dp=FindWord(Rptr,"dark");
	if(dp)
		{
		RTlist[pos].darkness=64;	// Default to 64

		// Try and get a parameter
		strcpy(Rbuffer,strgetword(Rptr,dp+1));
		if(strisnumber(Rbuffer))
			RTlist[pos].darkness=strgetnumber(Rbuffer);
		}

	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // If it exists..
		{
		RTlist[pos].fname = hardfirst(line);     // Get filename
		imagedata += Init_RoofTile(pos);
		pos++;  // Go onto the next entry in the sprite list
		}
	}

ilog_quiet("Finished\n");
ilog_printf("\n");
}


/*
 *      lightmaps - Load in sprites to be used as light maps
 */

void lightmaps(long start,long finish)
{
long ctr,pos,imagedata;
char *Rptr;
char *line;

// The lightmaps list is just a single list of sprite files:
//
// FILENAME
// FILENAME
//      :
//      :
//
// Therefore it is sufficient to just get the first term and store it.
//

// First, see how many sprites we have.

LTtot = 1; // 0 is special (no lightmap)

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];			// Get the line
	Rptr = strfirst(line);
	if(Rptr != NOTHING)
		LTtot++;
	}

ilog_printf("    Lightmaps..", LTtot);

// Allocate room on this basis

if(in_editor != 2)
    lgt_alloc = LTtot;
/*
else
    ilog_printf("      Allocating space for %d entries\n", rft_alloc);
*/

LTlist = (L_POOL *)M_get(sizeof(L_POOL),lgt_alloc+1);
//ilog_printf("      %d bytes allocated\n", sizeof(S_POOL)*(rft_alloc+1));

// Now, process the data.  We will never parse this again, so it can be done
// destructively.

Plot(LTtot);
pos = 1; // 0 is special (no roof tile)
imagedata=0;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(strrest(line));
	Rptr=strfirst(line);                       // Get first term
	if(Rptr!=NOTHING)                       // If it exists..
		{
		LTlist[pos].fname = hardfirst(line);     // Get filename
		imagedata += Init_LightMap(pos);
		pos++;  // Go onto the next entry in the sprite list
		}
	}

ilog_printf("\n");
}

/*
 *      Get the names for all the data tables
 */

void pre_tables(long start,long finish)
{
long ctr,pos;
char *line;
const char *Rptr;

DTtot = 0;
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);          // First see if it's blank
	if(!istricmp(Rptr,"name"))       // then look for the keyword 'Name'
		DTtot++;                    // Got one
	}

// Allocate room on this basis

if(in_editor != 2)
    tab_alloc = DTtot;  // If not in scripter, use original value of tab_alloc

ilog_printf("    Creating %d lookup tables ", tab_alloc);

DTlist = (DATATABLE *)M_get(sizeof(DATATABLE),tab_alloc+1);

ilog_printf("using %d bytes\n", sizeof(DATATABLE)*(tab_alloc+1));

// Now, get the name for each entry

pos = 0;
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{
		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Find the offset in the block

				DTlist[pos].name = (char *)strrest(line);   // Get name label
				strstrip(DTlist[pos].name);         // Kill whitespace
				pos++;
				}
			}
		}
	}
}

/*
 *      Tables - user-defined data lookup
 */

void tables(long start,long finish)
{
long ctr,pos,list,lpos=0;
char *line;
const char *Rptr;
long liststart=0,listend=0;

// Tables are optional

if(DTtot<1)
	return;

ilog_printf("    User Lookup Tables");

Plot(DTtot);
pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{

		// The 'NAME' clause.  Big.

		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr==NOTHING)           // Make sure its there
				Dump(ctr,"NA: This table has no name",NULL);

			DTlist[pos].entries=0;
			DTlist[pos].keytype=0;
			DTlist[pos].listtype=0;

			// First, find the boundary of the framelist
			// so we can calculate the right number of frames

			liststart=0;
			listend=0;

			list = ctr+1;  // Skip the NAME: line
			do
				{
				Rptr = strfirst(script.line[list]);
				if(!liststart)                  // Don't have start
					if(!istricmp(Rptr,"list:"))
						liststart = list+1;

				if(!DTlist[pos].keytype)        // Don't know keytype
					if(!istricmp(Rptr,"key=integer"))
						DTlist[pos].keytype='i';

				if(!DTlist[pos].keytype)        // Don't know keytype
					if(!istricmp(Rptr,"key=string"))
						DTlist[pos].keytype='s';

				if(!DTlist[pos].listtype)        // Don't know keytype
					if(!istricmp(Rptr,"data=integer"))
						DTlist[pos].listtype='i';

				if(!DTlist[pos].listtype)        // Don't know keytype
					if(!istricmp(Rptr,"data=string"))
						DTlist[pos].listtype='s';

				if(!listend)                    // Don't have end
					if(!istricmp(Rptr,"end"))
						listend = list-1;

				if(!istricmp(Rptr,"name") || list == finish)
					list = -1;                  // Reached the end

				list++;
				} while(list>0);                // list = 0 on exit

			// Now we should have the boundaries.

			if(!liststart)
				Dump(ctr,"NA: This table has no LIST: entry!",DTlist[pos].name);

			if(!listend)
				Dump(ctr,"NA: This table has no END entry!",DTlist[pos].name);

			// Good.  Now, we'll count the entries

			for(list = liststart;list<=listend;list++)
				{
				Rptr = strfirst(script.line[list]);
				if(Rptr != NOTHING)
					DTlist[pos].entries++;
				}

			if(!DTlist[pos].keytype)
				Dump(ctr,"NA: Key type not set for this list",DTlist[pos].name);

			if(!DTlist[pos].listtype)
				Dump(ctr,"NA: List type not set for this list",DTlist[pos].name);

			if(!DTlist[pos].entries)
				Dump(ctr,"NA: This table has no items at all!",DTlist[pos].name);

			// Now we have the number of items in the list
			DTlist[pos].list = (DT_ITEM *)M_get(sizeof(DT_ITEM),DTlist[pos].entries+1);

			// Now we've allocated the space.  We're done I think
			}

		// The 'FRAMELIST' clause.

		if(!istricmp(Rptr,"list:"))       // Got the first one
			{
			if(!DTlist[pos].name)
				Dump(pos,"LI: This table has no name",NULL);

			if(!liststart)
				Dump(ctr,"LI: This table has no LIST: entry!",DTlist[pos].name);

			if(!listend)
				Dump(ctr,"LI: This table has no END entry!",DTlist[pos].name);

			lpos=0;

			for(long list = liststart;list<=listend;list++)
				{
				if(strfirst(script.line[list]) != NOTHING)
					{
					// Get the second entry first because of hardfirst
					Rptr = strfirst(strrest(script.line[list]));
					if(Rptr == NOTHING)
						Dump(ctr,"LI: No item.  Tables must be <key> <item>",DTlist[pos].name);
					if(DTlist[pos].listtype == 'i')
						DTlist[pos].list[lpos].ii=strgetnumber(Rptr);
					if(DTlist[pos].listtype == 's')
						DTlist[pos].list[lpos].is=hardfirst((char *)strrest(script.line[list]));

					// Get the first entry
					Rptr = strfirst(script.line[list]);
					if(Rptr == NOTHING)
						Dump(ctr,"LI: No key.  Tables must be <key> <item>",DTlist[pos].name);
					if(DTlist[pos].keytype == 'i')
						DTlist[pos].list[lpos].ki=strgetnumber(Rptr);
					if(DTlist[pos].keytype == 's')
						DTlist[pos].list[lpos].ks=hardfirst(script.line[list]);
					lpos++;
					}
				}
			}

		// The 'END' clause.

		if(!istricmp(Rptr,"END"))       // End of the sequence
			{
			// Sort by key for faster lookup
			if(DTlist[pos].keytype == 's')
				qsort(DTlist[pos].list,lpos,sizeof(DT_ITEM),CMP_sort_DTS);
			if(DTlist[pos].keytype == 'i')
				qsort(DTlist[pos].list,lpos,sizeof(DT_ITEM),CMP_sort_DTI);
			DTlist[pos].entries=lpos;
			pos++;
			Plot(0);
			}


		}
	}

ilog_printf("\n");
}

/*
 *      Get the names for all the data tables
 */

void pre_strings(long start,long finish)
{
long ctr,pos,ctr2;
char *line;
const char *Rptr;

STtot = 0;
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);          // First see if it's blank
	if(!istricmp(Rptr,"name"))       // then look for the keyword 'Name'
		STtot++;                    // Got one
	if(!istricmp(Rptr,"quickname"))       // then look for the keyword 'Name'
		STtot++;                    // Got one
	}

// Allocate room on this basis

if(in_editor != 2)
    str_alloc = STtot;  // If not in scripter, use original value of tab_alloc

ilog_printf("    Creating %d strings ", str_alloc);

STlist = (ST_ITEM *)M_get(sizeof(ST_ITEM),str_alloc+1);

ilog_printf("using %d bytes\n", sizeof(ST_ITEM)*(str_alloc+1));

// Now, get the name for each entry

pos = 0;
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{
		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			
			for(ctr2=0;ctr2<pos;ctr2++)
				if(!stricmp(STlist[ctr2].name,Rptr))
					Dump(ctr,"ST: Duplicate string name",Rptr);
			
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Find the offset in the block
				STlist[pos].name = (char *)strrest(line);   // Get name label
				strstrip(STlist[pos].name);         // Kill whitespace
				pos++;
				}
			}
			
		if(!istricmp(Rptr,"quickname"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			
			for(ctr2=0;ctr2<pos;ctr2++)
				if(!stricmp(STlist[ctr2].name,Rptr))
					Dump(ctr,"ST: Duplicate string name",Rptr);
			
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Grab the name
				STlist[pos].name = stradd(NULL,Rptr);
				pos++;
				}
			}
			
		}
	}
}

/*
 *      Tables - user-defined data lookup
 */

void strings(long start,long finish)
{
long ctr,pos,list,len;
char *line;
const char *Rptr;
long liststart=0,listend=0;

// Strings are optional

if(STtot<1)
	return;

ilog_printf("    Strings");

Plot(STtot);
pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{

		// The 'NAME' clause.  Big.

		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr==NOTHING)           // Make sure its there
				Dump(ctr,"ST: This string has no name",NULL);

			// First, find the boundary of the framelist
			// so we can calculate the right number of frames

			liststart=0;
			listend=0;

			list = ctr+1;  // Skip the NAME: line
			do
				{
				Rptr = strfirst(script.line[list]);
				if(!liststart)                  // Don't have start
					if(!istricmp(Rptr,"start:"))
						liststart = list+1;

				if(!listend)                    // Don't have end
					if(!istricmp(Rptr,"end"))
						listend = list-1;

				if(!istricmp(Rptr,"name") || list == finish)
					list = -1;                  // Reached the end

				list++;
				} while(list>0);                // list = 0 on exit

			// Now we should have the boundaries.

			if(!liststart)
				Dump(ctr,"ST: This string has no START: marker!",DTlist[pos].name);

			if(!listend)
				Dump(ctr,"ST: This string has no END marker!",DTlist[pos].name);

			// Good.  Now, figure out how much string there is
			len=0;
			for(list = liststart;list<=listend;list++)
				{
				len+=strlen(script.line[list]);
				len++; // CRLF character
				}

			STlist[pos].data = (char *)M_get(len+1,1);
			for(list = liststart;list<=listend;list++)
				{
				strcat(STlist[pos].data,script.line[list]);
				strcat(STlist[pos].data,"|");
				}
			line=strrchr(STlist[pos].data,'|');
			if(line)
				*line=0; // Remove last CRLF
			
			// Sorted
			}

		// Title, optional

		if(!istricmp(Rptr,"title"))
			{
			Rptr=strrest(line);
			STlist[pos].title = stradd(NULL,Rptr);
			}

		// Tag, optional

		if(!istricmp(Rptr,"tag"))
			{
			Rptr=strrest(line);
			STlist[pos].tag = stradd(NULL,Rptr);
			}

		// The 'END' clause.

		if(!istricmp(Rptr,"END"))       // End of the sequence
			{
			// Sort by name for faster lookup
			pos++;
			qsort(STlist,pos,sizeof(ST_ITEM),CMP_sort_ST);
			Plot(0);
			}

		// Fast-track string definition, quickname name stringdata
		if(!istricmp(Rptr,"quickname"))       // Got the first one
			{
			Rptr=strrest(strrest(line));        // Get second term
			if(Rptr==NOTHING)           // Make sure its there
				Dump(ctr,"STQN: This string has no string data",NULL);

			// First, find the boundary of the framelist
			// so we can calculate the right number of frames

			STlist[pos].data = stradd(NULL,Rptr);
		
			// Sort by name for faster lookup
			qsort(STlist,pos,sizeof(ST_ITEM),CMP_sort_ST);
			pos++;
			Plot(0);
			}

		}
	}

ilog_printf("\n");

// Now define them for the compiler

for(ctr=0;ctr<STtot;ctr++)
	if(!add_symbol_ptr(STlist[ctr].name,'P',&STlist[ctr].data))
		Dump(finish,"Strings: Could not define script keyword - duplicate symbol?",STlist[ctr].name);
}



/*
 *      Get the names for all the tile links
 */

void pre_tilelink(long start,long finish)
{
long ctr,pos;
char *line;
const char *Rptr;

if(in_editor == 0)	// Not used in game
	return;

TLtot = 1; // 0 is blank
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];        // Get the line
	Rptr = strfirst(line);          // First see if it's blank
	if(!istricmp(Rptr,"name"))       // then look for the keyword 'Name'
		TLtot++;                    // Got one
	}

// Allocate room on this basis

if(in_editor != 2)
    tli_alloc = TLtot;  // If not in scripter, use original value of tli_alloc

ilog_printf("    Creating %d tilelinks ", tli_alloc);

TLlist = (TILELINK *)M_get(sizeof(TILELINK),tli_alloc+1);

ilog_printf("using %d bytes\n", sizeof(TILELINK)*(tli_alloc+1));

// Now, get the name for each entry

pos = 1; // 0 is blank
for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{
		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr!=NOTHING)           // Make sure its there
				{
				// Find the offset in the block

				TLlist[pos].name = (char *)strrest(line);   // Get name label
				strstrip(TLlist[pos].name);         // Kill whitespace
				pos++;
				}
			}
		}
	}
}

/*
 *      TileLinks - tile relationships for automatic path generation
 */

void tilelink(long start,long finish)
{
long ctr,pos,tilno;
int c,d,ok,problems,tlnum;
char *line,*p;
const char *Rptr;

// Tables are optional

if(bookview[0]) // Not needed for the book viewer
	return;

if(TLtot<1)
	return;

ilog_printf("    TileLinks");

Plot(TLtot);
pos = 0;

for(ctr=start;ctr<=finish;ctr++)
	{
	line = script.line[ctr];
	Rptr=strfirst(line);                    // Get first term
	if(Rptr!=NOTHING)                       // Main parsing is here
		{

		// The 'NAME' clause.  Big.

		if(!istricmp(Rptr,"name"))       // Got the first one
			{
			Rptr=strgetword(line,2);        // Get second term
			if(Rptr==NOTHING)           // Make sure its there
				Dump(ctr,"NA: This tilelink has no name",NULL);

			pos++;
			Plot(0);

			TLlist[pos].tile=0;

#if 0 // If we are going to allocate the precise amount, and not make fixes

			TLlist[pos].nt=0;
			TLlist[pos].st=0;
			TLlist[pos].et=0;
			TLlist[pos].wt=0;
			TLlist[pos].net=0;
			TLlist[pos].nwt=0;
			TLlist[pos].set=0;
			TLlist[pos].swt=0;

			// Scan the entry to count the relationships

			list = ctr+1;  // Skip the NAME: line
			do
				{
				Rptr = strfirst(script.line[list]);

				if(!istricmp(Rptr,"n"))
					TLlist[pos].nt++;
				if(!istricmp(Rptr,"s"))
					TLlist[pos].st++;
				if(!istricmp(Rptr,"e"))
					TLlist[pos].et++;
				if(!istricmp(Rptr,"w"))
					TLlist[pos].wt++;
				if(!istricmp(Rptr,"ne"))
					TLlist[pos].net++;
				if(!istricmp(Rptr,"nw"))
					TLlist[pos].nwt++;
				if(!istricmp(Rptr,"se"))
					TLlist[pos].set++;
				if(!istricmp(Rptr,"sw"))
					TLlist[pos].swt++;

				if(!istricmp(Rptr,"name") || list == finish)
					list = -1;                  // Reached the end

				list++;
				} while(list>0);                // list = 0 on exit

			// Now we can allocate the stuff
			// (Each link has two entries)

			if(TLlist[pos].nt)
				TLlist[pos].n = (TL_DATA *)M_get(TLlist[pos].nt,sizeof(TL_DATA));

			if(TLlist[pos].st)
				TLlist[pos].s = (TL_DATA *)M_get(TLlist[pos].st,sizeof(TL_DATA));

			if(TLlist[pos].et)
				TLlist[pos].e = (TL_DATA *)M_get(TLlist[pos].et,sizeof(TL_DATA));

			if(TLlist[pos].wt)
				TLlist[pos].w = (TL_DATA *)M_get(TLlist[pos].wt,sizeof(TL_DATA));

			if(TLlist[pos].net)
				TLlist[pos].ne = (TL_DATA *)M_get(TLlist[pos].net,sizeof(TL_DATA));

			if(TLlist[pos].nwt)
				TLlist[pos].nw = (TL_DATA *)M_get(TLlist[pos].nwt,sizeof(TL_DATA));

			if(TLlist[pos].set)
				TLlist[pos].se = (TL_DATA *)M_get(TLlist[pos].set,sizeof(TL_DATA));

			if(TLlist[pos].swt)
				TLlist[pos].sw = (TL_DATA *)M_get(TLlist[pos].swt,sizeof(TL_DATA));
#else
			TLlist[pos].n = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].s = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].e = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].w = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].ne = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].nw = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].se = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
			TLlist[pos].sw = (TL_DATA *)M_get(TLtot,sizeof(TL_DATA));
#endif

			// Set them back to 0..

			TLlist[pos].nt=0;
			TLlist[pos].st=0;
			TLlist[pos].et=0;
			TLlist[pos].wt=0;
			TLlist[pos].net=0;
			TLlist[pos].nwt=0;
			TLlist[pos].set=0;
			TLlist[pos].swt=0;
			}

		// Tile

		if(!istricmp(Rptr,"tile"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: tile [tile name]","e.g. tile mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tile(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tile in section tiles:",p);

			TLlist[pos].tile=tilno;
			}

		// connections

		if(!istricmp(Rptr,"N"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: N [north tile]","e.g. n mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].n[TLlist[pos].nt].tile=tilno;
			TLlist[pos].nt++;
			}

		if(!istricmp(Rptr,"S"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: S [south tile]","e.g. s mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].s[TLlist[pos].st].tile=tilno;

			TLlist[pos].st++; // skip past second entry
			}

		if(!istricmp(Rptr,"E"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: E [east tile]","e.g. e mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].e[TLlist[pos].et].tile=tilno;

			TLlist[pos].et++; // skip past second entry
			}

		if(!istricmp(Rptr,"W"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: W [west tile]","e.g. w mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].w[TLlist[pos].wt].tile=tilno;

			TLlist[pos].wt++; // skip past second entry
			}

		if(!istricmp(Rptr,"NW"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: NW [northwest tile]","e.g. nw mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].nw[TLlist[pos].nwt].tile=tilno;

			TLlist[pos].nwt++; // skip past second entry
			}

		if(!istricmp(Rptr,"NE"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: NE [northeast tile]","e.g. ne mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].ne[TLlist[pos].net].tile=tilno;

			TLlist[pos].net++; // skip past second entry
			}

		if(!istricmp(Rptr,"SW"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: SW [southwest tile]","e.g. sw mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].sw[TLlist[pos].swt].tile=tilno;

			TLlist[pos].swt++; // skip past second entry
			}

		if(!istricmp(Rptr,"SE"))
			{
			// Error checking
			if(strfirst(strrest(line)) == NOTHING)
				Dump(ctr,"Usage: SE [southeast tile]","e.g. se mud03");

			// Find the tilelink to the left
			p = strfirst(strrest(line));
			tilno=getnum4tilelink(p);
			if(tilno==-1)
				Dump(ctr,"Could not find tilelink:",p);

			TLlist[pos].se[TLlist[pos].set].tile=tilno;

			TLlist[pos].set++; // skip past second entry
			}

		}
	}
ilog_printf("\n");

// Don't check symmetry in Resedit (it would automatically fix changes!)

if(in_editor == 2)
	return;

// Check symmetry of connections
for(ctr=1;ctr<TLtot;ctr++)
	{
	// North
	for(c=0;c<TLlist[ctr].nt;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].n[c].tile;
		for(d=0;d<TLlist[tlnum].st;d++)
			{
			if(TLlist[tlnum].s[d].tile == ctr)
				ok=1;
			}
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to North, but not vice-versa\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].s[TLlist[tlnum].st++].tile=ctr;
			problems=1;
			}
		}

	// South
	for(c=0;c<TLlist[ctr].st;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].s[c].tile;
		for(d=0;d<TLlist[tlnum].nt;d++)
			if(TLlist[tlnum].n[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to South, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].n[TLlist[tlnum].nt++].tile=ctr;
			problems=1;
			}
		}

	// East
	for(c=0;c<TLlist[ctr].et;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].e[c].tile;
		for(d=0;d<TLlist[tlnum].wt;d++)
			if(TLlist[tlnum].w[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to East, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].w[TLlist[tlnum].wt++].tile=ctr;
			problems=1;
			}
		}

	// West
	for(c=0;c<TLlist[ctr].wt;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].w[c].tile;
		for(d=0;d<TLlist[tlnum].et;d++)
			if(TLlist[tlnum].e[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to West, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].e[TLlist[tlnum].et++].tile=ctr;
			problems=1;
			}
		}

	// NorthEast
	for(c=0;c<TLlist[ctr].net;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].ne[c].tile;
		for(d=0;d<TLlist[tlnum].swt;d++)
			if(TLlist[tlnum].sw[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to NorthEast, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].sw[TLlist[tlnum].swt++].tile=ctr;
			problems=1;
			}
		}

	// NorthWest
	for(c=0;c<TLlist[ctr].nwt;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].nw[c].tile;
		for(d=0;d<TLlist[tlnum].set;d++)
			if(TLlist[tlnum].se[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to NorthWest, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].se[TLlist[tlnum].set++].tile=ctr;
			problems=1;
			}
		}

	// SouthEast
	for(c=0;c<TLlist[ctr].set;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].se[c].tile;
		for(d=0;d<TLlist[tlnum].nwt;d++)
			if(TLlist[tlnum].nw[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to SouthEast, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].nw[TLlist[tlnum].nwt++].tile=ctr;
			problems=1;
			}
		}

	// SouthWest
	for(c=0;c<TLlist[ctr].swt;c++)
		{
		ok=0;
		tlnum=TLlist[ctr].sw[c].tile;
		for(d=0;d<TLlist[tlnum].net;d++)
			if(TLlist[tlnum].ne[d].tile == ctr)
				ok=1;
		if(!ok)
			{
#ifdef VERBOSE_TL			  
			Bug("TileLink '%s' has '%s' to SouthWest, but not vice-versa.\n",TLlist[ctr].name,TLlist[tlnum].name);
#endif

			// Patch in memory
			TLlist[tlnum].ne[TLlist[tlnum].net++].tile=ctr;
			problems=1;
			}
		}

	}

// Write out a corrected version of the TileLinks

#ifdef TILELINK_OUTPUT

if(problems)
	{
	ilog_quiet("section: tilelink\n");
	for(ctr=1;ctr<TLtot;ctr++)
		{
		ilog_quiet("\tname %s\n",TLlist[ctr].name);
		ilog_quiet("\ttile %s\n",TIlist[TLlist[ctr].tile].name);
		for(c=0;c<TLlist[ctr].nt;c++)
			ilog_quiet("\tn %s\n",TLlist[TLlist[ctr].n[c].tile].name);
		for(c=0;c<TLlist[ctr].st;c++)
			ilog_quiet("\ts %s\n",TLlist[TLlist[ctr].s[c].tile].name);
		for(c=0;c<TLlist[ctr].et;c++)
			ilog_quiet("\te %s\n",TLlist[TLlist[ctr].e[c].tile].name);
		for(c=0;c<TLlist[ctr].wt;c++)
			ilog_quiet("\tw %s\n",TLlist[TLlist[ctr].w[c].tile].name);
		for(c=0;c<TLlist[ctr].net;c++)
			ilog_quiet("\tne %s\n",TLlist[TLlist[ctr].ne[c].tile].name);
		for(c=0;c<TLlist[ctr].nwt;c++)
			ilog_quiet("\tnw %s\n",TLlist[TLlist[ctr].nw[c].tile].name);
		for(c=0;c<TLlist[ctr].set;c++)
			ilog_quiet("\tse %s\n",TLlist[TLlist[ctr].se[c].tile].name);
		for(c=0;c<TLlist[ctr].swt;c++)
			ilog_quiet("\tsw %s\n",TLlist[TLlist[ctr].sw[c].tile].name);
		ilog_quiet("\n");
		}
	}

ilog_quiet("\n\n");
#endif
}


/*
 *      Restring - Turn all the static strings from loaded script
 *                 into dynamically-allocated strings (that can be lengthened)
 */

/*
void Restring()
{
int ctr;
char buf[MAX_LINE_LEN];

#define RESTR_MACRO(x) {strcpy(buf,x);\
                       strupr(buf);\
                       x=(char *)M_get(MAX_LINE_LEN,1);\
                       strcpy(x,buf);}

#define I_RESTR_MACRO(x) {strcpy(buf,x);\
                       x=(char *)M_get(MAX_LINE_LEN,1);\
                       strcpy(x,buf);}

for(ctr=0;ctr<SPtot;ctr++)
    {
    RESTR_MACRO(SPlist[ctr].name);
    RESTR_MACRO(SPlist[ctr].fname);
    }

for(ctr=0;ctr<SQtot;ctr++)
    RESTR_MACRO(SQlist[ctr].name);

for(ctr=0;ctr<Songs;ctr++)
    {
    RESTR_MACRO(mustab[ctr].name);
    RESTR_MACRO(mustab[ctr].fname);
    }

for(ctr=0;ctr<CHtot;ctr++)
    {
    RESTR_MACRO(CHlist[ctr].name);
    I_RESTR_MACRO(CHlist[ctr].desc);
    I_RESTR_MACRO(CHlist[ctr].shortdesc);
    }

for(ctr=0;ctr<Waves;ctr++)
    {
    RESTR_MACRO(wavtab[ctr].name);
    RESTR_MACRO(wavtab[ctr].fname);
    }

for(ctr=0;ctr<TItot;ctr++)
    {
    RESTR_MACRO(TIlist[ctr].name);
    I_RESTR_MACRO(TIlist[ctr].desc);
    }

for(ctr=0;ctr<STtot;ctr++)
    {
    RESTR_MACRO(STlist[ctr].name);
    }

}
*/

/*
 *      Dump - Crash out and show a fragment of the code
 */

void Dump(long lineno,const char *error,const char *help)
{
char errbuf[1024];
char errstr[1024];
short ctr,sl,c2,c2m8;
long l2,Lctr;

//KillGFX();

//ilog_printf("Error: %s\n%s [%d]\n",error,help,lineno);

ilog_printf("\n\n");
irecon_colour(0,200,0); // Green
sprintf(errstr,"Error in %s at line %ld: %s\r\n",parsename,lineno,error);

ilog_printf(errstr);

l2=lineno+5;
if(l2>script.lines)
	l2=script.lines;
irecon_colour(200,0,0); // Red
for(Lctr=lineno;Lctr<l2;Lctr++)
	if(script.line[Lctr]!=NOTHING&&script.line[Lctr]!=NULL)
	{
	c2=0;
	sl=strlen(script.line[Lctr]);
	for(ctr=0;ctr<sl;ctr++)
		if(script.line[Lctr][ctr]=='\t')
			{
			c2m8=c2%8;
			if(!c2m8)
				c2m8=8;
			for(;c2m8>0;c2m8--)
				errbuf[c2++]=' ';
			}
		else
			errbuf[c2++]=script.line[Lctr][ctr];
	errbuf[c2++]=0;
	sprintf(errstr,"%6ld: %s\r\n",Lctr,errbuf);

	ilog_printf(errstr);
	}
irecon_colour(0,200,0); // Green
if(help)
	{
	sprintf(errstr,"\r\n%s\r\n",help);
	ilog_printf(errstr);
	}
ilog_printf("\n");
irecon_colour(200,200,200); // Light gray
ilog_printf("Press a key to exit\n");
WaitForAscii();
//KillGFX();
exit(1);
}

/*
 *      Purify - Remove comments from the script to ease compilation
 */

void Purify()
{
int ctr,len,pos;
char *ptr;

// Scan each line for comments

for(ctr=0;ctr<script.lines;ctr++)
    {
    ptr = strchr(script.line[ctr],'#');         // Look for the hash
    if(ptr)
           *ptr=0;                              // make EOL if we found one

    ptr = strchr(script.line[ctr],10);          // Look for the LF
    if(ptr)
           *ptr=0;                              // make EOL if we found one
    }

// Scan each line for whitespace and turn it into space

for(ctr=0;ctr<script.lines;ctr++)
    {
//    strupr(script.line[ctr]);
    len = strlen(script.line[ctr]);    // Got length of line
    for(pos=0;pos<len;pos++)
        if(isxspace(script.line[ctr][pos]))
                script.line[ctr][pos] = ' ';    // Whitespace becomes space

    strstrip(script.line[ctr]);
    }


return;
}


/*
 *      SeekSect - Locate each discrete section
 */

void SeekSect()
{
int ctr;
char buf[MAX_LINE_LEN];

no_of_sections = 0;

for(ctr=0;ctr<script.lines;ctr++)
	{
//	ilog_quiet("line %d: %x\n",ctr,script.line[ctr]);
//	ilog_quiet("line %d='%s'\n",ctr,script.line[ctr]);
//	strupr(script.line[ctr]);               // Convert to uppercase
	strcpy(Rbuffer,script.line[ctr]);       // Get line
	strcpy(buf,strfirst(Rbuffer));          // Get first word from line
	if(buf[0])
		if(!istricmp(buf,"SECTION:"))
			{
			sect_map[no_of_sections++].end = ctr-1;
			sect_map[no_of_sections].start = ctr;
//			strcpy(sect_map[no_of_sections].name,strfirst(strrest(Rbuffer)));
			strcpy(sect_map[no_of_sections].name,strgetword(Rbuffer,2));
			}
	}
sect_map[no_of_sections++].end = script.lines-1;
}


static int CMP_sort_DTS(const void *a,const void *b)
{
return istricmp(((DT_ITEM *)a)->ks,((DT_ITEM*)b)->ks);
}

static int CMP_sort_DTI(const void *a,const void *b)
{
return ((DT_ITEM *)a)->ki-((DT_ITEM*)b)->ki;
}

static int CMP_sort_ST(const void *a,const void *b)
{
return istricmp(((ST_ITEM *)a)->name,((ST_ITEM*)b)->name);
}

int FindWord(const char *str, const char *find)
{
const char *ptr,*f;
int pos=1;

ptr=str;
while(ptr != NOTHING)
	{
	f=strfirst(ptr);
	if(f == NOTHING)
		break;
	if(!istricmp(f,find))
		return pos;
	ptr=(char *)strrest(ptr);
	pos++;
	}

return 0;
}

//
//  Look for a tilelink matching both of these
//

int FindTilelinkEW(int e, int w)
{
int ctr,actr,gotone;

//printf("Find EW link for tiles %s (%d) and %s (%d)\n",TLlist[e].name,e,TLlist[w].name,w);

for(ctr=0;ctr<TLtot;ctr++)
	{
	if(TLlist[ctr].et > 0 && TLlist[ctr].wt > 0)
		{
		gotone=0;
		for(actr=0;actr<TLlist[ctr].et;actr++)
			if(TLlist[TLlist[ctr].e[actr].tile].tile == e)
				{
				gotone=1;
				break;
				}
		if(!gotone)
			continue; // Not here

		for(actr=0;actr<TLlist[ctr].wt;actr++)
			if(TLlist[TLlist[ctr].w[actr].tile].tile == w)
				{
//				printf("chose %s (%d)\n",TLlist[ctr].name,ctr);
				return ctr;
				}
		}
	}

return -1;  
}

//
//  Look for a tilelink matching both of these
//

int FindTilelinkNS(int n, int s)
{
int ctr,actr,gotone;

for(ctr=0;ctr<TLtot;ctr++)
	{
	if(TLlist[ctr].nt > 0 && TLlist[ctr].st > 0)
		{
		gotone=0;
		for(actr=0;actr<TLlist[ctr].nt;actr++)
			{
			if(TLlist[TLlist[ctr].n[actr].tile].tile == n)
				{
				gotone=1;
				break;
				}
			}
		if(!gotone)
			continue; // Not here

		for(actr=0;actr<TLlist[ctr].st;actr++)
			if(TLlist[TLlist[ctr].s[actr].tile].tile == s)
				return ctr;
		}
	}

return -1;  
}
