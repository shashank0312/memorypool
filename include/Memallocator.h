#ifndef OFSallocator_h
#define OFSallocator_h

#include <vector>
#include <map>

namespace MemPool
{
	namespace Private
	{
		//////////////////////////////////////////////////////////////////////////////////////
		/////
		///  Manages a dynamically allocated, fixed size slab of memory. Provides an interface
		///  to allocate and deallocate fixed sized slots in this array. Once the slab is full
		///  the allocation function will start returning errors to the allocation requests.
		///
		///////////////////////////////////////////////////////////////////////////////////////
		class Slab
		{
		public:
			Slab (std::size_t iNumSlots);

			// Default copyconstructor
			// Default assignment operator
			// Default destructor

			void initialize(std::size_t iSlotSize);

			void destroy();

			void *allocate(std::size_t iSlotSize);

			bool deallocate(void *pv, std::size_t iSlotSize);

			void * initialized() const { return mpcMemoryPool; }

			bool empty()  const { return size() == 0; }

			std::size_t  size()  const { return miNumUsed; }

			bool full() const { return miNextFree == miNumSlots?true:false; }

			std::size_t capacity() const { return miNumSlots; }

		private:
			char *mpcMemoryPool;        /// The array of slots
			std::size_t  *maiFreeList;  /// Array based linked list of free slots.
			long miNextFree;			/// Head of the free list.
			std::size_t miNumSlots;      /// Number of slots.
			std::size_t miNumUsed;       /// Number of used slots.
		};

		///////////////////////////////////////////////////////////////////////////////////////////
		///////
		////
		///  Maintains a variable sized set of Slab objects. This allows us to allocate an unlimited
		///  number of slots, using slabs to fulfil the allocation requests.
		///
		///////////////////////////////////////////////////////////////////////////////////////////
		////
		class Pool
		{
		public:
			Pool(std::size_t iNumSlots, std::size_t iSlotSize);

			// To Check:
			// Default constructor required for map's operator[]
			// which inserts a mapped value using default construtor.
			// This will not be used by the allocator, but added for compilation.
			Pool(){}

			~Pool();

			void *allocate(std::size_t iSlotSize);

			void deallocate (void *pv, std::size_t iSlotSize);

            /// Query the size of the slot this pool manages.
            /// return: Size of the slot managed by the pool.
			std::size_t slotSize() const { return miSlotSize ; }

            /// Query to see if the pool is currently in use.
			bool empty() const { return size() == 0 ; }

			std::size_t size() const;

			std::size_t capacity() const;

		private:
			//Garbage collection logic.
			void shrink();

			typedef std::vector<Slab> SlabTable;
			SlabTable maSlabs;
			long miLastAllocate;
			long miLastDeallocate;
			std::size_t miNumSlots;
			std::size_t miSlotSize;
		};
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	/////
	////
	/// It is pooled memory allocator interface. Allocator maintains a set of Pool of varying slot
	/// size  and forwards the allocation and deallocation requests to the relevant one.
	/// It cannot be copied as assignment operator and copy constructor is protected.
	/// To Do: Create a base class which prevents copying, and derive allocator from it.
	class Allocator
	{
		

	protected:
		Allocator(const Allocator &rhs);

		Allocator &operator=(const Allocator &rhs);

	public:
		static const std::size_t DEFAULT_NUM_SLOTS = 1024;
		Allocator( std::size_t iNumSlots = DEFAULT_NUM_SLOTS):miNumSlots(iNumSlots){}
		

		// Destruct an allocator.
		~Allocator(){}

		void *allocate(std::size_t iSlotSize);

		void deallocate (void *pv, std::size_t iSlotSize);

		static std::size_t adjust(std::size_t iSlotSize);

	private:
		typedef std::map<std::size_t, Private::Pool> PoolMap;

		PoolMap maPoolMap;

		std::size_t miNumSlots;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////
	///
	/// Base class for objects which will use the global allocator. The allocator is a singleton.
	/// There will be one global singleton allocator created for each distinct value of the
	/// template argument(iNumSlots) when considered across the entire program.
	///
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <std::size_t iNumSlots = Allocator::DEFAULT_NUM_SLOTS>
	class PooledObject
	{
	public:
		static void *operator new(std::size_t iSize)
		{
			return instance().allocate(iSize);
		}
		static void operator delete(void *pv, std::size_t  iSize)
		{
			instance().deallocate(pv, iSize);
		}

		static Allocator &instance();

		virtual ~PooledObject(){}
	};

	// To Do: Add synchronization support for threaded allocator.
	template <std::size_t iNumSlots>
	Allocator & PooledObject<iNumSlots>::instance()
	{
		static Allocator gAllocator(iNumSlots);
		return gAllocator;
	}
}
#endif