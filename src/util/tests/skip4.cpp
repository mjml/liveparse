/**
 * @include skiparraylist-defines
 **/

#include <cassert>
#include <iostream>
#include <sstream>
#include <string.h>
#include <testmatrix.h>

#define DEBUG_SKIPARRAYLIST
#include "util/skiparraylist.hpp"

std::string s("This test establishes a large array and performs patterned removes, then pseudorandomly inserts "
							"until the array is large enough again, and repeats the removes ops. "
							"It is intended to systematically test the entire removal code base and to include all edge cases "
							"associated with this operation.");


using namespace util;
using namespace util::detail;
using namespace std;

#define ARRAY_TARGET_SIZE 512

bool compare (const std::string& truth, skiparraylist<char>& array) {
	std::stringstream arraydata;
	arraydata << array;
	bool b = arraydata.str() == truth;
	test_assert(arraydata.str() == truth);
	if (!b) {
		std::cout << arraydata.str() << std::endl;
		std::cout << "[" << arraydata.str().size() << "]--- compared to ---[" << truth.size() << "]" << std::endl;
		std::cout << truth << endl << flush;
	}
	return b;
}

int main (int argc, char* argv[])
{
	report_executable_parameters();
	int array_target_size = ARRAY_TARGET_SIZE;
	int n = s.size();
	
	util::skiparraylist<char> array;
	std::string truth;
	
	int x = 71;
	cout << "n=" << n << endl;
	for (int k=1; k <= array_target_size; k++) { // lengths to remove

		for (int i=0; i <= array_target_size-k; i++) { // position to remove at
			
			// First, perform inserts until we have a maximal length array.
			do {
				x = labs(x * x * k * (i + 1) + 1);
				int sz = truth.size();
				int len = std::min(x % 50 + 1, array_target_size - sz);
				int p;
				p = sz ? x % sz : 0;
				int q;
				q = x % (n-len);
				
				cout << "insert(" << p << "," << q << "," << len << ")  " << flush;
				truth.insert(p, &s[q], len);
				array.insert(p, &s[q], len);

				compare(truth,array);
				
			} while (array.size() < array_target_size);

			test_assert(array.size() == array_target_size);
			test_assert(truth.size() == array_target_size);
			
			// Second, perform a patterned remove
			cout << "remove(" << i << "," << k << ")  " << flush;
			truth.erase(i,k);
			array.remove(i,i+k);


			compare(truth, array);
			
		}
		
	}
	
	report_success();
	return 0;
}
