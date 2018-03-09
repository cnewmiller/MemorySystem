//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#include "Heap.h"
#include "Mem.h"

Heap::Heap(void * ptr)
	: pUsedHead(0),
	pFreeHead(0),
	mInitialize(true),
	bytePad1(0),
	bytePad2(0),
	bytePad3(0),
	pNextFit(0),
	pad2(0),
	pad3(0)
{
	mStats.peakNumUsed = 0;			// number of peak used allocations
	mStats.peakUsedMemory = 0;		// peak size of used memory

	mStats.currNumUsedBlocks = 0;		// number of current used allocations
	mStats.currUsedMem = 0;			// current size of the total used memory

	mStats.currNumFreeBlocks = 0;		// number of current free blocks
	mStats.currFreeMem = 0;			// current size of the total free memory

	mStats.heapTopAddr = reinterpret_cast<void *> ((Type::U8 *)ptr + sizeof(Heap));		// start address available heap
	mStats.heapBottomAddr = reinterpret_cast<void *> ((Type::U8 *)ptr + Mem::HEAP_SIZE);		// last address available heap

	mStats.sizeHeap = (Type::U32)mStats.heapBottomAddr - (Type::U32)mStats.heapTopAddr + sizeof(Heap);				// size of Heap total space, including header
	mStats.sizeHeapHeader = sizeof(Heap);		// size of heap header
}

void Heap::addFree(FreeHdr *toInsert)
{
	//insert in empty list
	if (nullptr == this->pFreeHead) {
		this->pFreeHead = toInsert;
		toInsert->pFreeNext = nullptr;
		toInsert->pFreePrev = nullptr;
	}
	else {
		FreeHdr * tracer = this->pFreeHead;

		//Loop 2 of 2: iterate to insert a free block in sorted order
		while (nullptr != tracer) {
			if (toInsert < tracer) {
				
				toInsert->pFreePrev = tracer->pFreePrev;
				toInsert->pFreeNext = tracer;
				
				if (nullptr == tracer->pFreePrev) {
					this->pFreeHead = toInsert;
					tracer->pFreePrev = toInsert;
				}
				else {
					tracer->pFreePrev->pFreeNext = toInsert;
				}
				tracer->pFreePrev = toInsert;
				tracer = nullptr;
			}
			else if (nullptr == tracer->pFreeNext) { //insert at end of list
				tracer->pFreeNext = toInsert;
				toInsert->pFreePrev = tracer;
				toInsert->pFreeNext = nullptr;
				tracer = nullptr;
			}
			else {
				tracer = tracer->pFreeNext;
			}
			
		}
	}

	//update stats
	this->mStats.currFreeMem += (toInsert->mBlockSize);
	this->mStats.currNumFreeBlocks++;

	initSecretPtr(toInsert);

}

void Heap::removeFree(FreeHdr *toRemove)
{
	//not end
	if (toRemove->pFreeNext != nullptr) {
		toRemove->pFreeNext->pFreePrev = toRemove->pFreePrev;
	}

	//not beginning
	if (toRemove->pFreePrev != nullptr) {
		toRemove->pFreePrev->pFreeNext = toRemove->pFreeNext;
	}

	//if this block is the first in the free list, update the head
	if (toRemove == this->pFreeHead) {
		this->pFreeHead = this->pFreeHead->pFreeNext;
		if (this->pFreeHead != nullptr) {
			this->pFreeHead->pFreePrev = nullptr;
		}
	}

	//update the nextFit pointer to the next free
	if (this->pNextFit == toRemove) {
		this->pNextFit = toRemove->pFreeNext;
	}

	//wrap around
	if (this->pNextFit == nullptr) {
		this->pNextFit = this->pFreeHead;
	}

	this->mStats.currNumFreeBlocks--;


	this->mStats.currFreeMem -= (toRemove->mBlockSize);
}


void Heap::addUsed(UsedHdr *toInsert)
{
	//push to front
	toInsert->pUsedNext = this->pUsedHead;
	if (this->pUsedHead != nullptr) {
		this->pUsedHead->pUsedPrev = toInsert;
	}
	this->pUsedHead = toInsert;
	this->pUsedHead->pUsedPrev = nullptr;


	this->mStats.currUsedMem += (toInsert->mBlockSize);
	this->mStats.currNumUsedBlocks++;

	if (this->mStats.currUsedMem > this->mStats.peakUsedMemory) {
		this->mStats.peakUsedMemory = this->mStats.currUsedMem;
	}
	if (this->mStats.currNumUsedBlocks > this->mStats.peakNumUsed) {
		this->mStats.peakNumUsed = this->mStats.currNumUsedBlocks;
	}

}

