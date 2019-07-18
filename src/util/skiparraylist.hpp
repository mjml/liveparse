#pragma once


// Set to a page size
#ifndef LEAF_CAPACITY
#define LEAF_CAPACITY 4096
#endif

#ifndef NODE_FANOUT
#define NODE_FANOUT 3
#endif

#ifdef DEBUG_SKIPARRAYLIST
#define PROTECTED public
#else
#define PROTECTED protected
#endif

namespace util::detail {
template<typename T> class node;
template<typename T> class inner;
template<typename T> class leaf;
}

#include <boost/intrusive/list.hpp>

#include <string>
#include <algorithm>

namespace util
{

using namespace util::detail;

typedef int   offset_type;

template<typename T> struct iterator;
template<typename T> class skiparraylist;

template<typename T>
std::ostream& operator<< (std::ostream& os, skiparraylist<T>& b);

template <typename T>
class skiparraylist
{
public:
	typedef T char_type;

	friend std::ostream& operator<<<T>(std::ostream& os, skiparraylist<T>& b);
	
	skiparraylist();
	~skiparraylist();
	
	iterator<T> begin ();
	iterator<T> end ();
	iterator<T> at (int pos);
	int pos (iterator<T>& it) const;
	
	int size () const;
	void clear ();
	void insert (int pos, const T* strdata, int length);
	void insert (const iterator<T>& it, const T* strdata, int length);
	void append (const T* strdata, int length);
	void remove (int from, int to);
	void remove (iterator<T>& from, iterator<T>& to);
	
	std::ostream& dot (std::ostream& os) const;
	
	friend std::ostream& operator<<<> (std::ostream& os, skiparraylist<T>& skip);
	
PROTECTED:
	inner<T>* root;
		
};


template<typename T>
struct iterator {
public:
	leaf<T>* leaf = nullptr;
	offset_type offset = 0;
	bool valid = false;
	
	bool operator== (const iterator& o) { return leaf == o.leaf && offset == o.offset; }
	iterator& operator++ ();
	iterator& operator-- ();
	iterator& operator+= (offset_type i);
	iterator& operator-= (offset_type i);
	T& operator* () const { return leaf->buf[offset]; }
	
	static bool is_end (const iterator& it) { return it.leaf==nullptr && it.valid; }
	static iterator end () { return iterator {nullptr,0,true}; }
};




} // namespace util

#include "skiparraylist_detail.hpp"

#include "skiparraylist_impl.hpp"
#include "skiparraylist_text.hpp"

