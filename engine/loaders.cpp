//
//	Resource loaders for various data formats
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "ithelib.h"
#include "media.hpp"

#ifdef _WIN32
	extern "C" {
	#include "jpeg/win/jpeglib.h" // What a crock
	}
	#include <io.h> // Because Windows doesn't have mkstemp
	#include <fcntl.h> // ditto
#else
	#include <jpeglib.h>
#endif
#include "core.hpp"
#include "gamedata.hpp"
#include "loadfile.hpp"

// Defines

#ifdef __BEOS__
#define P_tmpdir "/tmp"
#endif


// Strict checking for structure save (not for release code)
//#define STRUCT_CHECK

#ifdef STRUCT_CHECK
	#define S_START() char msg[128];int checksum=0;
	#define S_END(x) if(checksum != sizeof(x)) {sprintf(msg,"%s processed %d bytes instead of %d",#x,checksum,sizeof(x));ithe_panic("structure mismatch",msg);}
	#define S_GETWI(x) x=igetl_i(f);checksum+=4;
	#define S_GETWM(x) x=igetl_m(f);checksum+=4;
	#define S_GETSHI(x) x=igetsh_i(f);checksum+=2;
	#define S_GETSHM(x) x=igetsh_m(f);checksum+=2;
	#define S_GETB(x) x=igetb(f);checksum++;
	#define S_GETPTR(x) igetl_i(f);checksum+=4; // skip 4 bytes regardless
	#define S_READ(x,y) iread(x,y,f);checksum+=y;
	#define S_PUTWI(x) iputl_i(x,f);checksum+=4;
	#define S_PUTWM(x) iputl_m(x,f);checksum+=4;
	#define S_PUTSHI(x) iputsh_i(x,f);checksum+=2;
	#define S_PUTSHM(x) iputsh_m(x,f);checksum+=2;
	#define S_PUTB(x) iputb(x,f);checksum++;
	#define S_PUTPTR(x) iputl_i(0,f);checksum+=4; // 4 compatability bytes
	#define S_WRITE(x,y) iwrite(x,y,f);checksum+=y;
	#define S_HACK(x) checksum+=x; // Adjust for compatability
#else
	#define S_START()
	#define S_END(x)
	#define S_GETWI(x) x=igetl_i(f);
	#define S_GETWM(x) x=igetl_m(f);
	#define S_GETSHI(x) x=igetsh_i(f);
	#define S_GETSHM(x) x=igetsh_m(f);
	#define S_GETB(x) x=igetb(f);
	#define S_GETPTR(x) igetl_i(f); // skip 4 bytes regardless of true length
	#define S_READ(x,y) iread(x,y,f);
	#define S_PUTWI(x) iputl_i(x,f);
	#define S_PUTWM(x) iputl_m(x,f);
	#define S_PUTSHI(x) iputsh_i(x,f);
	#define S_PUTSHM(x) iputsh_m(x,f);
	#define S_PUTB(x) iputb(x,f);
	#define S_PUTPTR(x) iputl_i(0,f); // write 4 blank compatability bytes
	#define S_WRITE(x,y) iwrite(x,y,f);
	#define S_HACK(x)
#endif

// Variables

int MZ1_SavingGame=0;
int MZ1_LoadingGame=0;

// Functions

static IREBITMAP *iload_cel(char *filename);
static IREBITMAP *iload_pcx(char *filename);
static IREBITMAP *iload_jpeg(char *filename);
extern IREBITMAP *iload_png(char *filename);
static void UnPcx(unsigned char *in,unsigned char *out,int length);
static unsigned char *UnJPG(FILE *fp, int *w, int *h, int *bpp);
extern int getnum4char(const char *name);
extern int check_uuid(const char *uuid);

// Code

