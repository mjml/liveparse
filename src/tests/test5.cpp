/**
 * @cxxparams "-g -I.."
 * @ldparams -g
 * @define MEMORY_NODE_SIZE 4096
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
	li::tree_buffer<char> tree;
	auto *s = new li::tree_buffer<char>::span_node;
	auto *m = new li::tree_buffer<char>::memory_node;

	tree.root = s;
	s->children.push_back(*m);
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

	cout << "Success." << endl;
		
	return 0;
}
