#pragma once

#include <cassert>

namespace util
{


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>& shmfixedpool<T, addr_traits>::init_or_attach (typename addr_traits::poolid_t poolid)
{
	// 
	
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>& shmfixedpool<T, addr_traits>::init (typename addr_traits::poolid_t poolid)
{
	// 
	
}


template<typename T, typename addr_traits>
shmfixedpool<T, addr_traits>& shmfixedpool<T, addr_traits>::attach (typename addr_traits::poolid_t poolid)
{
	// 

	
}


template<typename T, typename addr_traits>
void shmfixedpool<T, addr_traits>::detach (shmfixedpool<T, addr_traits>&)
{
	
	
}


template<typename T, typename addr_traits>
T* shmfixedpool<T,addr_traits>::allocate (std::size_t n)
{
	assert(n==1);
	
	//
	
	return nullptr;
}


template<typename T, typename addr_traits>
void shmfixedpool<T,addr_traits>::deallocate (T* ptr, size_t s) noexcept
{
	assert(addr_traits::regionid(ptr) == addr_traits::rid);
	assert(s == 1);
	int poolid = addr_traits::poolid(ptr);
	
	
	
	
}


}
