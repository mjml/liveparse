#pragma once


// Set to a page size
#ifndef MEMORY_NODE_CAPACITY
#define MEMORY_NODE_CAPACITY 4096
#endif

#ifndef SPAN_NODE_FANOUT
#define SPAN_NODE_FANOUT 3
#endif

#ifdef DEBUG_TREEBUFFER
#define PROTECTED public
#else
#define PROTECTED protected
#endif

namespace treebuffer::detail {
template<typename T> class span_node;
template<typename T> class memory_node;
}

#include <boost/intrusive/list.hpp>

#include <string>
#include <algorithm>

namespace treebuffer
{

using namespace treebuffer::detail;
namespace bi = boost::intrusive;

typedef int   offset_type;

template<typename T> struct iterator;
template<typename T> class tree_buffer;

template<typename T>
std::ostream& operator<< (std::ostream& os, tree_buffer<T>& b);

template <typename T>
class tree_buffer
{
public:
	typedef T char_type;

	friend std::ostream& operator<<<T>(std::ostream& os, tree_buffer<T>& b);
	
	tree_buffer();
	~tree_buffer();
	
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
	
	friend std::ostream& operator<<<> (std::ostream& os, tree_buffer<T>& tree);
	
PROTECTED:
	span_node<T>* root;
	
};


template<typename T>
struct iterator {
public:
	memory_node<T>* mnode = nullptr;
	offset_type offset = 0;
	bool valid = false;
	
	bool operator== (const iterator& o) { return mnode == o.mnode && offset == o.offset; }
	iterator& operator++ ();
	iterator& operator-- ();
	iterator& operator+= (offset_type i);
	iterator& operator-= (offset_type i);
	iterator* operator->() { return this; }
	const iterator* operator->() const { return this; }
	T& operator* () const { return mnode->buf[offset]; }
	
	static bool is_end (const iterator& it) { return it.mnode==nullptr && it.valid; }
	static iterator end () { return iterator {nullptr,0,true}; }
};




} // namespace treebuffer

#include "tree_buffer_detail.hpp"

#include "tree_buffer_impl.hpp"
#include "tree_buffer_text.hpp"

