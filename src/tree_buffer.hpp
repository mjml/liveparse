#pragma once

namespace liveparse
{

template <typename CharT>
class tree_buffer;

}

#include <alloca.h>
#include <type_traits>
#include <string>
#include <stdexcept>
#include <iostream>
#include <boost/intrusive/list.hpp>

using namespace std;

namespace liveparse
{

// Set to a page size
#ifndef MEMORY_NODE_CAPACITY
#define MEMORY_NODE_CAPACITY 4096
#endif

#ifndef MEMORY_NODE_NUMERATOR_GROWTH
#define MEMORY_NODE_NUMERATOR_GROWTH 4
#endif

#ifndef MEMORY_NODE_DENOMINATOR_GROWTH
#define MEMORY_NODE_DENOMINATOR_GROWTH 3
#endif

namespace bi = boost::intrusive;

template <typename CharT>
class tree_buffer
{
public:
	typedef CharT char_type;
	typedef int   offset_type;
	
	#ifdef DEBUG_BUFFER
public:
	#else
protected:
	#endif
	
	class node;
	class memory_node;
	class span_node;
	
	struct iterator {
		memory_node* mnode;
		offset_type offset; 
		
		bool operator== (const iterator& o) { return mnode == o.mnode && offset == o.offset; }
		iterator& operator++ ();
		iterator& operator-- ();
		iterator& operator+= (offset_type i);
		iterator& operator-= (offset_type i);
		const iterator* operator->() const { return this; }
		iterator operator->() { return this; }
		
	};
	
	
	/** 
	 * Basic node for the buffer-tree structure.
	 */
	class node
	{
	public:
		friend class tree_buffer<CharT>;
		typedef node node_t;
		
	#ifdef DEBUG_BUFFER
public:
	#else
protected:
	#endif
		node() : parent(nullptr), offset_start(0) {}
		virtual ~node() {  }
	
		virtual iterator at (int pos) = 0;
		virtual offset_type size() = 0;
		virtual std::ostream& printTo (std::ostream& os) = 0;
		virtual std::ostream& dot (std::ostream& os) = 0;
		span_node* parent;
		offset_type offset_start;
		bi::list_member_hook<> list_member;
		
		typedef bi::member_hook < node,
															bi::list_member_hook<>,
															&node::list_member >  list_member_type;
		
	};
	
	/**
	 * Indexing structure that contains other span nodes and memory nodes.
	 */
	class span_node : public node
	{
	public:
		typedef span_node span_t;
	
		span_node() : node(), offset_end() {}
		virtual ~span_node() { children.clear(); }
		
		typedef bi::list < node, typename node::list_member_type > child_list;
		child_list  children;
		offset_type offset_end;
		
		int get_num_children() { return children.size(); }
		iterator at (int pos);
		offset_type size() { return offset_end - node::offset_start; }
		void insert (const iterator& it, CharT* strdata, int length);
		void remove (iterator& from, iterator& to);
		std::ostream& printTo (std::ostream& os);
		std::ostream& dot (std::ostream& os);
	};
	
	struct memory_node_metadata_s {
		memory_node_metadata_s() : siz(0), next(nullptr) {}
		offset_type  siz;
		memory_node* next;
	};
	
	/**
	 * Refers to some contiguous range of memory.
	 */
	class memory_node : public node, public memory_node_metadata_s
	{
	public:
		typedef memory_node mem_t;
		friend class span_node;
		
	public:
		memory_node () {}
		virtual ~memory_node () { }
		
		iterator at (int pos);
		offset_type size() { return this->siz; }
		void insert (const iterator& it, CharT* strdata, int length);
		void remove (iterator& from, iterator& to);
		std::ostream& printTo (std::ostream& os);
		std::ostream& dot (std::ostream& os);
		
#ifdef DEBUG_BUFFER
	public:
#else
	protected:
#endif
		
		static constexpr int metadata_size() {
			return sizeof(node) + sizeof(memory_node_metadata_s);
		}

		static const int capacity = (MEMORY_NODE_CAPACITY - metadata_size()) / sizeof(CharT);
		
		CharT        buf[capacity];
	
	};
	
public:
	// Temporary provisioned types to be replaced by formal iterator classes
	typedef iterator marker;
	
	tree_buffer() : root(nullptr) {}
	~tree_buffer() {
		if (root) delete root;
	}
	
	/*
	const_iterator cbegin () const;
	const_iterator cend () const;
	const_iterator cat (int pos) const;
	*/
	
	iterator begin ();
	iterator end ();
	iterator at (int pos);
	
	void insert (const iterator& it, CharT* strdata, int length);
	void remove (iterator& from, iterator& to);

