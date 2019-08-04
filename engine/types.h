//
//	Data types
//

#ifndef __IRTYPES
#define __IRTYPES

// These three must match pointer size but support binary ops
typedef long VMINT;
typedef unsigned long VMUINT;
typedef unsigned long VMPTR;


typedef union VMTYPE
{
void *ptr;
unsigned long ul;
VMUINT u32;	// was int, but...
unsigned short u16;
unsigned char u8;
long l;
VMINT i32;	// This must be VMINT rather than int or we get sign truncation on AMD64
short i16;
char i8;
} VMTYPE;


typedef union OBJECTID
{
struct OBJECT *objptr;
unsigned int saveid;
} OBJECTID;


#endif
