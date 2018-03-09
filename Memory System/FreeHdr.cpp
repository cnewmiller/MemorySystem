//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#include <string.h>
#include <assert.h>

#include "FreeHdr.h"
#include "BlockType.h"

// add code here

// ---  End of File ---------------

FreeHdr::FreeHdr():
	pFreeNext(nullptr),
	pFreePrev(nullptr),
	mBlockSize(0),
	mBlockType((int)BlockType::FREE),
	mAboveBlockFree(false),
	pad1(0),
	pad2(0)
{
}

FreeHdr* FreeHdr::getEnd()
{
	FreeHdr* end = (FreeHdr*)((Type::U8*) this + mBlockSize + sizeof(FreeHdr));
	return end;
}
