//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#include "Heap.h"
#include "Mem.h"

Heap::Heap(void * ptr)
:	pUsedHead(0),
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
	mStats.peakUsedMemory  = 0;		// peak size of used memory

	mStats.currNumUsedBlocks =0;		// number of current used allocations
	mStats.currUsedMem =0;			// current size of the total used memory

	mStats.currNumFreeBlocks =0;		// number of current free blocks
	mStats.currFreeMem =0 ;			// current size of the total free memory

	mStats.heapTopAddr = reinterpret_cast<void *> ( (Type::U8 *)ptr + sizeof(Heap) );		// start address available heap
	mStats.heapBottomAddr = reinterpret_cast<void *> ( (Type::U8 *)ptr + Mem::HEAP_SIZE );		// last address available heap
	
	mStats.sizeHeap = (Type::U32)mStats.heapBottomAddr - (Type::U32)mStats.heapTopAddr + sizeof(Heap);				// size of Heap total space, including header
	mStats.sizeHeapHeader = sizeof(Heap);		// size of heap header
}

void Heap::addFree(FreeHdr *toInsert)
{
	//not updating the next pointer i guess?

	//insert in empty list
	if (nullptr == this->pFreeHead) {
		this->pFreeHead = toInsert;
		toInsert->pFreeNext = nullptr;
		toInsert->pFreePrev = nullptr;
	}
	else {
		FreeHdr * tracer = this->pFreeHead;
		while (nullptr != tracer) {
			if (toInsert < tracer) {
				//insert in middle
				toInsert->pFreePrev = tracer->pFreePrev;
				toInsert->pFreeNext = tracer;
				tracer->pFreePrev = toInsert;
				if (nullptr == tracer->pFreePrev) {//insert at head of list
					//toInsert->pFreeNext = this->pFreeHead;
					this->pFreeHead = toInsert;
					this->pNextFit = toInsert;
				}
				else {
					tracer->pFreePrev->pFreeNext = toInsert;
				}
			}
			else if (nullptr == tracer->pFreeNext) { //insert at end of list
				tracer->pFreeNext = toInsert;
				toInsert->pFreePrev = tracer;
				toInsert->pFreeNext = nullptr;
			}
			tracer = tracer->pFreeNext;
		}
	}
	

	//update stats
	this->mStats.currFreeMem += (toInsert->mBlockSize);
	this->mStats.currNumFreeBlocks++;
	

}

void Heap::removeUsed(UsedHdr *toRemove)
{
	//remove from empty
	if (nullptr == this->pUsedHead) {
		return;
	}
	//remove from front
	if (this->pUsedHead == toRemove) {
		this->pUsedHead = toRemove->pUsedNext;
		if (nullptr != this->pUsedHead) {
			this->pUsedHead->pUsedPrev = nullptr;
		}
	}
	else {//remove from somewhere in the list


		//just making sure
		if (nullptr != toRemove->pUsedPrev) {
			toRemove->pUsedPrev->pUsedNext = toRemove->pUsedNext;
		}
		//remove from the end
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

void Heap::removeFree(FreeHdr *toRemove)
{
	if (toRemove->pFreeNext != nullptr) {
		toRemove->pFreeNext->pFreePrev = toRemove->pFreePrev;
	}
	//if this block is the first in the free list, update the head
	if (toRemove == this->pFreeHead) {
		this->pFreeHead = this->pFreeHead->pFreeNext;
	}
	//update the nextFit pointer to the next free
	this->pNextFit = toRemove->pFreeNext;


	this->mStats.currNumFreeBlocks--;
	this->mStats.currFreeMem -= (toRemove->mBlockSize);



}

void Heap::addUsed(UsedHdr *toRemove)
{
	toRemove->pUsedNext = this->pUsedHead;
	if (this->pUsedHead != nullptr) {
		this->pUsedHead->pUsedPrev = toRemove;
	}
	this->pUsedHead = toRemove;

	this->mStats.currUsedMem += (toRemove->mBlockSize);
	this->mStats.currNumUsedBlocks++;
	if (this->mStats.currUsedMem > this->mStats.peakUsedMemory) {
		this->mStats.peakUsedMemory = this->mStats.currUsedMem;
	}
	if (this->mStats.currNumUsedBlocks > this->mStats.peakNumUsed) {
		this->mStats.peakNumUsed = this->mStats.currNumUsedBlocks;
	}

}

void Heap::merge(FreeHdr *toMerge)
{
	//first merge down
	FreeHdr* next = toMerge->getEnd();
	if ((int)BlockType::FREE == next->mBlockType) {
		FreeHdr* currentNextFit = this->pNextFit;
		this->removeFree(next);
		this->removeFree(toMerge);
		toMerge->mBlockSize += next->mBlockSize + sizeof(FreeHdr);
		this->addFree(toMerge);
		if (currentNextFit == toMerge) {
			this->pNextFit = toMerge;
		}
	}
	

	//then merge up

	//if (toMerge->mAboveBlockFree) {
	//	FreeHdr *above = (toMerge - 1); //secret pointer!!!
	//}

	//then call merge on the new header
}



// ---  End of File ---------------
