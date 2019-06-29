/**
 * @include treebuffer-define-directives
 **/

#include <cassert>
#include <iostream>
#include <string.h>

#define DEBUG_TREEBUFFER

#include "tree_buffer.hpp"

using namespace treebuffer;
using namespace treebuffer::detail;
using namespace std;

int main (int argc, char* argv[])
{
	auto m8b = new memory_node<char>;
	auto m8c = new memory_node<char>;
	auto m8d = new memory_node<char>;
	auto s8a = new span_node<char>;
	auto s8a2 = new span_node<char>;
	auto s8a3 = new span_node<char>;
	
	try {

		char text_buffer[] = "Test string.";
		
		s8a2->children.push_back(*m8b);
		s8a2->children.push_back(*m8c);
		s8a2->children.push_back(*m8d);
		m8b->parent = m8c->parent = m8d->parent = s8a2;

		m8b->next = m8c;
		m8c->next = m8d;
		
		s8a->children.push_back(*s8a2);
		s8a->children.push_back(*s8a3);
		s8a2->parent = s8a3->parent = s8a;

		auto it = m8b->at(0);
		m8b->insert(it, text_buffer, strlen(text_buffer));

		it = m8c->at(0);
		m8c->insert(it, text_buffer, strlen(text_buffer));

		it = m8d->at(0);
		m8d->insert(it, text_buffer, strlen(text_buffer));
		
		cout << "The buffer contains: " << endl;
		cout << *s8a << endl;
	
		cout << "s8a: " << "[ " << s8a->offset_start << ", " << s8a->offset_end << " ]" << endl;
		cout << "s8a2: " << "[ " << s8a2->offset_start << ", " << s8a2->offset_end << " ]" << endl;
		cout << "s8a3: " << "[ " << s8a3->offset_start << ", " << s8a3->offset_end << " ]" << endl;
	
		it = m8b->at(5);
		s8a2->insert(it, text_buffer, strlen(text_buffer));

		cout << "After re-insert, the buffer contains: " << endl;
		cout << *s8a << endl;

		cout << "s8a: " << "[ " << s8a->offset_start << ", " << s8a->offset_end << " ]" << endl;
		cout << "s8a2: " << "[ " << s8a2->offset_start << ", " << s8a2->offset_end << " ]" << endl;
		cout << "s8a3: " << "[ " << s8a3->offset_start << ", " << s8a3->offset_end << " ]" << endl;

		delete s8a;
		
		cout << "*** Success ***" << endl << flush;

	} catch (const std::exception& e) {
		cout << "Got here." << e.what() <<  endl << flush;
		return 1;
	} catch (...) {
		cout << "Got here.2" << endl << flush;
		return 1;
	}
		
	return 0;
}
