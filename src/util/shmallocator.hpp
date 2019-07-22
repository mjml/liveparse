#pragma once

#include <memory>
#include <cassert>


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
	
	struct shmfixedpool_header {
		int ref_cnt;
		int num_free;
		int num_uncommitted;
		segment_t* seghead;
		segment_t* segtail;
		free_meta* freehead;
		free_meta* freetail;
		std::shared_mutex mut;
	};
	
	shmfixedpool() = delete;
	~shmfixedpool() = delete;

	static self_t& init_or_attach (typename addr_traits::poolid_t poolid);
	static self_t& init (typename addr_traits::poolid_t poolid);
	static self_t& attach (typename addr_traits::poolid_t poolid);
	
	static void detach (self_t& pool);
	
	static shmfixedpool_header* hdr;
	
	T* allocate(std::size_t n);
	
	void deallocate (T* p, std::size_t) noexcept;
	
};

template<typename T, typename addr_traits>
typename shmfixedpool<T,addr_traits>::shmfixedpool_header* shmfixedpool<T,addr_traits>::hdr = nullptr;

}


#include "bits/shmallocator_impl.hpp"
