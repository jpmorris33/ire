//
//	Key buffering code for the main game engine
//

#include <string.h>
#include "../iregraph.hpp"

#define KBSIZE 2 // Size of keyboard buffer

static int ikeybuf_rptr=0; // Key buffer code
static int ikeybuf_wptr=0;
static int ikeybuf[KBSIZE+1];


// This is a ring buffer

int IRE_ReadKeyBuffer()
{
int k;

// Take key from buffer if necessary
IRE_WaitFor(0);
IRE_GetKeys();
if(!IRE_KeyPressed())
	{
	k = ikeybuf[ikeybuf_rptr];
	if(!k)
		{
		return k; // Abort if nothing is there
		}
	ikeybuf[ikeybuf_rptr++]=0;
	// Wrap the buffer around if we go too far
	if(ikeybuf_rptr>=KBSIZE)
		ikeybuf_rptr=0;
	return k;
	}

// Fill the buffer

while(IRE_KeyPressed())
	{
	if(ikeybuf[ikeybuf_wptr]) // It's full.
		{
		IRE_ClearKeyBuf();
		}
	else
		{
		ikeybuf[ikeybuf_wptr]=IRE_NextKey(NULL);
		if(IRE_TestShift(IRESHIFT_SHIFT))
			ikeybuf[ikeybuf_wptr] |= IREKEY_SHIFTMOD;
		if(IRE_TestShift(IRESHIFT_CONTROL))
			ikeybuf[ikeybuf_wptr] |= IREKEY_CTRLMOD;
		ikeybuf_wptr++;
		if(ikeybuf_wptr>=KBSIZE)
			{
			ikeybuf_wptr=0;
			}
		}
	};

k = ikeybuf[ikeybuf_rptr];
ikeybuf[ikeybuf_rptr++]=0;
if(ikeybuf_rptr>=KBSIZE)
	{
	ikeybuf_rptr=0;
	}

return k;
}

// Flush the buffer

void IRE_FlushKeyBuffer()
{
IRE_ClearKeyBuf();
ikeybuf_rptr=0;
ikeybuf_wptr=0;
memset(ikeybuf,0,sizeof(ikeybuf));
}
