//
//	Fast allocation caching used by the pathfinder
//

#include "ithelib.h"

class SLOTALLOC
	{
	public:
	SLOTALLOC(const char *slotname, size_t size, int quantity);
	~SLOTALLOC();
	void *Alloc();
	void Free(void *a);
	void Purge();
	private:
		int AllocSlot();
		void FreeSlot(int slot);
		int FindSlot(void *ptr);

		int Slots;
		unsigned int *Slotmap;
		void **Slot;
		int EntrySize;
		const char *Slotname;
	};

//