IREBITMAP *iload_bitmap(char *filename)
{

if(strstr(filename,".cel"))
	return iload_cel(filename);
if(strstr(filename,".pcx"))
	return iload_pcx(filename);
if(strstr(filename,".jpg"))
	return iload_jpeg(filename);
if(strstr(filename,".png"))
	return iload_png(filename);
if(strstr(filename,".CEL"))
	return iload_cel(filename);
if(strstr(filename,".PCX"))
	return iload_pcx(filename);
if(strstr(filename,".JPG"))
	return iload_jpeg(filename);
if(strstr(filename,".PNG"))
	return iload_png(filename);
return NULL;
}

/*
 *		load a CEL file using the IT-HE filesystem wrappers
 */

IREBITMAP *iload_cel(char *filename)
{
unsigned short w,h;                             // size of image
int len,x,y;
unsigned char *buf,*ptr;                        // conversion buffer
IFILE *fp;
IREBITMAP *img;
unsigned char *pal;
unsigned int poff;

// Read the whole file

fp = iopen(filename);
// Good, we're still around

len=ifilelength(fp);

buf=(unsigned char *)M_get(1,len);              // Allocate room
iread(buf,len,fp);
iclose(fp);

// OK, let's go

if(buf[0] != 0x19 || buf[1] != 0x91)
    ithe_panic("CEL header invalid for image",filename);

w = buf[2]+(buf[3]<<8);
h = buf[4]+(buf[5]<<8);
len = w*h;

pal = &buf[32];
ptr = pal+768;

// Allocate the sprite we will put the .CEL into

img = MakeIREBITMAP(w,h);
if(!img)
    ithe_panic("Create bitmap failed for image",filename);
img->Clear(ire_transparent);

// Convert the sprite line by line

ptr = &buf[800];
for(y=0;y<h;y++)
	for(x=0;x<w;x++)
		{
		if(*ptr)
			{
			poff = *ptr;
			poff = poff + poff + poff; // Multiply by 3
			img->PutPixel(x,y,pal[poff]<<2,pal[poff+1]<<2,pal[poff+2]<<2);
			}
		ptr++;
		}

// Free the temporary buffers

M_free(buf);
return img;
}


/*
 *		load a PCX file using the IT-HE filesystem wrappers
 */

IREBITMAP *iload_pcx(char *filename)
{
IREBITMAP *spr;					// Output
IFILE *fp;						// VFS filepointer
unsigned char *buf;				// Decoding buffer
unsigned char *raw;				// Raw buffer
long fl;						// file length
int w,h,len;					// Various data items
int ptr=0;						// For PCX-24 debanding
int ctr,ctr2,mode;
unsigned char pal[768];
unsigned int poff;

fp = iopen(filename);
// Good, we're still around

fl=ifilelength(fp)-128;			// PCX file is HEADER:DATA:PAL
raw = (unsigned char *)M_get(1,fl);
iseek(fp,8,SEEK_SET);			// Skip to the width

w = igetsh_i(fp)+1; // make 1-based from 0-based
h = igetsh_i(fp)+1;

iseek(fp,65,SEEK_SET);			// Skip to the type byte
mode = igetc(fp);

if(mode != 1 && mode != 3)
	{
    sprintf((char *)raw,"%d planes in file %s",mode,filename);
    ithe_panic("Funny number of bitplanes in PCX file:",(char *)raw);
	}

// Get the length of the raw bitmap

spr = MakeIREBITMAP(w,h);
if(!spr)
	{
	ithe_panic("Error creating bitmap",filename);
	}

spr->Clear(ire_transparent);
len = w*h;

// Allocate decompression buffer
buf = (unsigned char *)M_get(1,len*mode);

switch(mode)
    {
    // 8 bits per pixel

    case 1:
    iseek(fp,-768L,SEEK_END);			// Seek to palette data (end-768)
    iread(pal,768,fp);
    iseek(fp,128,SEEK_SET);				// Seek back to start of RLE data
	iread(raw,fl,fp);
    UnPcx(raw,buf,len);					// Undo the RLE coding

	ptr=0;
	for(ctr=0;ctr<h;ctr++)
		{
		for(ctr2=0;ctr2<w;ctr2++)
			{
			if(buf[ptr])
				{
				poff = buf[ptr];
				poff = poff + poff + poff; // Multiply by 3
				spr->PutPixel(ctr2,ctr,pal[poff],pal[poff+1],pal[poff+2]);
				}
			ptr++;
			}
		}

    break;                                 // Finished

    // 24 bits per pixel, true colour

    case 3:
    iseek(fp,128,SEEK_SET);				// Seek back to start of RLE data
	iread(raw,fl,fp);
    UnPcx(raw,buf,len*3);				// Undo the RLE coding

    // Because PCX-24 is so strange, the RGB levels are banded.
    // First we have a complete mono image of the red component
    // Then a complete mono image of the Green component
    // Finally a complete mono image of the Blue component

	// The code below re-combines the three bands.
    // The result is fed into makecol, to get the natively-packed word
    // for the current video mode

	ptr=0;
	for(ctr=0;ctr<h;ctr++)
		{
		for(ctr2=0;ctr2<w;ctr2++)
			{
			spr->PutPixel(ctr2,ctr,buf[ptr],buf[ptr+w],buf[ptr+w+w]);
			ptr++;
			}
		ptr+=(w*2);
		}

    break;

    default:
    sprintf((char *)buf,"%d planes in file %s",mode,filename);
    ithe_panic("Funny number of bitplanes in PCX file:",(char *)buf);
    }

M_free(buf);
M_free(raw);
iclose(fp);							// Close the file
              
return spr;
}





