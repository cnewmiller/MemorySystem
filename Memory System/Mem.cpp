//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#include <malloc.h>
#include <new>
#include <assert.h>

#include "FileIO.h"

#include "Mem.h"
#include "Heap.h"
#include "BlockType.h"

#define STUB_PLEASE_REPLACE(x) (x)

#define HEAP_ALIGNMENT			16
#define HEAP_ALIGNMENT_MASK		(HEAP_ALIGNMENT-1)

#define ALLOCATION_ALIGNMENT		16
#define ALLOCATION_ALIGNMENT_MASK	(ALLOCATION_ALIGNMENT-1)

#define UNUSED_VAR(v)  ((void *)v)

#ifdef _DEBUG
	#define HEAP_HEADER_GUARDS  16
	#define HEAP_SET_GUARDS  	Type::U32 *pE = (Type::U32 *)((Type::U32)pRawMem + HEAP_SIZE); \
								*pE++ = 0xEEEEEEEE;*pE++ = 0xEEEEEEEE;*pE++ = 0xEEEEEEEE;*pE++ = 0xEEEEEEEE;
	#define HEAP_TEST_GUARDS	Type::U32 *pE = (Type::U32 *)((Type::U32)pRawMem + HEAP_SIZE); \
								assert(*pE++ == 0xEEEEEEEE);assert(*pE++ == 0xEEEEEEEE); \
								assert(*pE++ == 0xEEEEEEEE);assert(*pE++ == 0xEEEEEEEE);  
#else
	#define HEAP_HEADER_GUARDS  0
	#define HEAP_SET_GUARDS  	
	#define HEAP_TEST_GUARDS			 
#endif
							

struct SecretPtrs
{
	FreeHdr *free;
};


Mem::~Mem()
{
	HEAP_TEST_GUARDS
	_aligned_free(this->pRawMem);
}


Heap *Mem::GetHeap()
{
	return this->pHeap;
}

Mem::Mem()
{
	// now initialize it.
	this->pHeap = 0;
	this->pRawMem = 0;

	// Do a land grab --- get the space for the whole heap
	// Since OS have different alignments... I forced it to 16 byte aligned
	pRawMem = _aligned_malloc(HEAP_SIZE + HEAP_HEADER_GUARDS, HEAP_ALIGNMENT);
	HEAP_SET_GUARDS

	// verify alloc worked
	assert(pRawMem != 0);

	// Guarantee alignemnt
	assert( ((Type::U32)pRawMem & HEAP_ALIGNMENT_MASK) == 0x0 ); 

	// instantiate the heap header on the raw memory
	Heap *p = new(pRawMem) Heap(pRawMem);

	// update it
	this->pHeap = p;
}


void Mem::Initialize()
{
	this->pHeap->pFreeHead = new(pHeap->mStats.heapTopAddr) FreeHdr();
	this->pHeap->pFreeHead->mBlockSize = (Type::U32)(((Type::U8*)this->pHeap->mStats.heapBottomAddr - (Type::U8*)pHeap->pFreeHead) - sizeof(FreeHdr));
	this->pHeap->pNextFit = this->pHeap->pFreeHead;

	this->pHeap->pFreeHead->pFreeNext = 0;
	this->pHeap->pFreeHead->pFreePrev = 0;
	this->pHeap->pFreeHead->mAboveBlockFree = false;

	this->pHeap->mStats.currNumFreeBlocks = 1;
	this->pHeap->mStats.currFreeMem = this->pHeap->pFreeHead->mBlockSize;

}

void Mem::changeFreeHeaderToUsed(const FreeHdr * const free) const //what the fuck am i even doing...
{

	Heap::Stats* stats = &this->pHeap->mStats;
	stats->currNumFreeBlocks--; //what the fuck though
	stats->currFreeMem -= (free->mBlockSize); //so does the amount of free memory include the header???


	if (++stats->currNumUsedBlocks > stats->peakNumUsed) {
		stats->peakNumUsed = stats->currNumUsedBlocks;
	}
	stats->currUsedMem += free->mBlockSize;
	if (stats->currUsedMem > stats->peakUsedMemory) {
		stats->peakUsedMemory = stats->currUsedMem;
	}
}