	std::ostream& operator<< (std::ostream& os);
	std::ostream& dot (std::ostream& os);
	
#ifdef DEBUG_BUFFER
	public:
#else
	protected:
#endif
	node* root;

};


template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::at (int pos)
{
	return root->at(pos);
}

template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::span_node::at (int pos)
{
	typename span_node::child_list::iterator last;
	for (auto child = children.begin(); child != children.end(); child++) {
		if (child->offset_start <= pos) {
			last = child;
		} else {
			break;
		}
	}
	return last->at(pos - last->offset_start);
}

template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::memory_node::at (int pos)
{
	if (pos > this->siz) {
		throw std::range_error ("pos > siz");
	}
	return { this, pos };
}


template <typename CharT>
void tree_buffer<CharT>::insert (const iterator& at, CharT* strdata, int length)
{
	at->mnode->insert(at, strdata, length);

	// last step: adjust to possibility of new root
	while (root->parent != nullptr) {
		root = root->parent;
	}
	
	return *this;
}


template <typename CharT>
void tree_buffer<CharT>::span_node::insert (const iterator& at, CharT* strdata, int length)
{
	auto mnode = at->mnode;
	
	// easy case: mnode can handle the data, so insert it
	if (mnode->capacity >= mnode->siz + length) {
		mnode->insert(at, strdata, length);
		return;
	}
	
	// harder case: mnode takes as much data as it can, then we insert new mnodes after it
	offset_type fill = mnode->capacity - mnode->siz;
	mnode->insert(at, strdata, fill);
	
	offset_type extra = length - fill;
	const int capacity = memory_node::capacity;

	typename child_list::iterator child_it = child_list::s_iterator_to(*mnode);
	
	memory_node* last = nullptr;
	while (extra > 0) {
		memory_node* m = new memory_node();
		if (last) {
			last->next = m;
		}
		m->insert(iterator({mnode, 0}), strdata + length - extra, std::min(extra,capacity));
		children.insert(child_it, *mnode);
		child_it++;
		extra -= capacity;
		last = m;
	}

}

template <typename CharT>
void tree_buffer<CharT>::memory_node::insert (const iterator& at, CharT* strdata, int length)
{
	if (length + this->siz > capacity) {
		throw std::range_error("mnode.size + length > mnode.capacity");
	}
	int backsize = this->siz - at.offset;
	CharT* tmpbuf = (CharT*)alloca(backsize);
	std::copy(buf + at.offset, buf + this->siz, tmpbuf);
	std::copy(strdata, strdata + length, buf + at.offset);
	std::copy(tmpbuf, tmpbuf + length, buf + at.offset + length);
	this->siz += length;
	
	// update sibling extents
	auto ma = this->parent;
	if (ma) {
		auto iter = ma->children.iterator_to(*this);
		while (iter != ma->children.end()) {
			iter++;
			iter->offset_start += length;
		}
	}
	
	// update ancestor extents
	while (ma) {
		auto grandma = ma->parent;
		ma->offset_end += length;
		
		if (grandma != nullptr) {
			auto aunti = span_node::child_list::s_iterator_to(*ma);
			aunti++;
			while (aunti != grandma->children.end()) {
				aunti->offset_start += length;
				auto aunt = dynamic_cast<span_node*> (&(*aunti));
				aunt->offset_end += length;
				aunti++;
			}
		}
		ma = grandma;
	}
	
}

template <typename CharT>
void tree_buffer<CharT>::remove (iterator& from, iterator& to)
{

}

template <typename CharT>
void tree_buffer<CharT>::span_node::remove (iterator& from, iterator& to)
{
	
}

template <typename CharT>
void tree_buffer<CharT>::memory_node::remove (iterator& from, iterator& to)
{
	
}

std::ostream& operator<< (std::ostream& os, tree_buffer<char>& b)
{
	return os << b.root;
}


std::ostream& operator<< (std::ostream& os, tree_buffer<char>::node& n)
{
	return n.printTo(os);
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::dot (std::ostream& os)
{
	os << "digraph { " << endl;
	root->dot(os);
	os << "}" << endl;
	return os << dec;
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::span_node::dot (std::ostream& os)
{
	
	os << "span_node" << std::hex <<  ((unsigned long)(this) & 0xffff)  << "[";
	os << "shape=box, color=grey, ";
	os << "label=\"" << std::hex << ((unsigned long)(this) & 0xffff) << endl;
	os << "offset_start=" << std::dec << this->offset_start << ", ";
	os << "offset_end=" << std::dec << this->offset_end << "\"];" << endl;
	
	for (auto& n : children) {
		n.dot(os);
	}

	if (this->parent) {
		os << "span_node" << std::hex << ((unsigned long)(this) & 0xffff)
			 << " -> span_node" << std::hex << ((unsigned long)(this->parent) & 0xffff) << " ;" << endl;
	}
	return os << std::dec;
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::memory_node::dot (std::ostream& os)
{
	os << "memory_node" << std::hex << ((unsigned long)(this) & 0xffff) << "[";
	os << "shape=box, ";
	os << "label=" << "\"" << std::hex << ((unsigned long)(this) & 0xffff) << endl;
	os << "offset_start=" << std::dec << this->offset_start << endl;
	os << "size=" << std::dec << this->siz <<"\"];"<< endl;

	if (this->parent) {
		os << "memory_node" << std::hex << ((unsigned long)(this) & 0xffff)
			 << " -> span_node" << std::hex << ((unsigned long)(this->parent) & 0xffff) << " ;" << endl;
	}
	return os << std::dec;
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::span_node::printTo (std::ostream& os)
{	
	for (auto& it : this->children) {
		os << it;
	}
	return os;
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::memory_node::printTo (std::ostream& os)
{
	for (int i=0; i < this->siz; i++) {
		os << this->buf[i];
	}
	return os;
}


}
