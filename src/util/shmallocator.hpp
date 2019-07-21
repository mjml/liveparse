#pragma once

namespace util
{
struct free_meta;
template<typename T> class shmobj;
template<typename T, typename addr_traits> class shmfixedsegment;
template<typename T, typename addr_traits> class shmfixedpool;
template<typename T, typename addr_traits> class shmallocator;
}

#include <shared_mutex>
#include "addrscheme.hpp"


namespace util {

struct free_meta
{
	typedef free_meta* pointer;
	// to be replaced by boost::intrusive possibly
	pointer next;
	pointer prev;
};


template<typename T>
class shmobj : public free_meta
{
public:
	
	shmobj() : free_meta(), obj() {}

	template<typename U>
	shmobj(U& other) : obj(other) {}

public:

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
	
private:
	union {
		T obj;
		free_meta meta;
	};
	
	
};


template<typename T, typename addr_traits = pool_addr_traits<>>
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
	
	shmfixedsegment() : hdr(usable_space(), sizeof(T)) {
		std::cout << "Creating segment for " << &hdr << " objects of size " << capacity() << std::endl;
	}
	
	constexpr static int usable_space() { return addr_traits::page_size - sizeof(shmfixedsegment_header); }
	constexpr static int capacity()     { return usable_space() / sizeof(T); }
	
protected:
	void init_objects();
	
	shmfixedsegment_header hdr;
	char data [usable_space()];
	
};


template<typename T, typename addr_traits = pool_addr_traits<>>
struct shmfixedpool
{
	typedef shmfixedpool<T,addr_traits> self_t;
	typedef shmfixedsegment<T,addr_traits> segment_t;

	struct shmfixedpool_header {
		int num_free;
		segment_t* head;
		segment_t* tail;
		free_meta* head_free;
		free_meta* tail_free;
		std::shared_mutex mut;
	};
	
	shmfixedpool() = delete;
	~shmfixedpool() = delete;
	
	static void init_or_attach (int poolid);
	static void detach ();
	
	static shmfixedpool_header* hdr = nullptr;
};


template<typename T, typename addr_traits>
class shmallocator
{
	shmfixedpool<T,addr_traits>& pool;
	
public:
	typedef T value_type;
	
	shmallocator(shmfixedpool<T,addr_traits>& _pool) : pool(_pool) {}
	
	T* allocate(std::size_t n);
	
	void deallocate (T* p, std::size_t) noexcept;
	
};

}


#include "bits/shmallocator_impl.hpp"
