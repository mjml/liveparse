/**
 * @cxxparams "-g -I.."
 * @ldparams -g
 * @define MEMORY_NODE_CAPACITY 4096
 **/

#include <cassert>
#include <iostream>
#include <string.h>

#define DEBUG_TREEBUFFER

#include "tree_buffer.hpp"

namespace li = liveparse;
using namespace std;

int main (int argc, char* argv[])
{

	li::tree_buffer<char>::memory_node m8a;

	char text_buffer[] = "Test string.";
	
	
	auto it = m8a.at(0);
	m8a.insert(it, text_buffer, strlen(text_buffer));
	m8a.insert(it, text_buffer, strlen(text_buffer));
	m8a.insert(it, text_buffer, strlen(text_buffer));

	it = m8a.at(5);
	m8a.insert(it, text_buffer, strlen(text_buffer));

	cout << m8a << endl << flush;

	cout << "Success." << endl;
		
	return 0;
}
