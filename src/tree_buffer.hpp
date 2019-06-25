#pragma once

namespace liveparse
{

template <typename CharT>
class tree_buffer;

}

#include <alloca.h>
#include <type_traits>
#include <string>
#include <algorithm>
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

#ifndef SPAN_NODE_FANOUT
#define SPAN_NODE_FANOUT 3
#endif

#ifdef DEBUG_TREEBUFFER
#define PROTECTED public
#else
#define PROTECTED protected
#endif

namespace bi = boost::intrusive;

template <typename CharT>
class tree_buffer
{
public:
	typedef CharT char_type;
	typedef int   offset_type;

PROTECTED:
	
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

	PROTECTED:
		node() : parent(nullptr), offset_start(0) {}
		node(span_node* p) : parent(p), offset_start(0) {}
		virtual ~node() {  }
	
		virtual iterator at (int pos) = 0;
		virtual offset_type size() = 0;
		virtual void set_size (offset_type) = 0;
		virtual std::ostream& printTo (std::ostream& os) = 0;
		virtual std::ostream& dot (std::ostream& os) = 0;
		virtual void fixup_siblings () {
			if (parent == nullptr) return;
			int ofs = offset_start + size();
			for (auto &it = ++parent->children.iterator_to(*this); it != parent->children.end(); it++) {
				int saved_size = it->size();
				it->offset_start = ofs;
				it->set_size(saved_size);
				ofs += saved_size;
			}
		}
		
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
		typedef bi::list < node, typename node::list_member_type > child_list;
		
		span_node() : node(), offset_end() {}
		span_node(span_node* p) : node(p), offset_end() {}
		virtual ~span_node() { children.clear(); }
		
		int get_num_children() { return children.size(); }
		iterator at (int pos);
		offset_type size() { return offset_end - node::offset_start; }
		void set_size (offset_type ofs) { offset_end = node::offset_start + ofs; }
		void insert (const iterator& it, const CharT* strdata, int length);
		void remove (iterator& from, iterator& to);
		std::ostream& printTo (std::ostream& os);
		std::ostream& dot (std::ostream& os);

	PROTECTED:
		void rebalance ();

		child_list  children;
		offset_type offset_end;
		
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

		static constexpr int metadata_size() {
			return sizeof(node) + sizeof(memory_node_metadata_s);
		}
		static const int capacity = (MEMORY_NODE_CAPACITY - metadata_size()) / sizeof(CharT);
	  static_assert (capacity > 0, "Capacity is too small");
		
		
	public:
		memory_node () {}
		memory_node (span_node* p) : node(p) {}
		virtual ~memory_node () { }
		
		iterator at (int pos);
		offset_type size() { return this->siz; }
		void set_size (offset_type ofs) { this->siz = ofs; }
		void insert (const iterator& it, const CharT* strdata, int length);
		void remove (iterator& from, iterator& to);
		std::ostream& printTo (std::ostream& os);
		std::ostream& dot (std::ostream& os);

	PROTECTED:
		void raw_insert (const iterator& it, const CharT* strdata, int length, CharT* carry_data, int* carry_length);
		void raw_prepend (const CharT* strdata, int length) { raw_insert(iterator{this,0}, strdata, length, nullptr, nullptr); }
		void raw_append (const CharT* strdata, int length) { raw_insert(iterator{this,this->siz}, strdata, length, nullptr, nullptr); }
		
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

PROTECTED:
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
	if (memory_node::capacity >= at->mnode->siz + length) {
		at->mnode->insert(at, strdata, length);
	} else {
		at->mnode->parent->insert(at, strdata, length);
	}

	// last step: adjust to possibility of pivoted root
	while (root->parent != nullptr) {
		root = root->parent;
	}
	
}


