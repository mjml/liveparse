#pragma once
#include <type_traits>

namespace util
{

struct free_meta;
template<typename T> class shmobj;

struct shmfixedsegment_header;
template<typename T, int Size> class shmfixedsegment;

struct shmfixedpool_header;
template<typename T, int Size> class shmfixedpool;

template<typename T> class shmallocator;

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

template<typename T, int Size>
class shmfixedsegment 
{
public:
	shmfixedsegment() : hdr(usable_space(), sizeof(T)) {
		std::cout << "Creating segment for " << hdr.num_free << " objects of size " << capacity() << std::endl;
	}

	constexpr static int capacity() { return (Size - sizeof(shmfixedsegment_header)) / sizeof(T); }
	constexpr static int usable_space() { return Size - sizeof(shmfixedsegment_header); }
	
protected:
	void init_objects();
	
	shmfixedsegment_header hdr;
	char data [usable_space()];
	
};


struct shmfixedpool_header
{
 	int num_free;
	void* head;
	void* tail;
	free_meta* head_free;
	free_meta* tail_free;
};


template<typename T, int Size>
class shmfixedpool
{
public:
	shmfixedpool();

private:
	shmfixedpool_header hdr;
};


}

#include "util/shmobj_impl.hpp"
