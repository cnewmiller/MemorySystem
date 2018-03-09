//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2017
// Optimized C++
//----------------------------------------------------------------------------- 

#include <string.h>
#include <assert.h>

#include "UsedHdr.h"

// Add code here

// ---  End of File ---------------

void * UsedHdr::getAbove()
{

	if (this->mAboveBlockFree) {
		UsedHdr* tracer = this - 1;
		return tracer;
	}
	return nullptr;
}