template <typename CharT>
void tree_buffer<CharT>::span_node::insert (const iterator& at, const CharT* strdata, int length)
{
	const int capacity = memory_node::capacity;
	auto from = at->mnode;

	assert(from->parent == this);

	// FIXME: unresolved edge case where buffer is newly created and doesn't have memory_nodes or span_nodes yet

	// Determine how many characters we can fit into the memory_node pointed to by 'at'.
	int carry_length = from->siz - at->offset;
	CharT* carry_data = (CharT*) alloca(sizeof(CharT) * carry_length);
	int first_segment_length = capacity - at->offset;
	int second_segment_length = length - first_segment_length;
	int remaining = second_segment_length;
	
	// Insert these first characters in 'at'
	from->raw_insert(at, strdata, first_segment_length, carry_data, &carry_length);
	
	// Treat the remainder of strdata using a new pointer, 'data', with 'remaining_length'.
	const CharT* data = strdata + first_segment_length;
	memory_node *last = from,	*to = from->next, *m = nullptr;
	bool same_parent = to->parent == from->parent;
	
	if (to->siz + carry_length <= capacity) {
		// insert the carry data into the to node
		to->insert(iterator{to,0}, carry_data, carry_length);
		carry_length = 0;
	}
	
	while (remaining > 0) {
		
		int amt = 0;
		
		m = new memory_node(this);
		if (same_parent) {
			children.insert(children.iterator_to(*to), *m);
		} else {
			children.push_back(*m);
		}
		amt = std::min(remaining,capacity);

		m->raw_prepend(data, amt);
		
		data += amt;
		remaining -= amt;
		m->offset_start = last->offset_start + last->siz;
		
		last->next = m;
		last = m;
	}

	// if there's still carry data:
	if (carry_length > 0) {
		if (carry_length + m->siz <= capacity) { // put in last node
			last->raw_append(carry_data, carry_length);
		} else { // create a separate node
			m = new memory_node(this);
			if (same_parent) {
				children.insert(children.iterator_to(*to), *m);
			} else {
				children.push_back(*m);
			}
			m->raw_prepend(carry_data, carry_length);
			m->offset_start = last->offset_start + last->siz;
			last->next = m;
			last = m;
		}
	}
	
	
	// fixup offset_start within this subtree
	m->next = to;
	m = to;
	while (m && m->parent == this) {
		m->offset_start += last->offset_start + last->siz;
		last = m;
		m = m->next;
	}
	
	// update forward ancestor extents
	auto ma = this;
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

	// rebalance the tree
	this->rebalance();
	
}


template <typename CharT>
void tree_buffer<CharT>::span_node::rebalance ()
{
	if (this->children.size() <= SPAN_NODE_FANOUT) return;

	int c = SPAN_NODE_FANOUT;
	int nc = this->children.size();
	
	if (this->parent == nullptr) {
		// special case: root pivot
		span_node* new_root = new span_node();
		this->parent = new_root;
		new_root->children.push_back(*this);
	}

	int movn = this->children.size() % SPAN_NODE_FANOUT;
	if (movn == 0) movn += SPAN_NODE_FANOUT;
	
	// add new siblings to the current span_node, then transfer children to those siblings
	while (children.size() > SPAN_NODE_FANOUT) {

		span_node* sib = new span_node();
		sib->parent = this->parent;
		auto it = ++(this->parent->children.iterator_to(*this));
		this->parent->children.insert(it, *sib);
		int sz = 0;

		// pop_back some children and push_front them onto the new sibling
		for (int i=0; i < movn; i++) {
			auto& last = children.back();
			children.pop_back();
			sz += last.size();
			sib->children.push_front(last);
			last.parent = sib;
		}
		offset_end -= sz;
		sib->set_size(sz);
		
		// fixup offset_start and size in new sibling's children
		sib->children.front().fixup_siblings();		
		
		movn = SPAN_NODE_FANOUT;
		
	}
	
	// fixup offset_start in all created siblings
	this->fixup_siblings();
	
	// now there may be too many siblings, so rebalance up the tree
	this->parent->rebalance();
	
}


template <typename CharT>
void tree_buffer<CharT>::memory_node::raw_insert (const iterator& at, const CharT* strdata, int length, CharT* carry_data, int* carry_length)
{
	// The second half of the existing text might need to be bumped out and saved (carried).
	int bumped = this->siz - at.offset;
	if (bumped > 0 && carry_data == nullptr) {
		carry_data = (CharT*)alloca(sizeof(CharT) * bumped);
	}
	
	if (bumped) {
		std::copy(buf + at.offset, buf + this->siz, carry_data);
	}

	// This is the most characters that we can fit in this node.
	int effective_length = std::min(length, capacity - at.offset);
	std::copy(strdata, strdata + effective_length, buf + at.offset);

	// If there was carried text, some of it might still fit
	if (bumped) {
		
		// This is the amount of space that remains
		offset_type remainder = capacity - (at.offset + effective_length);
		if (remainder > 0) {

			// This is how much text we'll insert back in from the carry
			int reinsert = std::min(bumped,remainder);
			std::copy(carry_data, carry_data + reinsert, buf + at.offset + effective_length);

			// Whatever didn't fit, leave in the carry_data array and adjust carry_length accordingly.
			std::copy(carry_data + reinsert, carry_data + bumped, carry_data);
			
			bumped -= reinsert;
		}
	}
	if (carry_length) *carry_length = bumped;
	this->siz += (effective_length - bumped);
	assert(this->siz <= this->capacity);
}

