/**
 * @include skiparraylist-defines
 **/

#include <cassert>
#include <iostream>
#include <string.h>


#define DEBUG_SKIPARRAYLIST
#include "util/skiparraylist.hpp"

using namespace util;
using namespace util::detail;
using namespace std;

int main (int argc, char* argv[])
{
	auto m8b = new leaf<char>;
	auto m8c = new leaf<char>;
	auto m8d = new leaf<char>;
	auto s8a = new inner<char>;
	auto s8a2 = new inner<char>;
	auto s8a3 = new inner<char>;
	
	char text_buffer[] = "Test string.";
		
	s8a2->push_back(m8b);
	s8a2->push_back(m8c);
	s8a2->push_back(m8d);
	m8b->parent = m8c->parent = m8d->parent = s8a2;

	m8b->_next = m8c;
	m8c->_next = m8d;
		
	s8a->push_back(s8a2);
	s8a->push_back(s8a3);
	s8a2->parent = s8a3->parent = s8a;

	auto it = m8b->at(0);
	m8b->insert(it, text_buffer, strlen(text_buffer));

	it = m8c->at(0);
	m8c->insert(it, text_buffer, strlen(text_buffer));

	it = m8d->at(0);
	m8d->insert(it, text_buffer, strlen(text_buffer));
		
	cout << "The buffer contains: " << endl;
	cout << *s8a << endl;
	
	cout << "s8a: " << "[ " << s8a->offset << ", " << s8a->offset + s8a->size() << " ]" << endl;
	cout << "s8a2: " << "[ " << s8a2->offset << ", " << s8a2->offset + s8a2->size() << " ]" << endl;
	cout << "s8a3: " << "[ " << s8a3->offset << ", " << s8a3->offset + s8a3->size() << " ]" << endl;
	
	it = m8b->at(5);
	s8a2->insert(it, text_buffer, strlen(text_buffer));

	cout << "After re-insert, the buffer contains: " << endl;
	cout << *s8a << endl;

	cout << "s8a: " << "[ " << s8a->offset << ", " << s8a->offset + s8a->size() << " ]" << endl;
	cout << "s8a2: " << "[ " << s8a2->offset << ", " << s8a2->offset + s8a2->size() << " ]" << endl;
	cout << "s8a3: " << "[ " << s8a3->offset << ", " << s8a3->offset + s8a3->size() << " ]" << endl;

	delete s8a;
		
	cout << "*** Success ***" << endl << flush;
		
	return 0;
}
