#pragma once

#include <string>
#include <cassert>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>
#include <shared_mutex>

extern std::string appName;

namespace util
{

template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>::shmfixedpool<T, addr_traits> ()
	:hdr(nullptr),
	 pool(0),
	 fh(-1)
{
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>::shmfixedpool<T, addr_traits> (shmfixedpool<T,addr_traits>&& moved)
	:hdr(moved.hdr),
	 pool(moved.pool),
	 fh(moved.fh)
{
	moved.hdr = nullptr;
	moved.pool = 0;
	moved.fh = -1;
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>::~shmfixedpool<T, addr_traits> ()
{
	if (moved.fh == -1) return;
	assert(hdr);
	hdr->mut.lock();
	size_t size = hdr->capacity;
	uint16_t refcnt = --hdr->refcnt;
	hdr->mut.unlock();
	void* base_addr = base_address();
	if (munmap(base_addr, size)) {
		throw errno_runtime_error;
	}
	
	if (refcnt == 0) {
		if (shm_unlink(shared_name().c_str())) {
			throw errno_runtime_error;
		}
	}
	
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits> shmfixedpool<T, addr_traits>::init_or_attach (poolid_t poolid)
{
	self_t pool = { .hdr=nullptr, .pool=poolid, .fh=0 };
	
	uint64_t hdrsize = hdrsegs() * addr_traits::segment_size;
	void* base_addr = pool.base_address();
	std::string name = pool.shared_name();
	bool created = false;
  
	pool.fh = shm_open(name.c_str(), O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
  
	// fstat to find if the file is zero length (new)
	struct stat statbuf;
	fstat(pool.fh, &statbuf);
	
	if (statbuf.st_size == 0) {
		created = true;
		ftruncate(pool.fh, hdrsize); 
	}
	
	int flags =
		MAP_SHARED | // allow other processes
		MAP_FIXED; // use this address exactly
	
	void *result = mmap (base_addr, hdrsize, PROT_READ|PROT_WRITE,
											 flags, pool.fh, 0); // -1 fd and 0 file offset
	
	assert(base_addr == result);
	
	if (created) {
		pool.hdr = new (base_addr) header_t();
		pool.hdr->capacity = hdrsize;
	} else {
		pool.hdr = reinterpret_cast<header_t*>(base_addr);
	}
	
	return pool;
}


template<typename T, typename addr_traits>
T* shmfixedpool<T,addr_traits>::allocate (std::size_t n)
{
	assert(n==1);
	
	// look first in the free list
	if (this->fl.size() > 0) {
		free_object& fo = fl.pop_back();
		T* objbytes = reinterpret_cast<T*>(&fo);
		return objbytes;
	}
	
	// failing the free list, use uncommitted objects
	if () {

	}

	// failing uncommitted objects, allocate a new segment

	// return the object
	
	return nullptr;
}


template<typename T, typename addr_traits>
void shmfixedpool<T,addr_traits>::deallocate (T* ptr, size_t s) noexcept
{
	assert(addr_traits::regionid(ptr) == addr_traits::rid);
	assert(s == 1);
	int poolid = addr_traits::poolid(ptr);
	
	
	
}


template<typename T, typename addr_traits>
std::string shmfixedpool<T,addr_traits>::shared_name ()
{
	//std::string result(0,255);
	//snprintf(&result[0], 255, "/%s/pool-%x.%lx.%x.shm", appName.c_str(), addr_traits::regionid_bits, addr_traits::rid, this->pool);
	std::stringstream ss;
	ss << "/" << appName.c_str() << "-pool-" << std::hex
		 << addr_traits::regionid_bits << "."
		 << addr_traits::rid << "."
		 << this->pool;
	return ss.str();
}

}
