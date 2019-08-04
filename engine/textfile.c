/*
 *      IRE - text file routines, loading and indexing thru' VFS layer
 */

#define NO_FORTIFY		// Disable FORTIFY here (for speed)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "textfile.h"
//#include "loadfile.hpp"
#include "ithelib.h"

#define FUDGE_CHARACTER -2	// This is for string munging

static void fix_quotes(char *line);
static void unfix_quotes(char *line);
extern int loadfile(const char *filename, char *fn);


// set values to reasonable defaults

void TF_init(struct TF_S *s)
{
if(!s)
	return;
s->size=0;
s->lines=0;
s->linewords=0;
s->blocks=1;
s->block = (char **)calloc(1,sizeof(char *));
s->line=NULL;
s->lineword=NULL;
}

// free memory if necessary and set to defaults

void TF_term(struct TF_S *s)
{
int ctr;

if(s->size)
	{
	if(s->block) {
		for(ctr=0;ctr<s->blocks;ctr++)
			if(s->block[ctr])
				free(s->block[ctr]);
		free(s->block);
		s->block=NULL;
		}
	if(s->lineword)
		free(s->lineword);

	s->size=0;
	s->blocks=0;
	}

if(s->lines)
	{
	if(s->line)
		free(s->line);
	s->lines=0;
	}
}

// open file, alloc memory and index by line

int TF_load(struct TF_S *s, const char *filename)
{
int ctr,current;
char fn[1024];
IFILE *fp;
char *blockptr;

if(!s)
	return 0;
if(!s->block || s->block[0])
	return 0;	// You're supposed to use Merge for this

if(!loadfile(filename,fn))
	ithe_panic("Could not open text file:",filename);

fp=iopen(fn);          // Open file
s->size = ifilelength(fp);
blockptr = (char *)calloc(1,s->size+1);    // Alloc memory for block, and a NULL
if(!blockptr) {
	iclose(fp);
	return 0;
	}

iread((unsigned char *)blockptr,s->size,fp);          // Read the file into block
iclose(fp);                        // Finished

s->lines = 1;                     // Count Carriage Returns

for(ctr = 0;ctr < s->size;ctr++)
	if(blockptr[ctr] == '\n')   // Found one
		s->lines++;

// We now know how many lines there are, so allocate the line index
s->line = (char **)calloc(s->lines,sizeof(char *));
if(!s->line) {
	free(blockptr);
	return 0;
	}

// Now we go through the lines indexing them.
current = 0;                                	// First line
s->line[0] = blockptr;                        	// This is the first line

for(ctr = 0;ctr < s->size;ctr++)
	if(blockptr[ctr] == '\n')               	// Found a line break
		{
		blockptr[ctr] = 0;                   	// End the string

		// If we've got an MSDOS-style CR/LF format
		if(ctr>0)
			if(blockptr[ctr-1] == 13)
				blockptr[ctr-1]=0; // Kill it
		s->line[++current] = &blockptr[ctr+1];  // This is the next line
		}
s->block[0] = blockptr;
return 1;
}

// Split loaded file into words

void TF_splitwords(struct TF_S *s, char comment)
{
int ctr,ctr2,len,word,maxwords;
char *cptr;

if(!s)
	return;
if(s->linewords)
	return;

// Allocate the line word index
s->linewords = (int *)calloc(s->lines,sizeof(int *));
s->lineword = (char ***)calloc(s->lines,sizeof(char **));

// Now we go through the lines indexing them.

for(ctr = 0;ctr < s->lines;ctr++)
	{
	// Smash any comments
	cptr=strchr(s->line[ctr],comment);
	if(cptr)
		*cptr=0;

	fix_quotes(s->line[ctr]);
	// Count number of words in the line

	len=strcount(s->line[ctr]);
	s->linewords[ctr]=len;
	if(!len)
		{
		s->lineword[ctr]=NULL;
		continue; // Round again
		}

	// Reserve space for at least 10 words (parser expects NULL for checking)
	maxwords=16;
	if(len>16)
		maxwords=len;
	s->lineword[ctr]=calloc(maxwords,sizeof(char **));

	// Find the gaps between each word

	cptr=s->line[ctr];
	word=0;
	do
		{
		if(*cptr == ' ')
			cptr++;
		else
			{
			s->lineword[ctr][word++]=cptr;
			cptr=strchr(cptr,' ');
			if(cptr)
				*cptr++=0;
			else
				break; // Found the end
			}

		} while(*cptr);

	// Fix the quotes
	for(ctr2=0;ctr2<word;ctr2++)
		unfix_quotes(s->lineword[ctr][ctr2]);
	}
}


