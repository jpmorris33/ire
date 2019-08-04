#ifndef __IRE__SCRIPT__
#define __IRE__SCRIPT__

#include "core.hpp"

extern void Compile(char *file);
extern void Restring();

/*
extern int getnum4char(char *name);     // Find index in array, of a character
extern int getnum4sequence(char *name); // Find index in array, of a sequence
extern int getnum4sprite(char *name);   // Find index in array, of a sprite
extern int getnum4VRM(char *name);      // Find index in array, of a VRM
*/

//extern char *rest(char *input);         // Get pointer to tail of string
extern char *last(char *input);         // Get pointer to end of tail
extern char *first(char *input);        // Get COPY of head of string
extern char *hardfirst(char *line);     // Mutilate original string
extern void Strip(char *tbuf);          // Eliminate trailing spaces

extern int FindRooftile(const char *name);

#endif