//mBlockSize does NOT count the header size!
//the header is NOT in the memory block, so to get to the next one you have to include the header size
//    HHBBBBBBBB: block size 8, 10 blocks used
void *Mem::Malloc( const Type::U32 size )
{
	
	//round size up to the nearest 16

	FreeHdr *tracer = pHeap->pNextFit;
	while (tracer != nullptr) { //will need to change to != pNextFit or similar
		if (tracer->mBlockSize >= size) {

			tracer->mBlockType = (int)BlockType::USED;
			pHeap->removeFree(tracer);

			unsigned int oldSize = tracer->mBlockSize;
			tracer->mBlockSize = size;

			notifyNextBlockDown(tracer, false);

			createFreeBlockFromExtraSpace(oldSize, size, tracer);
			
			
			pHeap->addUsed((UsedHdr*)tracer);

			return tracer;
		}
		tracer = tracer->pFreeNext;
		//if the tracer hit the end, restart at the beginning of the list
	}
	return nullptr;
}

void Mem::createFreeBlockFromExtraSpace(unsigned int oldSize, const unsigned int size, FreeHdr * tracer)
{
	if (oldSize > size + sizeof(FreeHdr)) { //making a new header
		FreeHdr* next = tracer->getEnd();
		next->mAboveBlockFree = false;
		next->mBlockSize = oldSize - (size + sizeof(FreeHdr));
		next->mBlockType = (int)BlockType::FREE;
		//if the parent block had nextFree, this one has nextFree

		pHeap->addFree(next);
		//pHeap->mStats.currFreeMem -= sizeof(FreeHdr);
		pHeap->pNextFit = next;
	}
}

void Mem::notifyNextBlockDown(FreeHdr * tracer, bool newValue)
{
	if (tracer->getEnd() < pHeap->mStats.heapBottomAddr) {
		tracer->getEnd()->mAboveBlockFree = newValue;
	}
}


void Mem::Free( void * const data )
{
	FreeHdr* toFree = (FreeHdr*)data;

	//if (isNextFit || nextGuy->isNextFit) nextFit = merge
	toFree->mBlockType = (int)BlockType::FREE;
	pHeap->removeUsed((UsedHdr*)toFree);
	pHeap->addFree(toFree);
	

	pHeap->merge(toFree);

	//FreeHdr *next = toFree->getEnd();
	//next->mAboveBlockFree = true;
	//next--;

	//if (toFree->mAboveBlockFree) {
	//	//hello
	//	(void)toFree;
	//}

}


void Mem::Dump()
{

	fprintf(FileIO::GetHandle(),"\n------- DUMP -------------\n\n");

	fprintf(FileIO::GetHandle(), "heapStart: 0x%p     \n", this->pHeap );
	fprintf(FileIO::GetHandle(), "  heapEnd: 0x%p   \n\n", this->pHeap->mStats.heapBottomAddr );
	fprintf(FileIO::GetHandle(), "pUsedHead: 0x%p     \n", this->pHeap->pUsedHead );
	fprintf(FileIO::GetHandle(), "pFreeHead: 0x%p     \n", this->pHeap->pFreeHead );
	fprintf(FileIO::GetHandle(), " pNextFit: 0x%p   \n\n", this->pHeap->pNextFit);

	fprintf(FileIO::GetHandle(),"Heap Hdr   s: %p  e: %p                            size: 0x%x \n",(void *)((Type::U32)this->pHeap->mStats.heapTopAddr-sizeof(Heap)), this->pHeap->mStats.heapTopAddr, this->pHeap->mStats.sizeHeapHeader);

	Type::U32 p = (Type::U32)pHeap->mStats.heapTopAddr;

	char *type;
	char *typeHdr;

	while( p < (Type::U32)pHeap->mStats.heapBottomAddr )
	{
		UsedHdr *used = (UsedHdr *)p;
		if( used->mBlockType == (Type::U8)BlockType::USED )
		{
			typeHdr = "USED HDR ";
			type    = "USED     ";
		}
		else
		{
			typeHdr = "FREE HDR ";
			type    = "FREE     ";
		}

		Type::U32 hdrStart = (Type::U32)used;
		Type::U32 hdrEnd   = (Type::U32)used + sizeof(UsedHdr);
		fprintf(FileIO::GetHandle(),"%s  s: %p  e: %p  p: %p  n: %p  size: 0x%x    AF: %d \n",typeHdr, (void *)hdrStart, (void *)hdrEnd, used->pUsedPrev, used->pUsedNext, sizeof(UsedHdr), used->mAboveBlockFree );
		Type::U32 blkStart = hdrEnd;
		Type::U32 blkEnd   = blkStart + used->mBlockSize; 
		fprintf(FileIO::GetHandle(),"%s  s: %p  e: %p                            size: 0x%x \n",type, (void *)blkStart, (void *)blkEnd, used->mBlockSize );

		p = blkEnd;
	
	}
}

// ---  End of File ---------------