// open file, alloc memory and index by line

int TF_merge(struct TF_S *olds, const char *filename, int insert)
{
int ctr,current;
char fn[1024];
IFILE *fp;
char *blockptr;
struct TF_S *s,news;
char **newblock;

s=&news;
TF_init(s);

if(!olds)
	return 0;

if(!olds->blocks)
	return 0;

if(!loadfile(filename,fn))
	ithe_panic("Could not open text file:",filename);

fp=iopen(fn);          // Open file
s->size = ifilelength(fp);
blockptr = (char *)calloc(1,s->size+1);    // Alloc memory for block, and a NULL
if(!blockptr) {
	iclose(fp);
	return 0;
	}

iread((unsigned char *)blockptr,s->size,fp);          // Read the file into block
iclose(fp);                        // Finished

s->lines = 1;                     // Count Carriage Returns

for(ctr = 0;ctr < s->size;ctr++)
	if(blockptr[ctr] == '\n')   // Found one
		s->lines++;

// We now know how many lines there are, so allocate the line index
s->line = (char **)calloc(s->lines,sizeof(char *));
if(!s->line) {
	free(blockptr);
	return 0;
	}

// Now we go through the lines indexing them.
current = 0;                                	// First line
s->line[0] = blockptr;                        	// This is the first line

for(ctr = 0;ctr < s->size;ctr++)
	if(blockptr[ctr] == '\n')               	// Found a line break
		{
		blockptr[ctr] = 0;                   	// End the string

		// If we've got an MSDOS-style CR/LF format
		if(ctr>0)
			if(blockptr[ctr-1] == 13)
				blockptr[ctr-1]=0; // Kill it
		s->line[++current] = &blockptr[ctr+1];  // This is the next line
		}

// Can we add it the easy way?  e.g. if someone is calling 'merge' on an empty file?
for(ctr=0;ctr<olds->blocks;ctr++)
	if(!olds->block[ctr]) {
		olds->block[ctr]=blockptr;
		blockptr=NULL;
		break;
		}

// Add the new block to the list, stretching it as necessary
if(blockptr) {
	newblock = (char **)realloc(olds->block,(olds->blocks+1)*sizeof(char *));
	if(!newblock) {
		TF_term(s);
		return 0;
		}
	olds->block = newblock;
	newblock[olds->blocks] = blockptr;
	olds->blocks++;
	blockptr = NULL;
	}

// Now we need to merge the line lists
if(insert < 0)
	insert=olds->lines; // Add to end
if(insert >= olds->lines)
	insert=olds->lines; // Sanity check

newblock = (char **)calloc(s->lines+olds->lines+1,sizeof(char *));
if(!newblock) {
	TF_term(s);
	return 0;
	}

// Add the preamble
current=0;
for(ctr=0;ctr<insert;ctr++) {
	newblock[current]=olds->line[ctr];
	current++;
	}
	
// Insert the new file
for(ctr=0;ctr<s->lines;ctr++) {
	newblock[current]=s->line[ctr];
	current++;
	}

// Add the rest of the original file
for(ctr=insert;ctr<olds->lines;ctr++) {
	newblock[current]=olds->line[ctr];
	current++;
	}

// Swap in new line list
free(olds->line);
olds->line=newblock;
olds->lines = s->lines+olds->lines;

TF_term(s);

return 1;
}


//   Turn all spaces inside quotes to solid mass of guck for parameter system

static void fix_quotes(char *line)
{
char *p,inquote;

inquote=0;
for(p=line;*p;p++)
	{
	if(*p == '\"')
		inquote=!inquote;
	if(inquote && isxspace(*p))
		*p=FUDGE_CHARACTER;
	}
}

//   Undo the operation of the previous function

void unfix_quotes(char *line)
{
char *p;
do
	{
	p=strchr(line,FUDGE_CHARACTER);
	if(p)
		*p=' ';
	} while(p);
}

