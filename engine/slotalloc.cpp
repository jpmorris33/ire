//
//	Fast allocation caching used by the pathfinder
//

#include "slotalloc.hpp"
#include <stdlib.h>
#include <string.h>

//#define NO_SLOTS	// Only for debugging, Purge() is not implemented so it will leak if you do this

void *SLOTALLOC::Alloc()
{
#ifdef NO_SLOTS
	return calloc(1,EntrySize);
#else
int idx=AllocSlot();

if(idx<0)
	{
	char temp[128];
	sprintf(temp,"used all %d entries from %s",Slots,Slotname);
	ithe_panic("Slotalloc - out of slots!",temp);
	}

return Slot[idx];
#endif
}


void SLOTALLOC::Free(void *ptr)
{
#ifdef NO_SLOTS
free(ptr);
#else
int idx=FindSlot(ptr);
if(idx>=0)
	{
	memset(ptr,0,EntrySize);
	FreeSlot(idx);
	return;
	}

printf("Did not find %p from %s\n",ptr,Slotname);
#endif
}


void SLOTALLOC::Purge()
{
int ctr;
for(ctr=0;ctr<Slots;ctr++)
	{
	memset(Slot[ctr],0,EntrySize);
	if((ctr&31) == 0)
		Slotmap[ctr>>5]=0;
	}
}


//
//	32-bit based allocation mask.  We waste the rest on 64-bit, sadly
//

int SLOTALLOC::AllocSlot()
{
int ctr,slotpos,index,slots;
unsigned int slotbit;
slotpos=0;
slots=Slots/32;
for(ctr=0;ctr<slots;ctr++)
	if(Slotmap[ctr] != 0xffffffff)
		{
//printf("slot %d = %x\n",ctr,Slotmap[ctr]);
		slotpos=ctr*32;
		index=ctr;
		slotbit=1;
		for(ctr=0;ctr<32;ctr++)
			{
			if((Slotmap[index] & slotbit) == 0)
				{
//printf("Using bit %d (%x)\n",ctr,slotbit);
				// Found a slot, mark it used
				Slotmap[index] |= slotbit;
				// And set up the index
				slotpos += ctr;				
				break;
				}
			// move the mask along
			slotbit <<=1;
			}
		// Now we should have a valid slot index
//printf("Using slot %d\n",slotpos);
		return slotpos;
		}

return -1;	// Out of slots!
}



void SLOTALLOC::FreeSlot(int index)
{
int i,p;
unsigned int mask;

if(index<0)
	return;
i=index>>5;
p=index&31;
if(i>Slots)
	return;

mask = 1<<p;
Slotmap[i] &=~mask;

//printf("Freeing slot %d (%d, bit %d = %x) from %s\n",index,i,p,mask,Slotname);
}


int SLOTALLOC::FindSlot(void *ptr)
{
int ctr;
for(ctr=0;ctr<Slots;ctr++)
	if(Slot[ctr] == ptr)
		return ctr;
return -1;
}


SLOTALLOC::SLOTALLOC(const char *name, size_t size, int entries)
{
int ctr;
// Ensure it is divisible by 32, else pad it
if(entries&31)
	{
	entries &=~31;
	entries +=32;
	}

Slotname=name;
EntrySize=size;
Slots=entries;
Slotmap=(unsigned int *)M_get(entries/32,sizeof(unsigned int));

Slot=(void **)M_get(entries,sizeof(void *));
for(ctr=0;ctr<Slots;ctr++)
	Slot[ctr]=(void *)M_get(1,size);
}


SLOTALLOC::~SLOTALLOC()
{
int ctr=0;
M_free(Slotmap);

for(ctr=0;ctr<Slots;ctr++)
	if(Slot[ctr])
		M_free(Slot[ctr]);
M_free(Slot);
}