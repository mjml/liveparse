/**
 * @include skiparraylist-defines
 **/

/**
 * This whitebox test uses the insert methods called on individual leaf objects.
 * It also contains some code for rendering pretty .dot files.
 * 
 * - leaf::insert
 * - leaf::insert_raw
 */

#include <cassert>
#include <iostream>
#include <fstream>
#include <string.h>
#include <testmatrix.h>

#define DEBUG_SKIPARRAYLIST

#include "skiparraylist.hpp"

using namespace std;
using namespace util;
using namespace util::detail;


int main (int argc, char* argv[])
{

	auto *m8b = new leaf<char>;
	auto *m8c = new leaf<char>;
	auto *m8d = new leaf<char>;
	auto *s8a = new inner<char>;
	auto *s8a2 = new inner<char>;
	auto *s8a3 = new inner<char>;
	
	skiparraylist<char> s;
	
	fstream dot4a;
	fstream dot4b;
	dot4a.open("./test4a.dot", ios_base::out);
	dot4b.open("./test4b.dot", ios_base::out);
	
	char text_buffer[] = "Test string.";
	char long_buffer[] = "(This is a particularly long test string). ";
	
	cout << "Memory node capacity is: " << leaf<char>::capacity << endl;
	
	(m8b->_next = m8c)->_next = m8d;
	
	s8a2->push_back(m8b);
	s8a2->push_back(m8c);
	s8a2->push_back(m8d);
	m8b->parent = m8c->parent = m8d->parent = s8a2;
	
	s8a->push_back(s8a2);
	s8a->push_back(s8a3);
	s8a2->parent = s8a3->parent = s8a;
	
	s.root = s8a;
	
	auto it = m8b->at(0);
	m8b->insert(it, text_buffer, strlen(text_buffer));
	test_assert(m8b->offset == 0);
	test_assert(m8b->size() == 12);
	
	it = m8c->at(0);
	m8c->insert(it, text_buffer, strlen(text_buffer));
	test_assert(m8c->offset == 12);
	test_assert(m8c->size() == 12);

	it = m8d->at(0);
	m8d->insert(it, text_buffer, strlen(text_buffer));
	test_assert(m8d->offset == 24);
	test_assert(m8d->size() == 12);

	test_assert(s8a2->offset == 0);
	test_assert(s8a2->size() == 36);
	test_assert(s8a3->offset == 36);
	test_assert(s8a3->size() == 0);
	test_assert(s8a->offset == 0);
	test_assert(s8a->size() == 36);
	
	cout << *s8a << endl;
	cout << "Desired size: " << 3 * strlen(text_buffer) << endl;
	s.dot(dot4a);
	dot4a.close();
	
	it = s8a->at(5);
	s.insert(it, long_buffer, strlen(long_buffer));

	test_assert(m8b->offset == 0);
	test_assert(m8b->_next != m8c);
	test_assert(m8c->offset == 55);
	test_assert(m8c->size() == 12);
	test_assert(m8d->offset == 67);
	test_assert(m8d->size() == 12);
	
	test_assert(s8a2->offset == 0);
	test_assert(s8a2->size() == 48);
	test_assert(s8a3->offset == 79);
	test_assert(s8a3->size() == 0);
	test_assert(s8a->offset == 0);
	test_assert(s8a->size() == 79);
	
	cout << *s8a << endl;
	cout << "Desired size: " << 3 * strlen(text_buffer) + strlen(long_buffer) << endl;
	s.dot(dot4b);
	
	s8a->clear_and_delete_children();
	s8a2->clear_and_delete_children();

	test_assert(s8a->size() == 79);
	
	dot4b.close();
	
	cout << "Success." << endl;
	
	return 0;
}