/*
 * UnPCX - Decode the RLE data into RAW bitmap info
 */

void UnPcx(unsigned char *in,unsigned char *out,int length)
{
unsigned char ob;						// Output Byte
int ctr,ctr2;

for(ctr=0;ctr<length;ctr++)		// Decode it all
	{
	ob=*in++;
	if(ob > 0xbf)						// If it's a run of colour
		{
		ctr2=(ob & 0x3f);				// Get run length
		ob=*in++;					// Get run colour
		memset(out,ob,ctr2);			// Do the run
		out+=ctr2;						// Increment pointer
		ctr+=ctr2-1;					// Adjust RLE pointer
		}
    else
		*out++=ob;						// It was a single colour
    }
return;                                 // Finished
}


/*
 *  load_JPG - As display_JPG but for sprites
 *
 */

IREBITMAP *iload_jpeg(char *filename)
{
char msg[256];
IFILE *fp;                                      // VFS file pointer
int w,h,bpp,x,y,ptr;
IREBITMAP *out;
unsigned char *buf;

fp = iopen(filename);

w=h=bpp=0;
buf = UnJPG(fp->fp, &w, &h, &bpp);
if(!buf)
	ithe_panic("Jpeg: Could not allocate memory to decompress",filename);

iclose(fp);

out=MakeIREBITMAP(w,h);
if(!out)
	ithe_panic("Jpeg: Out of memory loading .JPG file :",filename);

// It seems we have a result.  Convert to native colour format

ptr=0;

switch(bpp)
	{
	case 8: // Greyscale
	for(y=0;y<h;y++)
		for(x=0;x<w;x++)
			{
			if(buf[ptr])
				out->PutPixel(x,y,buf[ptr],buf[ptr],buf[ptr]);
			else
				out->PutPixel(x,y,ire_transparent);
			ptr++;
			}
	break;

    case 24:
	case 32:
	for(y=0;y<h;y++)
		for(x=0;x<w;x++)
			{
			out->PutPixel(x,y,buf[ptr+0],buf[ptr+1],buf[ptr+2]);
			ptr+=3;
			}
	break;

    default:
	sprintf(msg,"Jpeg: Unsupported bit depth %d in JPEG file",bpp);
    ithe_panic(msg,filename);
    break;
    }

M_free(buf);      // ok

return out;
}


