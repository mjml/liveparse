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
#include "testmatrix.h"

#define DEBUG_TREEBUFFER

#include "tree_buffer.hpp"

using namespace std;
using namespace treebuffer;
using namespace treebuffer::detail;


int main (int argc, char* argv[])
{

	auto *m8b = new memory_node<char>;
	auto *m8c = new memory_node<char>;
	auto *m8d = new memory_node<char>;
	auto *s8a = new span_node<char>;
	auto *s8a2 = new span_node<char>;
	auto *s8a3 = new span_node<char>;
	
	tree_buffer<char> buf;
	
	fstream dot4a;
	fstream dot4b;
	dot4a.open("./test4a.dot", ios_base::out);
	dot4b.open("./test4b.dot", ios_base::out);
	
	char text_buffer[] = "Test string.";
	char long_buffer[] = "(This is a particularly long test string). ";
	
	cout << "Memory node capacity is: " << memory_node<char>::capacity << endl;
	
	(m8b->next = m8c)->next = m8d;
	
	s8a2->children.push_back(*m8b);
	s8a2->children.push_back(*m8c);
	s8a2->children.push_back(*m8d);
	m8b->parent = m8c->parent = m8d->parent = s8a2;
	
	s8a->children.push_back(*s8a2);
	s8a->children.push_back(*s8a3);
	s8a2->parent = s8a3->parent = s8a;
	
	buf.root = s8a;
	
	auto it = m8b->at(0);
	m8b->insert(it, text_buffer, strlen(text_buffer));
	test_assert(m8b->offset_start == 0);
	test_assert(m8b->size() == 12);
	
	it = m8c->at(0);
	m8c->insert(it, text_buffer, strlen(text_buffer));
	test_assert(m8c->offset_start == 12);
	test_assert(m8c->size() == 12);

	it = m8d->at(0);
	m8d->insert(it, text_buffer, strlen(text_buffer));
	test_assert(m8d->offset_start == 24);
	test_assert(m8d->size() == 12);

	test_assert(s8a2->offset_start == 0);
	test_assert(s8a2->size() == 36);
	test_assert(s8a3->offset_start == 36);
	test_assert(s8a3->size() == 0);
	test_assert(s8a->offset_start == 0);
	test_assert(s8a->size() == 36);
	
	cout << *s8a << endl;
	cout << "Desired size: " << 3 * strlen(text_buffer) << endl;
	buf.dot(dot4a);
	dot4a.close();
	
	it = s8a->at(5);
	buf.insert(it, long_buffer, strlen(long_buffer));

	test_assert(m8b->offset_start == 0);
	test_assert(m8b->next != m8c);
	test_assert(m8c->offset_start == 55);
	test_assert(m8c->size() == 12);
	test_assert(m8d->offset_start == 67);
	test_assert(m8d->size() == 12);
	
	test_assert(s8a2->offset_start == 0);
	test_assert(s8a2->size() == 48);
	test_assert(s8a3->offset_start == 79);
	test_assert(s8a3->size() == 0);
	test_assert(s8a->offset_start == 0);
	test_assert(s8a->size() == 79);
	
	cout << *s8a << endl;
	cout << "Desired size: " << 3 * strlen(text_buffer) + strlen(long_buffer) << endl;
	buf.dot(dot4b);
	
	s8a->children.clear();
	s8a2->children.clear();

	test_assert(s8a->size() == 79);
	
	dot4b.close();
	
	cout << "Success." << endl;
	
	return 0;
}
