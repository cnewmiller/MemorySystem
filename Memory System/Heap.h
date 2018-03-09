//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef HEAPHDR_H
#define HEAPHDR_H

#include "Types.h"

#include "UsedHdr.h"
#include "FreeHdr.h"

class Heap
{
public:
	struct Stats
	{
		Type::UInt peakNumUsed;		// number of peak used allocations
		Type::UInt peakUsedMemory;		// peak size of used memory

		Type::UInt currNumUsedBlocks;	// number of current used allocations
		Type::UInt currUsedMem;		// current size of the total used memory

		Type::UInt	currNumFreeBlocks;	// number of current free blocks
		Type::UInt currFreeMem;		// current size of the total free memory

		Type::UInt sizeHeap;			// size of Heap total space, including header
		Type::UInt sizeHeapHeader;		// size of heap header

		void *heapTopAddr;		// start address available heap
		void *heapBottomAddr;    // bottom of address of heap
	};


	void addFree(FreeHdr*);
	void removeFree(FreeHdr*);
	void addUsed(UsedHdr*);
	void removeUsed(UsedHdr*);

	void merge(FreeHdr*);

public:
	// Make sure that the Heap is 16 byte aligned.

	// allocation links
	UsedHdr		*pUsedHead;
	FreeHdr		*pFreeHead;

	Type::Bool	mInitialize;
	Type::U8	bytePad1;
	Type::U8	bytePad2;
	Type::U8	bytePad3;

	FreeHdr		*pNextFit;
	Type::U32	pad2;
	Type::U32	pad3;

	Stats		mStats;		

	// specialize constructor
	Heap(void * ptr);

};

#endif 

// ---  End of File ---------------