/*
 *	Jpeg reader:
 */


// Error handler

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
char msgbuf[256];

  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
//  struct jpeg_error_mgr *myerr = cinfo->err;

  /* Display the message */
  (*cinfo->err->format_message)(cinfo,msgbuf);
  ithe_panic("Jpeg decompression error:",msgbuf);
}

//
// Here's the decompressor
//

unsigned char *UnJPG(FILE *fp, int *w, int *h, int *bpp)
{
struct jpeg_decompress_struct cinfo;
struct jpeg_error_mgr jerr;
JSAMPARRAY buffer;
int row_stride;
unsigned char *outbuf,*op;


cinfo.err = jpeg_std_error(&jerr);
jerr.error_exit = my_error_exit;

jpeg_create_decompress(&cinfo);

jpeg_stdio_src(&cinfo, fp);     // FILE *fp
jpeg_read_header(&cinfo, TRUE);

jpeg_calc_output_dimensions(&cinfo);

*w = cinfo.output_width;
*h = cinfo.output_height;

outbuf = (unsigned char *)M_get(*w**h,cinfo.output_components);
if(!outbuf)
      {
      ilog_quiet("alloc (%d*%d*%d) failed in load_jpeg\n",*w,*h,cinfo.output_components);
      return NULL;
      }
op=outbuf;

jpeg_start_decompress(&cinfo);

row_stride = cinfo.output_width * cinfo.output_components;
buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE,row_stride,1);


while (cinfo.output_scanline < cinfo.output_height)
      {
      jpeg_read_scanlines(&cinfo,buffer,1);
      memcpy(op,buffer[0],row_stride);
      op+=row_stride;
      }

*bpp = cinfo.output_components*8;       // 8 bit or 24 bit

jpeg_finish_decompress(&cinfo);
jpeg_destroy_decompress(&cinfo);

return outbuf;
}

/*
 *  iload_tempfile - Create a temporary file from a .RAR entry
 *  Dest is set to the filename.  You must remove the file afterwards.
 */

/*
int iload_tempfile(char *filename, char *dest)
{
char outname[1024];
char fname[1024];
IFILE *fp;                                      // VFS file pointer
int length,handle;
unsigned char *buf;
#ifdef _WIN32
char *unsafe; // For the unsafe OS (Windows)
#endif

if(!loadfile(filename,fname))
	{
	//printf("loadfile '%s','%s' failed\n",filename,dest);
	return 0;
	}
//	ithe_panic("Could not load font: (file not found)",filename);

fp = iopen(fname);
length=fp->length; // Store file length for later

buf = (unsigned char *)M_get(1,length); // Read it into memory, so allocate some first
iread(buf,length,fp);  // Read it in
iclose(fp);

// Create a temporary file
strcpy(outname,"IRXXXXXX");
#if defined(__SOMEUNIX__) || defined(__DJGPP__)
strcpy(outname,P_tmpdir);
strcat(outname,"/IRXXXXXX");

handle=mkstemp(outname);
if(handle == -1)
	{
	M_free(buf); // Free buffer up
	return 0;
	}
#else
// Humour Windows by doing it the unstable way
//unsafe=tmpnam(NULL);
unsafe = _tempnam( "c:\\tmp", "iretmp");
if(!unsafe)
	{
	//printf("tmpnam returned NULL\n");
	M_free(buf); // Free buffer up
	return 0;
	}
handle=open(unsafe,_O_CREAT|_O_TRUNC|_O_BINARY|_O_RDWR,_S_IREAD|_S_IWRITE);
if(handle == -1)
	{
	//printf("open '%s' failed\n",unsafe);
	free(unsafe);
	M_free(buf); // Free buffer up
	return 0;
	}
SAFE_STRCPY(outname,unsafe);
free(unsafe);
//printf("outname = %s\n",outname);
#endif

length=write(handle,buf,length); // Write it out
close(handle);

M_free(buf); // Free buffer up

strcpy(dest,outname);

// Don't free the tmpnam buffer or windows will go apeshit
return 1;
}
*/


