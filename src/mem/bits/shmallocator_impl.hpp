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
#include "util/log.hpp"
#include "util/errno_exception.hpp"

#ifndef LOGLEVEL_MEM
#define LOGLEVEL_MEM 3
#endif 


namespace mem
{

struct shmallocator_logger_traits
{
	constexpr static const char* name = "mem";
	constexpr static int logLevel = LOGLEVEL_MEM;
};



typedef Log<shmallocator_logger_traits> shmlog;


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>::shmfixedpool ()
	:hdr(nullptr),
	 pool(0),
	 fh(-1)
{
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>::shmfixedpool (shmfixedpool<T,addr_traits>&& moved)
	:hdr(moved.hdr),
	 pool(moved.pool),
	 fh(moved.fh)
{
	moved.hdr = nullptr;
	moved.pool = 0;
	moved.fh = -1;
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>::~shmfixedpool ()
{
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits> shmfixedpool<T, addr_traits>::attach (poolid_t poolid)
{
	self_t pool = self_t();
	pool.pool = poolid;

	uint64_t total_size = 0;
	void* base_addr = pool.base_address();
	std::string name = pool.shared_name();
	bool created = false;
  
	pool.fh = shm_open(name.c_str(), O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
  
	// fstat to find if the file is zero length (new)
	struct stat statbuf;
	fstat(pool.fh, &statbuf);
	
	if (statbuf.st_size == 0) {
		created = true;
		total_size = pool.initial_size();
		ftruncate(pool.fh, total_size); 
	} else {
		total_size = statbuf.st_size;
	}
	
	int flags =
		MAP_SHARED | // allow other processes
		MAP_FIXED; // use this address exactly
	
	void *result = mmap (base_addr, total_size, PROT_READ|PROT_WRITE,
											 flags, pool.fh, 0); // -1 fd and 0 file offset
	
	assert(base_addr == result);
	
	if (created) {
		pool.hdr = new (base_addr) header_t();
		pool.hdr->mut.lock();
		pool.hdr->capacity = total_size;
		pool.hdr->mut.unlock();
		std::cout << "Created." << std::endl;
	} else {
		pool.hdr = reinterpret_cast<header_t*>(base_addr);
		std::cout << "Attached." << std::endl;
	}
	
	return pool;
}


template<typename T, typename addr_traits>
void shmfixedpool<T, addr_traits>::detach (shmfixedpool<T, addr_traits>& pool)
{
	assert(pool.hdr != nullptr);
	bool last = false;

	pool.hdr->mut.lock();
	int16_t refcnt = --pool.hdr->refcnt;
	auto size = pool.hdr->capacity;
	if (refcnt == 0) {
		last = true;
	}
	pool.hdr->mut.unlock();

	if (munmap(pool.base_address(), size)) {
		throw errno_runtime_error;
	}
	
	int unlink_result = shm_unlink(pool.shared_name().c_str());
	if (unlink_result) {
		if (errno == EACCES) {
			// This can be OK - it would mean that another process could have mapped
			// the object since the time at which we did pool.hdr->unlock()
			shmlog::warning("A call to shm_unlink failed with EACCESS.");
		} else if (errno == ENOENT) {
			// This won't cause problems, but is still worth a warning.
			shmlog::warning("Tried to unlink a shared memory segment that doesn't exist.");
		} else {
			throw errno_runtime_error;
		}
	}
	
}


template<typename T, typename addr_traits>
T* shmfixedpool<T,addr_traits>::allocate (std::size_t n)
{
	assert(n==1);
	
	// look first in the free list
	if (this->hdr->fl.size() > 0) {
		free_object& fo = this->hdr->fl.pop_back();
		T* objbytes = reinterpret_cast<T*>(&fo);
		return objbytes;
	}
	
	// failing the free list, use uncommitted objects
	if (this->hdr->size < this->hdr->capacity) {
		
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
	ss << "/" << appName << "-pool-" << std::hex
		 << addr_traits::regionid_bits << "."
		 << addr_traits::rid << "."
		 << this->pool;
	return ss.str();
}

}


template<>
FILE* mem::shmlog::logfile;


