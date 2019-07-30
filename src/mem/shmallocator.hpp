#pragma once

namespace mem
{
struct free_meta;
template<typename T, typename addr_traits> class shmfixedsegment;
template<typename T, typename addr_traits> class shmfixedpool;
template<typename T, typename addr_traits> class shmallocator;
}

#include <memory>
#include <cassert>
#include <stdio.h>
#include <shared_mutex>
#include "addr_traits.hpp"
#include <boost/intrusive/list.hpp>
#include <variant>

namespace mem {

namespace bi = boost::intrusive;

struct free_object
{
	typedef free_meta* pointer;
	// to be replaced by boost::intrusive possibly
	bi::list_member_hook<> memb;
};

typedef bi::list<free_object, bi::member_hook<free_object, bi::list_member_hook<>, &free_object::memb> > free_list;


template<typename T>
using shmobj = std::variant<free_object, T>;

template<typename T, typename addr_traits>
struct shmfixedsegment 
{
	struct shmfixedsegment_header
	{
		
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
	
	typedef struct header_s {
		uint64_t capacity;
		uint64_t size;
		int16_t refcnt;
		free_list fl;
		std::shared_mutex mut;
		
    header_s () = default;
		~header_s () = default;
		
	} header_t;
	
	constexpr static uint64_t header_size() {
		return sizeof(header_t);
	}
	
	constexpr static uint64_t initial_size() {
		return addr_traits::segment_size * 2;
	}

	T* next_uncommitted () {
		return reinterpret_cast<T*>(start_address() + this->size);
	}
	
	static self_t attach (poolid_t poolid);
	static void detach (self_t& pool);
	
	shmfixedpool ();
	shmfixedpool (const shmfixedpool&) = delete;
	shmfixedpool (shmfixedpool&& moved);
	~shmfixedpool ();
	
	shmfixedpool& operator= (const shmfixedpool<T,addr_traits>& other) = delete;
	
	std::string shared_name ();

	void swap (shmfixedpool<T,addr_traits>& other);
	
	void* base_address () {
		return addr_traits::base_address(pool);
	}
	
	void* start_address () {
		return ((header_size() / addr_traits::segment_size) + 1) * addr_traits::segment_size;
	}
	
	T* allocate(std::size_t n);
	
	void deallocate (T* p, std::size_t) noexcept;

  header_t* hdr;
	poolid_t pool;
	int fh;
	
};


} // namespace mem


#include "bits/shmallocator_impl.hpp"
