/**
 * @cxxparams "-g -I../.. -std=c++17"
 * @ldparams -g
 * @define MEMORY_NODE_SIZE 4096
 **/

#include <cassert>
#include <iostream>
#include <string.h>
#include <testmatrix.h>

#define DEBUG_SKIPARRAYLIST

#include "util/skiparraylist.hpp"

using namespace std;
using namespace util;
using namespace util::detail;

int main (int argc, char* argv[])
{
	report_executable_parameters();
	
	skiparraylist<char> tree;
	auto *s = new inner<char>;
	auto *m = new leaf<char>;
	
	tree.root = s;
	s->push_back(m);
	m->parent = s;
	
	char text_buffer[] = "Test string.";
	
	auto it = m->at(0);
	m->insert(it, text_buffer, strlen(text_buffer));
	m->insert(it, text_buffer, strlen(text_buffer));
	m->insert(it, text_buffer, strlen(text_buffer));

	it = m->at(5);
	m->insert(it, text_buffer, strlen(text_buffer));

	m->printTo(cout);
	cout << endl << flush;

	report_success();
	
	return 0;
}
