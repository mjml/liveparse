/**
 * @cxxparams "-g -I../.. -std=c++17"
 **/

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include "util/shmobj.hpp"

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
	char arr[43];
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

	shmfixedsegment<A,4096> segment;
	
	return 0;
}

