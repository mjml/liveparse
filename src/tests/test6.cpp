/** 
 * @include treebuffer-define-directives
 * @define STR_SIZE [ 1024, 5000 ]
 **/

#include <iostream>
#include <vector>
#include <string.h>

#include "tree_buffer.hpp"

using namespace std;
using namespace treebuffer;

int main (int argc, char* argv[]) try {

	std::string truth;
	tree_buffer<char> tb;

	std::string s("This is a very long string.");
	int n = s.size();
	
	// Create initial string for both structures
	for (int i=0; i < STR_SIZE; i++) {
		truth += s[i % n];
	}
	for (int i=0; i < STR_SIZE; i++) {
		tb.append(&s[i % n], 1);
	}

	cout << tb;

	return 0;
	
} catch (exception& e) {
	cerr << "err: " << e.what() << endl << flush;
}
