//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef USEDHDR_H
#define USEDHDR_H

#include "Types.h"
#include "BlockType.h"

class UsedHdr
{
public:

	UsedHdr		*pUsedNext;		// next used block
	UsedHdr		*pUsedPrev;		// prev used block
	Type::U32   mBlockSize;		// size of block
	Type::U8	mBlockType;		// block type 
	Type::Bool	mAboveBlockFree;	// if(block is free) -> true or if(block is used) -> false
	Type::U8	pad0;           // future use
	Type::U8	pad1;			// future use


	//big four
	UsedHdr() = delete;
	UsedHdr(const UsedHdr&) = delete;
	UsedHdr& operator = (const UsedHdr&) = delete;
	~UsedHdr() = delete;

	
};

#endif 

// ---  End of File ---------------