template <typename CharT>
void tree_buffer<CharT>::memory_node::insert (const iterator& at, const CharT* strdata, int length)
{
	if (length + this->siz > capacity) {
		throw std::runtime_error("The preconditions of memory_node::insert require that the data fit entirely into the node");
	}
	
	raw_insert(at,strdata,length,nullptr,nullptr);
	
	// update forward sibling extents (uses parent's child list rather than memory_node's next pointers)
	auto ma = this->parent;
	if (ma) {
		auto iter = ma->children.iterator_to(*this);
		iter++;
		while (iter != ma->children.end()) {
			iter->offset_start += length;
			iter++;
		}
	}
	
	// update forward ancestor extents
	while (ma) {
		auto grandma = ma->parent;
		ma->offset_end += length;
		
		if (grandma != nullptr) {
			auto aunti = span_node::child_list::s_iterator_to(*ma);
			aunti++;
			while (aunti != grandma->children.end()) {
				aunti->offset_start += length;
				auto aunt = dynamic_cast<span_node*> (&(*aunti));
				assert(aunt != nullptr);
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
	// easy case: from and to are in the same memory node
	if (from.node == to.node) {
		from.node->remove(from,to);
		return;
	}
	
	// more difficult case: from and to are different nodes
	memory_node* node = from.node;
	node->remove(from,to);
	
	while (node != to.node) {
		
		span_node* parent = node->parent;
		
		typename span_node::child_list it = parent->children.iterator_to(node);
		it++;
		
	}
}

template <typename CharT>
void tree_buffer<CharT>::span_node::remove (iterator& from, iterator& to)
{
	
}

template <typename CharT>
void tree_buffer<CharT>::memory_node::remove (iterator& from, iterator& to)
{
	
}

std::ostream& operator<< (std::ostream& os, tree_buffer<char>::node& n)
{
	return n.printTo(os);
}

std::ostream& operator<< (std::ostream& os, tree_buffer<char>& b)
{
	return os << *(b.root);
}


#define GRAPHVIZ_ID_MASK 0xffffff

template<typename CharT>
std::ostream& tree_buffer<CharT>::dot (std::ostream& os)
{
	os << "digraph { " << endl;
	os << "node [ fontname=\"Liberation Sans\" ];" << endl;
	os << "rankdir=BT;" << endl;
	root->dot(os);
	os << "}" << endl;
	return os << dec;
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::span_node::dot (std::ostream& os)
{
	
	os << "node" << std::hex <<  ((unsigned long)(this) & GRAPHVIZ_ID_MASK)  << "[";
	os << "shape=folder, color=grey, ";
	os << "label=\"" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << endl;
	os << "start=" << std::dec << this->offset_start << ", ";
	os << "end=" << std::dec << this->offset_end << "\"];" << endl;
	
	for (auto& n : children) {
		n.dot(os);
	}

	if (children.size() > 0) {
		os << "{ rank=same";
		for (auto& it : children) {
			node& n = dynamic_cast<node&> (it);
			os << "; node" << std::hex <<  ((unsigned long)(&n) & GRAPHVIZ_ID_MASK);
		}
		os << "}" << endl << dec;
	}
		
	if (this->parent) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->parent) & GRAPHVIZ_ID_MASK) << " ;" << endl;
	}
	
	return os << std::dec;
}

template<typename CharT>
std::ostream& tree_buffer<CharT>::memory_node::dot (std::ostream& os)
{
	os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << "[";
	os << "shape=box3d, ";
	os << "label=" << "\"" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << endl;
	os << "start=" << std::dec << this->offset_start << endl;
	os << "size=" << std::dec << this->siz <<"\"];"<< endl;

	if (this->parent) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->parent) & GRAPHVIZ_ID_MASK) << " ;" << endl;
	}

	if (this->next) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->next) & GRAPHVIZ_ID_MASK);
		os << "[color=red];" << endl;
		
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
	os << "--";
	for (int i=0; i < this->siz; i++) {
		os << this->buf[i];
	}
	return os;
}


}
