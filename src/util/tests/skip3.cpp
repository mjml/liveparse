/** 
 * @include skiparraylist-defines
 * @define STR_SIZE [ 50 ]
 **/

#include <iostream>
#include <algorithm>
#include <vector>
#include <string.h>
#include <sstream>
#include <fstream>
#include <testmatrix.h>
#include "util/skiparraylist.hpp"

using namespace std;
using namespace util;
using namespace util::detail;

bool compare (const std::string& truth, skiparraylist<char>& tb);

skiparraylist<char> tb;

void dot_it (skiparraylist<char>& mytb, std::string fn) {
	fstream dot6a;
	dot6a.open(fn.c_str(), ios_base::out);
  mytb.dot(dot6a);
	dot6a.close();	
}

void dot_it2 (const char* szfn) {
	std::string fn(szfn);
	fstream dot6a;
	dot6a.open(fn.c_str(), ios_base::out);
  ::tb.dot(dot6a);
	dot6a.close();	
}



int main (int argc, char* argv[]) {
	
	string truth;
	
	string s("This is a very long string that I made up to make these tests work. "
					 "The veracity of the implementation depends on a wide range of testing inputs. ");
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
	
	dot_it(tb, "test6a.dot");
	
	
	/**
	 * Step two: perform a sequence of removes and test equality at each step.
	 */
	tb.remove(5,12);
	truth.erase(5,12-5);
	compare(truth,tb);
	
	dot_it(tb, "test6b.dot");

	cout << "n=" << n << endl;
	
	/**
	 * Step three: perform a sequence of remove-insert steps at varying positions and lengths, comparing equality at each step.
	 */
	int x = 53;
	int len = 0;
	for (int k=500; k > 2; k--) {
		cout << "insert mode" << endl << flush;
		while (truth.size() < 1000) {
			x = labs(x * k + 1);
			len = (x % (n/2 - 1)) + 1;
			int p = x % truth.size();
			int q = labs(x * x) % (n-len);
			
			//                tgt pos     src pos       length
			cout << "insert(" << p << "," << q << "," << len << ")  " << flush;
			truth.insert(p,&s[q],len);
			tb.insert(p,&s[q],len);
			dot_it(tb, "test6c.dot");
			compare(truth,tb);
		}
		cout << "remove mode" << endl << flush;
	  while (truth.size() > 500) {
			x = labs(x * k + 1);
      len = (x % (n/2 - 1)) + 1;
			int p = x % (truth.size()-len);
		  
			//                tgt pos       length
			cout << "remove(" << p << "," << len << ")  " << flush;
			truth.erase(p,len);
		  tb.remove(p,p+len);
			dot_it(tb, "test6d.dot");
			compare(truth,tb);
		}
	}
	
	
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


bool compare (const std::string& truth, skiparraylist<char>& tb) {
	stringstream arraydata;
	arraydata << tb;
	bool b = arraydata.str() == truth;
	test_assert(arraydata.str() == truth);
	if (!b) {
		std::cout << arraydata.str() << std::endl;
		std::cout << "[" << arraydata.str().size() << "]--- compared to ---[" << truth.size() << "]" << std::endl;
		std::cout << truth << endl << flush;
	}
	return b;
}
