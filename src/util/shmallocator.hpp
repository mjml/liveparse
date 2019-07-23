#pragma once

#include <memory>
#include <cassert>
#include <stdio.h>

namespace util
{
struct free_meta;
template<typename T> union shmobj;
template<typename T, typename addr_traits> class shmfixedsegment;
template<typename T, typename addr_traits> class shmfixedpool;
template<typename T, typename addr_traits> class shmallocator;
}

#include <shared_mutex>
#include "addr_traits.hpp"


namespace util {

struct free_meta
{
	typedef free_meta* pointer;
	// to be replaced by boost::intrusive possibly
	pointer next;
	pointer prev;
};


template<typename T>
union shmobj
{
	
	shmobj() : obj() {}

	template<typename U>
	shmobj(U& other) : obj(other) {}

	T& operator* () noexcept {
		return obj;
	}
	
	template<typename U>
	operator U () noexcept {
		return (T&)(obj);
	}
	
	template<typename U>
	operator U& () noexcept {
		return (T&)(obj);
	}
	
	T obj;
	free_meta meta;
	
};


template<typename T, typename addr_traits>
struct shmfixedsegment 
{
	struct shmfixedsegment_header
	{
		int num_free;
		shmfixedsegment_header* next;
		shmfixedsegment_header* prev;
		void* head;
		void* tail;
		
		shmfixedsegment_header (int space, int fixedsize) :
			num_free(space/fixedsize), head(nullptr), tail(nullptr), next(nullptr), prev(nullptr)	{}
		
	};
	
	shmfixedsegment() : hdr(usable_space(), sizeof(T)) { }
	
	constexpr static int usable_space() { return addr_traits::page_size - sizeof(shmfixedsegment_header); }
	constexpr static int capacity()     { return usable_space() / sizeof(T); }
	
protected:
	void init_objects();
	
	shmfixedsegment_header hdr;
	uint8_t data [usable_space()];
	
};


template<typename T, typename addr_traits>
struct shmfixedpool
{
	typedef shmfixedpool<T,addr_traits> self_t;
	typedef shmfixedsegment<T,addr_traits> segment_t;

	using poolid_t = typename addr_traits::poolid_t;
	
	constexpr static uint64_t segtable_size = addr_traits::segmentid_space >> 3;
	
	typedef struct header_s {
		int ref_cnt = 0;
		int num_free = 0;
		int size = 0;
		free_meta* freehead = nullptr;
		free_meta* freetail = nullptr;
		std::shared_mutex mut;
		uint8_t segbits[segtable_size];
		
    header_s () = default;
		~header_s () = default;
		
	} header_t;
	
	constexpr static uint64_t hdrsegs() {
		return (sizeof(header_t) / addr_traits::segment_size) + 1;
	}
	
	static self_t init_or_attach (poolid_t poolid);
	static self_t init (poolid_t poolid);
	static self_t attach (poolid_t poolid);
	
	static void detach (self_t pool);
	
	T* allocate(std::size_t n);
	
	void deallocate (T* p, std::size_t) noexcept;

	void* base_address () {
		return (void*)(addr_traits::region_address() + (pool << (addr_traits::segmentid_bits + addr_traits::offset_bits)));
	}
	
	void* start_address () {
		return (T*)(uint64_t)(base_address() + addr_traits::segment_size * hdrsegs());
	}
	
	segment_t* find_segment ();

	segment_t* alloc_segment ();

	void free_segment (segment_t* seg);

	std::string shared_name ();

  header_t* hdr;
	poolid_t pool;
	int fh;
	
};


}


#include "bits/shmallocator_impl.hpp"
