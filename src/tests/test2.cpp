/**
 * @testtype: generator
 * @include treebuffer-define-directives
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

	li::tree_buffer<char>::memory_node m8b;
	li::tree_buffer<char>::memory_node m8c;
	li::tree_buffer<char>::memory_node m8d;
	li::tree_buffer<char>::span_node s8a;
	li::tree_buffer<char>::span_node s8a2;
	li::tree_buffer<char>::span_node s8a3;

	char text_buffer[] = "Test string.";
	

	
	s8a2.children.push_back(m8b);
	s8a2.children.push_back(m8c);
	s8a2.children.push_back(m8d);
	m8b.parent = m8c.parent = m8d.parent = &s8a2;

	s8a.children.push_back(s8a2);
	s8a.children.push_back(s8a3);
	s8a2.parent = s8a3.parent = &s8a;

	auto it = m8b.at(0);
	m8b.insert(it, text_buffer, strlen(text_buffer));

	it = m8c.at(0);
	m8c.insert(it, text_buffer, strlen(text_buffer));

	it = m8d.at(0);
	m8d.insert(it, text_buffer, strlen(text_buffer));
	
	cout << "The buffer contains: " << endl;
	cout << s8a << endl;
	
	cout << "s8a: " << "[ " << s8a.offset_start << ", " << s8a.offset_end << " ]" << endl;
	cout << "s8a2: " << "[ " << s8a2.offset_start << ", " << s8a2.offset_end << " ]" << endl;
	cout << "s8a3: " << "[ " << s8a3.offset_start << ", " << s8a3.offset_end << " ]" << endl;
	
	it = m8b.at(5);
	m8b.insert(it, text_buffer, strlen(text_buffer));

	cout << "After re-insert, the buffer contains: " << endl;
	cout << s8a << endl;

	cout << "s8a: " << "[ " << s8a.offset_start << ", " << s8a.offset_end << " ]" << endl;
	cout << "s8a2: " << "[ " << s8a2.offset_start << ", " << s8a2.offset_end << " ]" << endl;
	cout << "s8a3: " << "[ " << s8a3.offset_start << ", " << s8a3.offset_end << " ]" << endl;

	s8a.children.clear();
	s8a2.children.clear();
	
	cout << "Success." << endl;
		
	return 0;
}
