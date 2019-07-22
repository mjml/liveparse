/**
 * @cxxparams "-g -I../.. -std=c++17"
 **/

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include "util/addr_traits.hpp"
#include "util/shmallocator.hpp"

using namespace util;

class A
{
public:
	A() : fav(11), arr("abc"), x(5), y(0.1) {}
	A(const A& other) : fav(other.fav), x(other.x), y(other.y)
	{
	 std::copy(other.arr, other.arr+43, arr);
	}
	~A() = default;

	A& operator= (const A& other) {
		fav = other.fav;
	  x = other.x;
		y = other.y;
		std::copy(other.arr, other.arr+43, arr);
		return *this;
	}


	friend std::ostream& operator<< (std::ostream& os, const A& a);
	
public:
	int fav;
	char arr[7];
	uint64_t x;
	float y;
};

std::ostream& operator<< (std::ostream& os, const A& a) {
	return os << "{ fav = " << a.fav << ", arr = \"" << a.arr << "\", x=" << a.x << ", y=" << a.y << " }";
}

class B : public A
{
public:
	B() : A(), g(1002) { x = 25; }
	
private:
	int g;
};

int f(A& a) {
	return a.x;
}

int g(int b) {
	return b + 5;
}

typedef pool_addr_traits<0x1004,16,12,8,12> shglobal4;


int main ()
{
	A a1;
	A a2(a1);
	a2.x = 123;
	
	std::cout << "a1 is " << a1 << std::endl;
	std::cout << "a2 is " << a2 << std::endl;
	
	auto sa = new shmobj<B>();
	auto &ra = *sa;
	
	std::cout << "ra is " << (*ra) << std::endl;
	std::cout << "f(ra) = " << f(ra) << std::endl;
	
	shmobj<A> &a3 = reinterpret_cast<shmobj<A>&>(a2);
	
	std::cout << "f(a3) = " << f(a3) << std::endl;
	
	void* ptr = (void*)0x1004abcdefL;

	//auto& pool = shmfixedpool<A,shglobal4>::init_or_attach(0);
	
	std::cout << std::hex << ptr << std::endl;

	std::cout << "region_id: 0x" << std::hex << shglobal4::regionid(ptr) << std::endl;
	std::cout << "pool_id: 0x" << std::hex << shglobal4::poolid(ptr) << std::endl;
	std::cout << "segment_id: 0x" << std::hex << shglobal4::segmentid(ptr) << std::endl;
	std::cout << "offset: 0x" << std::hex << shglobal4::offset(ptr) << std::endl;
	std::cout << std::dec;
	
	std::cout << "regionid_t is " << typeid(shglobal4::regionid(ptr)).name() << std::endl;
	std::cout << "poolid_t is " << typeid(shglobal4::poolid(ptr)).name() << std::endl;
	std::cout << "segmentid_t is " << typeid(shglobal4::segmentid(ptr)).name() << std::endl;
	std::cout << "offset_t is " << typeid(shglobal4::offset(ptr)).name() << std::endl;

	return 0;
}