/*
 *		load a CEL file using the IT-HE filesystem wrappers
 *		and without palette, so it can be used for lightmaps
 */

IRELIGHTMAP *load_lightmap(char *filename)
{
unsigned short w,h;                             // size of image
int len;
unsigned char header[800];				// header
IFILE *fp;
IRELIGHTMAP *img;

// Read the whole file

fp = iopen(filename);
// Good, we're still around.  Read in the header
iread(header,800,fp);

// Check it's valid
if(header[0] != 0x19 || header[1] != 0x91)
    ithe_panic("CEL header invalid for image",filename);

w = header[2]+(header[3]<<8);
h = header[4]+(header[5]<<8);
len = w*h;

// Allocate the sprite we will put the .CEL into

img = MakeIRELIGHTMAP(w,h);
if(!img || !img->GetFramebuffer())
    ithe_panic("Create bitmap failed for image",filename);
// Read the bitmap directly into memory
iread(img->GetFramebuffer(),len,fp);

// Close the file
iclose(fp);

return img;
}



void read_WORLD(WORLD *w,IFILE *f)
{
S_START();

	S_GETWM(w->sig);
	S_GETWI(w->w);
	S_GETWI(w->h);
	S_GETPTR(w->physmap);
	S_GETPTR(w->object);
	S_GETPTR(w->roof);
	S_GETPTR(w->objmap);
	S_GETPTR(w->light);
	S_GETPTR(w->lightst);
	S_GETWI(w->con_n);
	S_GETWI(w->con_s);
	S_GETWI(w->con_e);
	S_GETWI(w->con_w);
//	S_READ((unsigned char *)w->temp,25*sizeof(int *));
	S_READ((unsigned char *)w->temp,25*4);

//S_END(WORLD);
}


void read_USEDATA(USEDATA *u,IFILE *f)
{
int ctr=0;
int temp;

// 20 user slots

for(ctr=0;ctr<USEDATA_SLOTS;ctr++) {
	u->user[ctr]=igetl_i(f);
}

u->poison = igetl_i(f);
u->unconscious = igetl_i(f);

for(ctr=0;ctr<USEDATA_POTIONS;ctr++) {
	u->potion[ctr]=igetl_i(f);
}

u->dx = igetl_i(f);
u->dy = igetl_i(f);
u->vigilante = igetl_i(f);
u->edecor = igetl_i(f);
u->counter = igetl_i(f);
u->experience = igetl_i(f);
u->magic = igetl_i(f);
u->archive = igetl_i(f);
u->oldhp = igetl_i(f);

ctr = igetl_i(f);	// Junk - arcpocket
ctr = igetl_i(f);	// Junk - pathgoal

for(ctr=0;ctr<USEDATA_ACTSTACK;ctr++) {
	u->actlist[ctr]=igetl_i(f);
}

for(ctr=0;ctr<USEDATA_ACTSTACK;ctr++) {
	u->acttarget[ctr]=NULL;
	temp=igetl_i(f);	// Junk
}

ctr = igetl_i(f);	// Junk - actptr
ctr = igetl_i(f);	// Junk - actlen

u->fx_func = igetl_i(f);	// FX function (now in .mz1 file by preference)

ctr = igetl_i(f);	// Junk - NPCtalk
ctr = igetl_i(f);	// Junk - lFlags
}


