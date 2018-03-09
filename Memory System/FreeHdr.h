//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef FREEHDR_H
#define FREEHDR_H

#include "Types.h"
#include "BlockType.h"

class FreeHdr
{
public:
	FreeHdr     *pFreeNext;			// next free block
	FreeHdr     *pFreePrev;			// prev free block
	Type::U32   mBlockSize;			// size of block
	Type::U8	mBlockType;			// block type 
	Type::Bool	mAboveBlockFree;	// if(block is free) -> true or if(block is used) -> false
	Type::U8	pad1;				// future use
	Type::U8	pad2;				// future use

	FreeHdr();
	//big four implementation, don't need these
	FreeHdr(const FreeHdr&) = delete;
	FreeHdr& operator = (const FreeHdr&) = delete;
	~FreeHdr() = delete;

	FreeHdr* getEnd() const;
	
};

#endif 

// ---  End of File ---------------
