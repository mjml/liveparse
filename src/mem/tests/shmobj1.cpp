/**
 * @cxxparams "-g -I../.. -std=c++17"
 * @ldparams "-lpthread -lrt"
 **/

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <typeinfo>
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#include <iomanip>
#include "mem/addr_traits.hpp"
#include "mem/shmallocator.hpp"

using namespace mem;

std::string demangle (const char* name) {
	int status = -4;
	std::unique_ptr<char, void(*)(void*)> res {
																						 abi::__cxa_demangle(name,NULL,NULL,&status),
																						 std::free
	};
	return (status==0) ? res.get() : name;
}

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

template<>
FILE* mem::shmlog::logfile = nullptr;

template<>
FILE* log::logfile = nullptr;


int main ()
{
	using namespace std;

	A a1;
	A a2(a1);
	a2.x = 123;

	log::initialize();
	mem::shmlog::initialize();
	
	cout << "a1 is " << a1 << endl;
	cout << "a2 is " << a2 << endl;
	
	auto sa = new shmobj<A>(A());
	
	cout << "sa is " << std::get<A>(*sa) << endl;
	cout << "f(sa) = " << std::get<A>(*sa) << endl;
	
	void* ptr = (void*)0x100489abcdefL;
	
	cout << hex << ptr << endl;

	cout << hex << setfill('0');
	cout << "region_id: 0x" << std::setw(shglobal4::regionid_bits / 8) << (uint32_t)shglobal4::regionid(ptr) << endl;
	cout << "pool_id: 0x" << setw(shglobal4::poolid_bits / 8) << (uint32_t)shglobal4::poolid(ptr) << endl;
	cout << "segment_id: 0x" << setw(shglobal4::segmentid_bits / 8) << (uint32_t)shglobal4::segmentid(ptr) << endl;
	cout << "offset: 0x" << setw(shglobal4::offset_bits / 8) << (uint32_t)shglobal4::offset(ptr) << endl;
	cout << dec << setfill(' ');
	
	cout << "regionid_t is " << demangle(typeid(shglobal4::regionid(ptr)).name()) << endl;
	cout << "poolid_t is " << demangle(typeid(shglobal4::poolid(ptr)).name()) << endl;
	cout << "segmentid_t is " << demangle(typeid(shglobal4::segmentid(ptr)).name()) << endl;
	cout << "offset_t is " << demangle(typeid(shglobal4::offset(ptr)).name()) << endl;
	
	auto pool = shmfixedpool<A,shglobal4>::attach(0);



	shmfixedpool<A,shglobal4>::detach(pool);
	
	return 0;
}