void Heap::removeUsed(UsedHdr *toRemove)
{
	//remove from front
	if (this->pUsedHead == toRemove) {
		this->pUsedHead = toRemove->pUsedNext;
		if (nullptr != this->pUsedHead) {
			this->pUsedHead->pUsedPrev = nullptr;
		}
	}
	else {//remove from somewhere in the list

		//remove head
		if (nullptr != toRemove->pUsedPrev) {
			toRemove->pUsedPrev->pUsedNext = toRemove->pUsedNext;
		}
		//remove tail
		if (nullptr != toRemove->pUsedNext) {
			toRemove->pUsedNext->pUsedPrev = toRemove->pUsedPrev;
		}

	}

	if (nullptr == this->pNextFit) {
		this->pNextFit = (FreeHdr*)toRemove;
	}

	//if the chain is empty
	if (--this->mStats.currNumUsedBlocks == 0) {
		this->pUsedHead = nullptr;
	}
	this->mStats.currUsedMem -= toRemove->mBlockSize;
}

//initializes the secret pointer for the bottom of the parameter pointer
void Heap::initSecretPtr(FreeHdr *free)
{
	FreeHdr* next = free->getEnd();
	FreeHdr** secretPtr = (FreeHdr**)next-1;
	*secretPtr = free;
}




void Heap::mergeStretchThisBlockUp(FreeHdr *toStretch)
{
	if (toStretch->mAboveBlockFree) {
		FreeHdr *above = *((FreeHdr**)toStretch - 1);
		FreeHdr* currentNextFit = this->pNextFit;
		FreeHdr* afterMergePrev = toStretch->pFreePrev;
		FreeHdr* afterMergeNext = toStretch->pFreeNext;
		FreeHdr* preMergeHead = this->pFreeHead;

		this->removeFree(toStretch);
		this->removeFree(above);
		above->mBlockSize += toStretch->mBlockSize + sizeof(FreeHdr);

		if (nullptr == this->pFreeHead || preMergeHead == toStretch) {
			this->pFreeHead = above;
			above->pFreeNext = preMergeHead->pFreeNext;
			if (preMergeHead->pFreeNext != nullptr) {
				above->pFreeNext->pFreePrev = above;
			}
			above->pFreePrev = nullptr;
		}
		else {

			above->pFreePrev = afterMergePrev;
			above->pFreeNext = afterMergeNext;

			if (nullptr == above->pFreePrev) {
				this->pFreeHead = above;
			}
			else {
				above->pFreePrev->pFreeNext = above;
			}
			if (nullptr != above->pFreeNext) {
				above->pFreeNext->pFreePrev = above;
			}
		}

		this->mStats.currFreeMem += (above->mBlockSize);
		this->mStats.currNumFreeBlocks++;

		initSecretPtr(above);

		if (currentNextFit == above || currentNextFit == toStretch) {
			this->pNextFit = above;
		}
	}
}

void Heap::mergeStretchThisBlockDown(FreeHdr *toStretch)
{
	FreeHdr* next = toStretch->getEnd();
	if (next < this->mStats.heapBottomAddr && (int)BlockType::FREE == next->mBlockType) {
		FreeHdr* currentNextFit = this->pNextFit;
		FreeHdr* afterMergePrev = toStretch->pFreePrev;
		FreeHdr* afterMergeNext = toStretch->pFreeNext;

		this->removeFree(toStretch);
		this->removeFree(next);

		toStretch->mBlockSize += next->mBlockSize + sizeof(FreeHdr);
		
		if (nullptr == this->pFreeHead || this->pFreeHead > toStretch) {
			FreeHdr* oldHead = pFreeHead;
			this->pFreeHead = toStretch;
			toStretch->pFreeNext =oldHead;
			if (toStretch->pFreeNext != nullptr) {
				toStretch->pFreeNext->pFreePrev = toStretch;
			}

			toStretch->pFreePrev = nullptr;
		}
		else {
			toStretch->pFreePrev = afterMergePrev;
			toStretch->pFreeNext = afterMergeNext;

			if (nullptr == toStretch->pFreePrev) {
				this->pFreeHead = toStretch;
			}
			else {
				toStretch->pFreePrev->pFreeNext = toStretch;
			}
			if (nullptr != toStretch->pFreeNext) {
				toStretch->pFreeNext->pFreePrev = toStretch;
			}


		}
		this->mStats.currFreeMem += (toStretch->mBlockSize);
		this->mStats.currNumFreeBlocks++;
		


		initSecretPtr(toStretch);


		if (currentNextFit == next || currentNextFit == toStretch) {
			this->pNextFit = toStretch;
		}
	}
}



// ---  End of File ---------------
