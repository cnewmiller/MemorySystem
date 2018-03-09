//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef MEM_H
#define MEM_H

#include "Heap.h"

class Mem
{
public:
	static const unsigned int HEAP_SIZE = (50 * 1024);


public:
	Mem();	
	~Mem();

	Heap *GetHeap();
	void Dump();

	// implement these functions
	void Free( void * const data );
	void *Malloc( const Type::U32 size );

	void Initialize( );


private:
	Heap	*pHeap;
	void	*pRawMem;

	void changeFreeHeaderToUsed(const FreeHdr * const) const;
	void createFreeBlockFromExtraSpace(unsigned int oldSize, const unsigned int size, FreeHdr * tracer);
	void notifyNextBlockDown(FreeHdr * tracer, bool newValue);

};

#endif 

// ---  End of File ---------------
