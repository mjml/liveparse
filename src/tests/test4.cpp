/**
 * @include treebuffer-define-directives
 **/

/**
 * This whitebox test uses the insert methods called on individual memory_node objects.
 * It also contains some code for rendering pretty .dot files.
 * 
 * - memory_node::insert
 * - memory_node::insert_raw
 */

#include <cassert>
#include <iostream>
#include <fstream>
#include <string.h>

#define DEBUG_TREEBUFFER

#include "tree_buffer.hpp"

namespace li = liveparse;
using namespace std;

int main (int argc, char* argv[])
{

	li::tree_buffer<char>::memory_node m8b;
	li::tree_buffer<char>::memory_node m8c;
	li::tree_buffer<char>::memory_node m8d;
	li::tree_buffer<char>::span_node s8a;
	li::tree_buffer<char>::span_node s8a2;
	li::tree_buffer<char>::span_node s8a3;
	
	li::tree_buffer<char> buf;
	
	fstream dot4a;
	fstream dot4b;
	dot4a.open("./test4a.dot", ios_base::out);
	dot4b.open("./test4b.dot", ios_base::out);
	
	char text_buffer[] = "Test string.";
	char long_buffer[] = "(This is a particularly long test string). ";

	cout << "Memory node capacity is: " << li::tree_buffer<char>::memory_node::capacity << endl;
	
	(m8b.next = &m8c)->next = &m8d;
	
	s8a2.children.push_back(m8b);
	s8a2.children.push_back(m8c);
	s8a2.children.push_back(m8d);
	m8b.parent = m8c.parent = m8d.parent = &s8a2;
	
	s8a.children.push_back(s8a2);
	s8a.children.push_back(s8a3);
	s8a2.parent = s8a3.parent = &s8a;
	
	buf.root = &s8a;
	
	auto it = m8b.at(0);
	m8b.insert(it, text_buffer, strlen(text_buffer));
	
	it = m8c.at(0);
	m8c.insert(it, text_buffer, strlen(text_buffer));

	it = m8d.at(0);
	m8d.insert(it, text_buffer, strlen(text_buffer));
	
	cout << s8a << endl;
	cout << "Desired size: " << 3 * strlen(text_buffer) << endl;
	buf.dot(dot4a);
	dot4a.close();
	
	it = s8a.at(5);
	buf.insert(it, long_buffer, strlen(long_buffer));

	cout << s8a << endl;
	cout << "Desired size: " << 3 * strlen(text_buffer) + strlen(long_buffer) << endl;
	buf.dot(dot4b);

	s8a.children.clear();
	s8a2.children.clear();

	dot4b.close();
	
	cout << "Success." << endl;
	exit(0); // avoids memory deallocation errors due to explicit construction of nodes on stack
	return 0;
}
