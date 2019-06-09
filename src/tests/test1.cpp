#include <cassert>
#include <iostream>
#include <string.h>

#define DEBUG_BUFFER

#include "buffer.hpp"

namespace li = liveparse;
using namespace std;

int main (int argc, char* argv[])
{

	li::buffer<char>::memory_node m8a;
	li::buffer<char16_t>::memory_node m16a;

	cout << "The sizeof of m8a is " << sizeof(m8a) << endl;
	cout << "The sizeof of m16a is " << sizeof(m16a) << endl;
	cout << "The capacity of m8a is " << m8a.capacity << endl;
	cout << "The capacity of m16a is " << m16a.capacity << endl;
	cout << flush;

	char text_buffer[] = "Test string.";
	

	auto it = m8a.at(0);
	m8a.insert(it, text_buffer, strlen(text_buffer));
	
	assert(m8a.capacity == m16a.capacity * 2);

	cout << "Success." << endl;
		
	return 0;
}
