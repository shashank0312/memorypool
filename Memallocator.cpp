// MemPoolallocator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MemPoolallocator.h"
#include <iostream>
#include <algorithm>
#include <conio.h>


/// //////////////////////////////////////////////////////////////////
///Slab Constructor
//////////////////////////////////////////////////////////////////
MemPool::Private::Slab::Slab(std::size_t iNumSlots):miNumSlots(iNumSlots){}


////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: initialize
// Initializes the Slab internal memory pool.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    iSlotSize: Size of each slot in the pool.
//  OUT
//    None
//
//  RETURN
//    None.
//
////////////////////////////////////////////////////////////////////////

void MemPool::Private::Slab::initialize(std::size_t iSlotSize)
{
	try
	{
		mpcMemoryPool = new char[miNumSlots * iSlotSize];
        memset(mpcMemoryPool, 0, iSlotSize * miNumSlots);

		maiFreeList = reinterpret_cast<std::size_t *>(mpcMemoryPool);
	}
	catch (std::bad_alloc &exep)
	{
		delete [] mpcMemoryPool;
		mpcMemoryPool = NULL;
		//delete [] maiFreeList;
		throw;
	}

	// Set the free list.
	for ( std::size_t index = 0; index <= miNumSlots - 1; index++ )
	{
		maiFreeList[index * (iSlotSize/sizeof(std::size_t))] = index + 1;
	}

	//maiFreeList[miNumSlots - 1] = -1;
	miNumUsed = 0;
	miNextFree = 0;
}


////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: allocate
// Request a block of memory from the Slab.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    iSlotSize: Size of the slot to allocate
//  OUT
//    None
//
//  RETURN
//    Memory from the slab or NULL.
//
////////////////////////////////////////////////////////////////////////

