/** 
 * @include treebuffer-define-directives
 * @define STR_SIZE [ 300 ]
 **/

#include <iostream>
#include <vector>
#include <string.h>
#include <sstream>
#include <fstream>
#include "testmatrix.h"
#include "tree_buffer.hpp"

using namespace std;
using namespace treebuffer;
using namespace treebuffer::detail;

bool compare (const std::string& truth, tree_buffer<char>& tb);

int main (int argc, char* argv[]) {
	
	string truth;
	tree_buffer<char> tb;
	
	string s("This is a very long string.");
	int n = s.size();
	
	
	/**
	 * Step one: populate truth and tb with the same characters and test if they are equal.
	 */
	for (int i=0; i < STR_SIZE; i++) {
		truth += s[i % n];
	}
	for (int i=0; i < STR_SIZE; i++) {
		tb.append(&s[i % n], 1);
	}

	compare(truth,tb);
	fstream dot6a;
	dot6a.open("./test6a.dot", ios_base::out);
  tb.dot(dot6a);
	dot6a.close();
	
	
	/**
	 * Step two: perform a sequence of removes and test equality at each step.
	 */
	tb.remove(5,12);
	truth.erase(5,12-5);
	compare(truth,tb);


	/**
	 * Step three: perform a sequence of remove-insert steps at varying positions and lengths, comparing equality at each step.
	 */
	
	
	
	/**
	 * Step four: perform a few nonsense operations on tb to ensure that it either noops or throws appropriately.
	 */
	// nonsense case, but noopable
	tb.remove(1,1);
	truth.erase(1,0);
	compare(truth,tb);

	// nonsense case, borderline
	bool throw_exception_for_bad_remove = false;
	try {
		tb.remove(30,25); // this throws
		//truth.erase(30,-5); // std::string actually deletes everything forward from 30 here.
		compare(truth,tb);
	} catch (const std::exception& e) {
		throw_exception_for_bad_remove = true;
	} 
	test_assert(throw_exception_for_bad_remove == true);
	
	// nonsense case, should throw range error
	bool throw_exception_for_bad_insert = false;
	try {
		tb.insert(1000000,"This should throw",17);     
	} catch (const std::exception& e) {
		throw_exception_for_bad_insert = true;
	}
	test_assert(throw_exception_for_bad_insert == true);
	
}


bool compare (const std::string& truth, tree_buffer<char>& tb) {
	stringstream ss;
	ss << tb;
	bool b = ss.str() == truth;
	test_assert(ss.str() == truth);
	if (!b) {
		std::cout << ss.str() << std::endl;
		std::cout << "[" << ss.str().size() << "]--- compared to ---[" << truth.size() << "]" << std::endl;
		std::cout << truth << endl << flush;
	}
	return b;
}