void write_USEDATA(USEDATA *u,IFILE *f)
{
int ctr=0;
long pstart,pend;
char emsg[128];

pstart = itell(f);

// 20 user slots

for(ctr=0;ctr<USEDATA_SLOTS;ctr++) {
	iputl_i(u->user[ctr],f);
}

iputl_i(u->poison,f);
iputl_i(u->unconscious,f);

for(ctr=0;ctr<USEDATA_POTIONS;ctr++) {
	iputl_i(u->potion[ctr],f);
}

iputl_i(u->dx,f);
iputl_i(u->dy,f);
iputl_i(u->vigilante,f);
iputl_i(u->edecor,f);
iputl_i(u->counter,f);
iputl_i(u->experience,f);
iputl_i(u->magic,f);
iputl_i(u->archive,f);
iputl_i(u->oldhp,f);

iputl_i(0,f);	// ArcPocket ptr
iputl_i(0,f);	// Pathgoal ptr

for(ctr=0;ctr<USEDATA_ACTSTACK;ctr++) {
	iputl_i(u->actlist[ctr],f);
}

for(ctr=0;ctr<USEDATA_ACTSTACK;ctr++) {
	iputl_i(0,f);	// Junk for compatability
}

iputl_i(0,f);	// Junk - actptr
iputl_i(0,f);	// Junk - actlen

iputl_i(0,f);	// Junk - FX function

iputl_i(0,f);	// Junk - NPCtalk
iputl_i(0,f);	// Junk - lFlags

pend = itell(f);

if((pend - pstart) != USEDATA_VERSION) {
	sprintf(emsg,"%ld bytes instead of %d",pend-pstart,USEDATA_VERSION);
	ithe_panic("USEDATA saved wrong number of bytes, change VERSION code",emsg);
}

}



#define IF_CHANGED(v) if(o->v != CHlist[type].v)
#define IF_S_CHANGED(v) if(stricmp(o->v,CHlist[type].v))