//
void *MemPool::Private::Slab::allocate(std::size_t iSlotSize)
{
	// Return NULL so that higher level code may decide
	// the next action.
	if ( miNumUsed == miNumSlots )
		return NULL;

	miNumUsed++;

	int iPreviousFreeSlotNum = miNextFree;

	miNextFree = maiFreeList[iPreviousFreeSlotNum * (iSlotSize/sizeof(std::size_t))];

	//miNextFree = maiFreeList[iPreviousFreeSlotNum];

	// If slab is full, set miNextFree as the end of list.
	if ( miNumUsed == miNumSlots )
		miNextFree = miNumSlots;

	return static_cast<void *>(mpcMemoryPool + (iPreviousFreeSlotNum) * iSlotSize);
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: deallocate
// Returns a block of allocated memory to the Slab.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    pv       : Pointer to slot to deallocate.
//    iSlotSize: Size of the slot to allocate
//  OUT
//    None
//
//  RETURN
//    true if the allocated memory is in the slab , false otherwise.
//
////////////////////////////////////////////////////////////////////////

bool MemPool::Private::Slab::deallocate(void *pv, std::size_t iSlotSize)
{
	unsigned char *toRelease = reinterpret_cast<unsigned char *>(pv);
	unsigned char *ptr = reinterpret_cast<unsigned char *>(mpcMemoryPool);
	// Find which Slot in the slab is getting freed.
	std::size_t iNumSlot = (toRelease - ptr)/iSlotSize;

	if ( pv < mpcMemoryPool ||  iNumSlot > miNumSlots )
		return false;

	miNumUsed--;

	
	// Changing the free slot.
	//If the slab was full before this allocation, set the list accordingly.
	if ( miNextFree == miNumSlots )
	{
		maiFreeList[iNumSlot * ((iSlotSize)/sizeof(std::size_t))] = miNextFree;
		miNextFree = iNumSlot;
	}
	// Add the free slot on the front of the list.
	else
	{
		maiFreeList[iNumSlot * ((iSlotSize)/sizeof(std::size_t))] = miNextFree;
		miNextFree = iNumSlot;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: destroy
// Frees the memory allocated in the slab.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    None
//
//  OUT
//    None
//
//  RETURN
//    void
//
////////////////////////////////////////////////////////////////////////

void MemPool::Private::Slab::destroy()
{
	delete [] mpcMemoryPool;
	mpcMemoryPool = NULL;
	maiFreeList = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: allocate
// Request a block of memory from the Slab.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    iSlotSize: Size of the slot to allocate
//  OUT
//    None
//
//  RETURN
//    Memory from the slab or NULL.
//
////////////////////////////////////////////////////////////////////////
void *MemPool::Private::Pool::allocate(std::size_t iSlotSize)
{
	void *pv = NULL;
	Slab *pSlab;

	// If no Slabs are allocated, push a new slab;
	if ( maSlabs.size() == 0  )
	{
		maSlabs.push_back(Slab(miNumSlots));

		Slab &slab = maSlabs.back();

		slab.initialize(iSlotSize);

		miLastAllocate = 0;

		pv = slab.allocate(iSlotSize);

	}
	//Find a free slab. The most likely location could be
	// the Slab from where the last allocation requested succeeded.

	else if ( miLastAllocate >=0  && !maSlabs.at(miLastAllocate).full())
	{
		pSlab = &maSlabs.at(miLastAllocate);
		pv = pSlab->allocate(iSlotSize);

	}
	else
	{
		SlabTable::iterator iter = maSlabs.begin();

		for (int index = 0;;++iter, index++)
		{
			if ( iter == maSlabs.end() )
			{
				// Get the number of slots in the previous slab
				pSlab = &maSlabs.back();

				miNumSlots = miNumSlots * 2;
				maSlabs.push_back(Slab(miNumSlots));

				pSlab = &maSlabs.back();

				pSlab->initialize(iSlotSize);

				pv = pSlab->allocate(iSlotSize);

				miLastAllocate = maSlabs.size() - 1;

				break;
			}
			else if (! iter->full() )
			{
				miLastAllocate = index;

				pv = iter->allocate(iSlotSize);
				break;
			}
		}
	}

    // If the allocation request was not succesful, throw bad_alloc.
	if ( pv == NULL )
	{
		throw std::bad_alloc();
	}
	return pv;
}

////////////////////////////////////////////////////////////////////////
// Pool class Constructor/Destructor Definitions
////////////////////////////////////////////////////////////////////////
MemPool::Private::Pool::Pool(std::size_t iNumSlots, std::size_t iSlotSize):miNumSlots(iNumSlots),
				   miSlotSize(iSlotSize),miLastAllocate(-1),miLastDeallocate(-1)
{

}

MemPool::Private::Pool::~Pool()
{
	// Call destroy for each slab
	SlabTable::iterator iter = maSlabs.begin();

	for ( ;iter != maSlabs.end(); ++iter)
	{
		iter->destroy();
	}
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: deallocate
// Returns a block of allocated memory to the Slab.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    pv       : Pointer to slot to deallocate.
//    iSlotSize: Size of the slot to allocate
//  OUT
//    None
//
//  RETURN
//    true if the allocated memory is in the slab , false otherwise.
//
////////////////////////////////////////////////////////////////////////
void MemPool::Private::Pool::deallocate(void *pv, std::size_t iSlotSize)
{
	bool bFound = true;
	int index = 0;

	// Check if the memory is getting deallocated from the last Slab
	// that was used in deallocation && proceed from it to start
	// or towards the end. It checks in the vicinity of the last
	// slab from where the deallocation happened

	int lo = miLastDeallocate;
	int hi = miLastDeallocate + 1;
	int lowbound = 0;
	int highbound = maSlabs.size();

	miLastDeallocate = -1;

	for ( ; ; )
	{
		if ( lo >= 0 )
		{
			bFound = maSlabs.at(lo).deallocate(pv, iSlotSize);
			if ( bFound )
			{
				miLastDeallocate = lo;
				break;
			}
			lo--;
		}
		if ( hi < highbound)
		{
			bFound = maSlabs.at(hi).deallocate(pv, iSlotSize);
			if ( bFound )
			{
				miLastDeallocate = hi;
				break;
			}
			hi++;
		}
	}

	//Garbage Collection logic.
	//Clean up the Slab if all the memory of the slab has been deallocated.
	if ( maSlabs.at(miLastDeallocate).empty() )
		shrink();
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: size
// Retrieve the total number of allocated objects in the pool.
// This is O(n) in complexity and should be used less.
// Trade off between pool size and speed of less used function.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//		void
//  OUT
//      	None
//
//  RETURN
//    Number of slots currently in used.
//
////////////////////////////////////////////////////////////////////////
std::size_t MemPool::Private::Pool::size() const
{
	int iSize = 0;

	SlabTable::const_iterator current, end;

	current = maSlabs.begin() + 1;

	end = maSlabs.end();

	for ( ; current != end ; ++current )
	{
		iSize += current->size();
	}

	return iSize;
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: capacity
// Retrieve the current capacity of the pool(total number of
// slots both used and unused).
// This is O(n) in complexity and should be used less.
// Trade off between pool size and speed of less used function like size().
//
// ARGUMENTS AND RETURN INFO:
//  IN
//		void
//  OUT
//      	None
//
//  RETURN
//    Total number of slots in the pool.
//
////////////////////////////////////////////////////////////////////////

std::size_t MemPool::Private::Pool::capacity() const
{
	int iCapacity = 0;

	Private::Pool::SlabTable::const_iterator current, end;

	current = maSlabs.begin();

	end = maSlabs.end();

	for ( ; current != end ; ++current )
	{
		iCapacity += current->capacity();
	}

	return  iCapacity;
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: shrink
//
// Does garbage collection on empty slabs.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//		void
//  OUT
//      	None
//
//  RETURN
//    Total number of slots in the pool.
//
////////////////////////////////////////////////////////////////////////

void MemPool::Private::Pool::shrink()
{
	/*if ( maSlabs.size() > 1 )
	{
		// erase Slabs whose slots are completely unused.
		// mem leak as romove will move pointers that will
		// cause the leaks.Need to call destroy of
		// Slab objects.
		// To Do: It is an O(n) time complex code as of now,
		//        to check if it could be optimized.
		//        Need to add an explicit loop for
		//        destroying the slab memory.
		maSlabs.erase( remove_if(maSlabs.begin() + 1, maSlabs.end(),
				mem_fun(&Private::Slab::empty)),  maSlabs.end());
	}*/

	// ToDo: Throw exception if the last deallocated slab is not set
	if ( miLastDeallocate == -1 )
	{
		return;
	}

	// If there is only one slab, keep the slab for further allocation request
	// Do garbage collection if there are more than one slabs.
	if ( maSlabs.size() == 1 )
		return;

	MemPool::Private::Slab &lastSlab =  maSlabs.back();

	int iLastIndex = maSlabs.size() - 1;

	
		return;

	// Check if this is the last slab and both the last and the second last slab is empty
	// If the current slab and the last slab are both empty, release the last slab
	// else if only the current slab is empty , swap it at the end.
	if (miLastDeallocate == iLastIndex )
	{
		miLastDeallocate = -1;
		return;
	}
	else if (   miLastDeallocate == iLastIndex - 1 )
	{
		if ( maSlabs[iLastIndex].empty() )
		{
			miNumSlots = lastSlab.capacity();

			lastSlab.destroy();
			
			maSlabs.pop_back();

			if ( miLastAllocate == iLastIndex )
				miLastAllocate = -1;

		}
		miLastDeallocate = -1;				
		return;

	}
	// If last slab is empty.
	else if ( lastSlab.empty() )
	{
		int lastIndx = maSlabs.size() - 1;

		miNumSlots = lastSlab.capacity();

		lastSlab.destroy();

		maSlabs.pop_back();

		lastSlab = maSlabs.back();

		std::swap(maSlabs[miLastDeallocate], lastSlab);

		if ( lastIndx == miLastAllocate )
			miLastAllocate = -1;
			
		// To DO:
		//miLastAllocate = maSlabs.size() - 1;
	}
	else
	{
		MemPool::Private::Slab &deallocateSlab = maSlabs.at(miLastDeallocate);
		std::swap(deallocateSlab, lastSlab);
	}
	miLastDeallocate = -1;
			
	return;
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: adjust
//
// Adjust the slot size in preparation for finding a pool for it.
// This allows object of similar size to share a pool, rather than having
// distinct pools.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//		iSlotSize Actual size of the slot requested.
//  OUT
//      	None
//
//  RETURN
//    Suggested size of the slot to use .
//
////////////////////////////////////////////////////////////////////////

std::size_t MemPool::Allocator::adjust(std::size_t iSlotSize)
{
	int iNewSlotSize = iSlotSize;

	// Align objects on multiples of 4 byte boundary
	// The memory allocator is not optimal for objects less than 4 bytes
	// (highly unlikely).
	if ( iSlotSize % 4 != 0 )
	{
		iNewSlotSize += (4 - iSlotSize%4);
	}

	return iNewSlotSize;
}

///////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: allocate
// Allocate a block of memory of size iSlotSize. The allocator will forward
// the request to the appropraite Pool, creating one if necessary.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    iSlotSize: Size of the slot to allocate
//  OUT
//    None
//
//  RETURN
//    Allocated memory.
//
////////////////////////////////////////////////////////////////////////
void * MemPool::Allocator::allocate(std::size_t iSlotSize)
{
	std::size_t iNewSlotSize  = adjust(iSlotSize);
	Private::Pool *pool;
	void *pv = NULL;

	// Check if a pool is there for the size requested.
	Allocator::PoolMap::iterator iter = maPoolMap.find(iNewSlotSize);

	// If the pool is present, use it.
	if ( iter != maPoolMap.end() )
	{
		pool = &iter->second;
	}
	// Create a new Pool
	else
	{
		iter = maPoolMap.insert(std::pair<std::size_t, Private::Pool>(iNewSlotSize,
			             MemPool::Private::Pool(miNumSlots, iNewSlotSize))).first;

		pool = &iter->second;
	}
	pv = pool->allocate(iNewSlotSize);

	return pv;
}

////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: deallocate
// Deallocate a block of memory.
//
// ARGUMENTS AND RETURN INFO:
//  IN
//    pv       : Pointer to slot to deallocate.
//    iSlotSize: Size of the slot to allocate
//  OUT
//    None
//
//  RETURN
//    true if the allocated memory is in the slab , false otherwise.
//
////////////////////////////////////////////////////////////////////////

void MemPool::Allocator::deallocate (void *pv, std::size_t iSlotSize)
{
	// Find the pool which has the given slot
	std::size_t iNewSlotSize  = adjust(iSlotSize);
	MemPool::Private::Pool *pool = NULL;

	Allocator::PoolMap::iterator iter = maPoolMap.find(iNewSlotSize);

	// throw exception if the deallocate is called with incorrect Slot size
	if ( iter == maPoolMap.end() )
	{
		// To do
		return;
	}
	else
	{
		pool = &iter->second;
		pool->deallocate(pv, iNewSlotSize);
	}
}



