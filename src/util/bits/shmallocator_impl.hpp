#pragma once

namespace util
{


template<typename T, typename addr_traits>
void shmfixedpool<T, addr_traits>::init_or_attach (int poolid)
{
	
	
}


template<typename T, typename addr_traits>
void shmfixedpool<T, addr_traits>::detach ()
{
	
	
}


template<typename T, typename addr_traits>
T* shmallocator<T,addr_traits>::allocate (std::size_t n)
{
	if (n != 1) {
		throw std::bad_alloc();
	}
}



}