void TWriteObject(FILE *fp,OBJECT *o, VMINT mapno)
{
int type,ctr;
char objectname[1024];

type = getnum4char(o->name);

if(!o->uid[0]) {
	//ithe_panic("No UID for object",o->name);
	printf("WARNING: No UID for object '%s' - object will not be saved to file!\n",o->name);
	ilog_quiet("WARNING: No UID for object '%s' - object will not be saved to file!\n",o->name);
	return;
}

// First line (declaration)
fprintf(fp,"\tobject %s\n",o->uid);

// Object type
SAFE_STRCPY(objectname,o->name);
strupr(objectname);
fprintf(fp,"\t\ttype %s\n",objectname);
// Location (coordinates or parent object)
if(o->parent.objptr && o->parent.objptr->uid[0])
	fprintf(fp,"\t\tinside %s\n",o->parent.objptr->uid);
else
	fprintf(fp,"\t\tat %ld %ld\n",o->x,o->y);


if(MZ1_SavingGame) // Do we care about these details?
	{
	// Animation Frame
	fprintf(fp,"\t\tform %s\n",o->form->name);

	// Sequence
	fprintf(fp,"\t\tsequence %ld %ld %ld\n",o->sptr,o->form->frames,o->sdir);
	}

// current direction
fprintf(fp,"\t\tcurdir %ld\n",o->curdir);

if(MZ1_SavingGame)
	{
	// Flags
	if(o->flags != CHlist[type].flags)
		fprintf(fp,"\t\tflags 0x%lx\n",o->flags);
	}

if(MZ1_SavingGame)
	{
	// Width and Height
	IF_CHANGED(w)
		fprintf(fp,"\t\twidth %ld\n",o->w);
	IF_CHANGED(h)
		fprintf(fp,"\t\theight %ld\n",o->h);

	// Width and Height (in tiles)
	IF_CHANGED(mw)
		fprintf(fp,"\t\tmwidth %ld\n",o->mw);
	IF_CHANGED(mh)
		fprintf(fp,"\t\tmheight %ld\n",o->mh);
	}

// z (unused)
IF_CHANGED(z)
	fprintf(fp,"\t\tz %ld\n",o->z);

// cost
IF_CHANGED(cost)
	fprintf(fp,"\t\tcost %ld\n",o->cost);

// Personal Name
if(o->personalname)
	if(stricmp(o->personalname,"-"))
		if(stricmp(o->personalname,""))
			fprintf(fp,"\t\tname %s\n",o->personalname);

// Target object
if(o->target.objptr)
	fprintf(fp,"\t\ttarget %s\n",o->target.objptr->uid);

// Target object
if(o->enemy.objptr)
	fprintf(fp,"\t\tenemy %s\n",o->enemy.objptr->uid);

// Tag
if(o->tag)
	fprintf(fp,"\t\ttag %ld\n",o->tag);

// Current activity
if(o->activity>0) {
	SAFE_STRCPY(objectname,PElist[o->activity].name);
	strupr(objectname);
	fprintf(fp,"\t\tactivity %s\n",objectname);
}

// Location tag (for long range pathfinder)
if(o->labels->location && strlen(o->labels->location)>0)
	fprintf(fp,"\t\tlabels->location %s\n",o->labels->location);

IF_CHANGED(light)
	fprintf(fp,"\t\tlight %ld\n",o->light);

//
//	Stats
//

IF_CHANGED(stats->hp)
	fprintf(fp,"\t\tstats->hp %ld\n",o->stats->hp);
IF_CHANGED(stats->dex)
	fprintf(fp,"\t\tstats->dex %ld\n",o->stats->dex);
IF_CHANGED(stats->str)
	fprintf(fp,"\t\tstats->str %ld\n",o->stats->str);
IF_CHANGED(stats->intel)
	fprintf(fp,"\t\tstats->intel %ld\n",o->stats->intel);
IF_CHANGED(stats->weight)
	fprintf(fp,"\t\tstats->weight %ld\n",o->stats->weight);
IF_CHANGED(maxstats->weight)
	fprintf(fp,"\t\tmaxstats->weight %ld\n",o->maxstats->weight);
IF_CHANGED(stats->quantity)
	fprintf(fp,"\t\tstats->quantity %ld\n",o->stats->quantity);
IF_CHANGED(stats->armour)
	fprintf(fp,"\t\tstats->armour %ld\n",o->stats->armour);
IF_CHANGED(stats->damage)
	fprintf(fp,"\t\tstats->damage %ld\n",o->stats->damage);
IF_CHANGED(stats->tick)
	fprintf(fp,"\t\tstats->tick %ld\n",o->stats->tick);
if(o->stats->owner.objptr)
	fprintf(fp,"\t\tstats->owner %s\n",o->stats->owner.objptr->uid);
IF_CHANGED(stats->karma)
	fprintf(fp,"\t\tstats->karma %ld\n",o->stats->karma);
IF_CHANGED(stats->bulk)
	fprintf(fp,"\t\tstats->bulk %ld\n",o->stats->bulk);
IF_CHANGED(stats->range)
	fprintf(fp,"\t\tstats->range %ld\n",o->stats->range);
IF_CHANGED(maxstats->speed)
	fprintf(fp,"\t\tstats->speed %ld\n",o->maxstats->speed);
IF_CHANGED(stats->level)
	fprintf(fp,"\t\tstats->level %ld\n",o->stats->level);
IF_CHANGED(stats->radius)
	fprintf(fp,"\t\tstats->radius %ld\n",o->stats->radius);
IF_CHANGED(stats->alignment)
	fprintf(fp,"\t\tstats->alignment %ld\n",o->stats->alignment);

if(MZ1_SavingGame)
	fprintf(fp,"\t\tstats->npcflags 0x%lx\n",o->stats->npcflags);

// Usedata - should probably put all future usedata in here rather than the savegame

if(MZ1_SavingGame && o->user) {

	// Original map for NPCs
	if(!o->user->originmap) {
		o->user->originmap = mapno;
	}
	fprintf(fp,"\t\tuser->originmap %ld\n",o->user->originmap);

	// Special effects
	if(o->user->fx_func > 0 && o->user->fx_func < PEtot) {
		fprintf(fp,"\t\tuser->fxfunc %s\n",PElist[o->user->fx_func].name);
	}
	if(o->user->fx_func < 0 && (-o->user->fx_func) < PEtot) {
		fprintf(fp,"\t\tuser->overfx %s\n",PElist[-(o->user->fx_func)].name);
	}
}

//
//  Funcs
//

IF_S_CHANGED(funcs->use)
	fprintf(fp,"\t\tfuncs->use %s\n",o->funcs->use);
IF_S_CHANGED(funcs->talk)
	fprintf(fp,"\t\tfuncs->talk %s\n",o->funcs->talk);
IF_S_CHANGED(funcs->kill)
	fprintf(fp,"\t\tfuncs->kill %s\n",o->funcs->kill);
IF_S_CHANGED(funcs->look)
	fprintf(fp,"\t\tfuncs->look %s\n",o->funcs->look);
IF_S_CHANGED(funcs->stand)
	fprintf(fp,"\t\tfuncs->stand %s\n",o->funcs->stand);
IF_S_CHANGED(funcs->hurt)
	fprintf(fp,"\t\tfuncs->hurt %s\n",o->funcs->hurt);
IF_S_CHANGED(funcs->init)
	fprintf(fp,"\t\tfuncs->init %s\n",o->funcs->init);
IF_S_CHANGED(funcs->resurrect)
	fprintf(fp,"\t\tfuncs->resurrect %s\n",o->funcs->resurrect);
IF_S_CHANGED(funcs->wield)
	fprintf(fp,"\t\tfuncs->wield %s\n",o->funcs->wield);
IF_S_CHANGED(funcs->horror)
	fprintf(fp,"\t\tfuncs->horror %s\n",o->funcs->horror);
IF_S_CHANGED(funcs->attack)
	fprintf(fp,"\t\tfuncs->attack %s\n",o->funcs->attack);
IF_S_CHANGED(funcs->quantity)
	fprintf(fp,"\t\tfuncs->quantity %s\n",o->funcs->quantity);
IF_S_CHANGED(funcs->user1)
	fprintf(fp,"\t\tfuncs->user1 %s\n",o->funcs->user1);
IF_S_CHANGED(funcs->user2)
	fprintf(fp,"\t\tfuncs->user2 %s\n",o->funcs->user2);

//
//  Schedules
//

if(o->schedule)
	{
	for(ctr=0;ctr<24;ctr++)
		if(o->schedule[ctr].active)
			{
			if(check_uuid(o->schedule[ctr].uuid))
				fprintf(fp,"\t\tschedule %d %d %s %s\n",o->schedule[ctr].hour,o->schedule[ctr].minute,o->schedule[ctr].vrm,o->schedule[ctr].uuid);
			else
				fprintf(fp,"\t\tschedule %d %d %s %s\n",o->schedule[ctr].hour,o->schedule[ctr].minute,o->schedule[ctr].vrm,"-");
			}
	}

fprintf(fp,"\n");
}


void put_uuid(OBJECT *obj, IFILE *ofp) {
unsigned char fakeuuid[UUID_SIZEOF];

if(obj && obj->uid[0]) {
	iwrite((unsigned char *)obj->uid,UUID_LEN,ofp);
	return;
}
// If it's null, improvise
memset(fakeuuid,0,UUID_LEN);
fakeuuid[0]='-';
iwrite(fakeuuid,UUID_LEN,ofp);
}

void get_uuid(char buf[UUID_SIZEOF], IFILE *ifp) {
iread((unsigned char *)buf,UUID_LEN,ifp);
buf[UUID_LEN]=0;
}


int check_uuid(const char *uuid) {
if(!uuid) {
	return 0;
}
if(uuid[0] == '-' && uuid[1] == 0) {
	return 0;
}
return strlen(uuid) == UUID_LEN;
}