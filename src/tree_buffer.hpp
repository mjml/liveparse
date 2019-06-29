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

// returns true if [a,b) overlaps [c,d)
bool overlaps (int a, int b, int c, int d)
{
	return (a>=c||b>c) && (a<d||b<=d);
}

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
	
	friend std::ostream& operator<< (std::ostream& os, node& n);
	friend std::ostream& operator<< (std::ostream& os, tree_buffer<char>& b);
	
	struct iterator {
	public:
		memory_node* mnode = nullptr;
		offset_type offset = 0;
		bool valid = false;

		bool operator== (const iterator& o) { return mnode == o.mnode && offset == o.offset; }
		iterator& operator++ ();
		iterator& operator-- ();
		iterator& operator+= (offset_type i);
		iterator& operator-= (offset_type i);
		iterator* operator->() { return this; }
		const iterator* operator->() const { return this; }
		CharT& operator* () const { return mnode->buf[offset]; }

		static bool is_end (const iterator& it) { return it.mnode==nullptr && it.valid; }
		static iterator end () { return iterator{nullptr,0,true}; }
	};
	
	class node_disposer {
	public:
		void operator()(node* p) { delete p; }
	};

	
	class node
	{
	public:
		friend class tree_buffer<CharT>;
		typedef node node_t;

		node() : parent(nullptr), offset_start(0) {}
		node(span_node* p) : parent(p), offset_start(0) {}
		virtual ~node() { }
		
		virtual iterator at (int pos) = 0;
		virtual offset_type size() = 0;
		virtual void set_size (offset_type) = 0;
		virtual std::ostream& printTo (std::ostream& os) = 0;
		virtual std::ostream& dot (std::ostream& os) = 0;
		virtual int insert (const iterator& it, const CharT* strdata, int length) = 0;
		virtual int append (const CharT* strdata, int length) = 0;
		virtual void fixup_siblings_extents () {
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

		friend std::ostream& operator<< (std::ostream& os, node& n);
		
	};
	
	class span_node : public node
	{
	public:
		typedef span_node span_t;
		typedef bi::list < node, typename node::list_member_type > child_list;
		friend class node;
		
		span_node() : node(), offset_end() {}
		span_node(span_node* p) : node(p), offset_end() {}
		virtual ~span_node() { children.clear_and_dispose(node_disposer());  }
		
		int get_num_children() { return children.size(); }
		iterator at (int pos);
		offset_type size() { return offset_end - node::offset_start; }
		void set_size (offset_type ofs) { offset_end = node::offset_start + ofs; }
		int insert (const iterator& it, const CharT* strdata, int length);
		int append (const CharT* strdata, int length);
		void remove (int from, int to);
		std::ostream& printTo (std::ostream& os);
		std::ostream& dot (std::ostream& os);

		void fixup_my_extents ();
		void fixup_child_extents ();
		void fixup_ancestors_extents ();
		
		void rebalance_after_insert ();
		void rebalance_after_remove ();

		child_list  children;
		offset_type offset_end;
		
	};
	
	class memory_node_metadata {
	public:
		memory_node_metadata() : siz(0), next(nullptr) {}
		offset_type  siz;
		//memory_node* prev;
		memory_node* next;
	};
	
	/**
	 * Refers to some contiguous range of memory.
	 */
	class memory_node : public node, public memory_node_metadata
	{
	public:
		typedef memory_node mem_t;
		friend class span_node;

		static constexpr int metadata_size() {
			return sizeof(node) + sizeof(memory_node_metadata);
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
		int insert (const iterator& it, const CharT* strdata, int length);
		int append (const CharT* strdata, int length);
		void remove (int from, int to);
		std::ostream& printTo (std::ostream& os);
		std::ostream& dot (std::ostream& os);

		int raw_insert (const iterator& it, const CharT* strdata, int length, CharT* carry_data, int* carry_length);
		int raw_prepend (const CharT* strdata, int length) { return raw_insert(iterator{this,0}, strdata, length, nullptr, nullptr); }
		int raw_append (const CharT* strdata, int length) { return raw_insert(iterator{this,this->siz}, strdata, length, nullptr, nullptr); }
		
		CharT        buf[capacity];
		
	};
	
public:
	// Temporary provisioned types to be replaced by formal iterator classes
	typedef iterator marker;
	
	tree_buffer() : root(nullptr) {}
	~tree_buffer() {
		if (root) delete root;
	}
	
	iterator begin ();
	iterator end ();
	iterator at (int pos);
	int pos (iterator& it);
	
	int size ();
	void clear ();
	void insert (int pos, CharT* strdata, int length);
	void insert (const iterator& it, CharT* strdata, int length);
	void append (const CharT* strdata, int length);
	void remove (int from, int to);
	void remove (iterator& from, iterator& to);
	
	std::ostream& operator<< (std::ostream& os);
	std::ostream& dot (std::ostream& os);

PROTECTED:
	span_node* root;
	
};


template <typename CharT>
int tree_buffer<CharT>::size ()
{
	if (!root) return 0;
	return root->size();
}

template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::begin ()
{
	return at(0);
}
	
template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::end ()
{
	return iterator{nullptr,0,false};
}
	

template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::at (int pos)
{
	if (!root) { return iterator::end(); }
	if (pos == root->size()) { return iterator::end(); }
	return root->at(pos);
}


template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::span_node::at (int pos)
{
	if (pos > this->size()) {
		throw std::range_error ("pos > siz");
	}
	typename span_node::child_list::iterator last;
	auto child = children.begin();
	while (child != children.end()) {
		if (child->offset_start <= pos) {
			last = child;
		} else {
			break;
		}
		child++;
	}
	if (child == children.end()) {
		if (pos >= last->size()) {
			return iterator{nullptr,0,false};
		}
	}
	return last->at(pos - last->offset_start);
}


template <typename CharT>
typename tree_buffer<CharT>::iterator tree_buffer<CharT>::memory_node::at (int pos)
{
	if (pos > this->siz) {
		std::cerr << "pos: " << pos << " siz: " << this->siz << endl << flush;
		throw std::range_error ("pos > siz");
	}
	return { this, pos, true };
}


template <typename CharT>
void tree_buffer<CharT>::insert (int pos, CharT* strdata, int length)
{
	if (root == nullptr) {
		root = new span_node();
		auto mnode = new memory_node(root);
		root->children.push_back(*mnode);
	}
	auto it = at(pos);
	insert(it, strdata, length);
}


template <typename CharT>
void tree_buffer<CharT>::insert (const iterator& at, CharT* strdata, int length)
{
	if (iterator::is_end(at)) {
		root->append(strdata,length);
		return;
	}
	
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

/**
 * Precondition: at.mnode is a child of this span_node.
 */
template <typename CharT>
int tree_buffer<CharT>::span_node::insert (const iterator& at, const CharT* strdata, int length)
{
	const int capacity = memory_node::capacity;
	auto from = at->mnode;
	
	assert(from->parent == this);
	
	// Determine how many characters we can fit into the memory_node pointed to by 'at'.
	int carry_length = from->siz - at->offset;
	CharT* carry_data = (CharT*) alloca(sizeof(CharT) * carry_length);
	int first_segment_length = std::min(length,capacity - at->offset);
	int second_segment_length = length - first_segment_length;
	int remaining = second_segment_length;
		
	// Insert these first characters in 'at'
	from->raw_insert(at, strdata, first_segment_length, carry_data, &carry_length);
		
	// Treat the remainder of strdata using a new pointer, 'data', with 'remaining_length'.
	const CharT* data = strdata + first_segment_length;
	memory_node *last = from,	*to = from->next, *m = from;
	bool same_parent = to && (to->parent == from->parent);
		
	if (to && (to->siz + carry_length <= capacity)) {
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
	
	m->fixup_siblings_extents();
	
	// update forward ancestor extents
	fixup_ancestors_extents();
	
	// rebalance the tree
	this->rebalance_after_insert();

	return length;	
}


template <typename CharT>
int tree_buffer<CharT>::memory_node::insert (const iterator& at_, const CharT* strdata, int length)
{
	iterator at = at_;
	int inserted = 0;
	
	inserted += raw_insert(at,strdata,length,nullptr,nullptr);
	
	this->parent->fixup_my_extents();
	this->parent->fixup_child_extents();	
	this->parent->fixup_ancestors_extents();
	
	return inserted;
}


template <typename CharT>
void tree_buffer<CharT>::append (const CharT* strdata, int length)
{
	if (!root) {
		root = new span_node();
	}
	int r = root->append(strdata,length);
	
	assert(r == length);
	
	// last step: adjust to possibility of pivoted root
	while (root->parent != nullptr) {
		root = root->parent;
	}
	
}


template <typename CharT>
int tree_buffer<CharT>::span_node::append (const CharT* strdata, int length)
{
	assert(this->parent == nullptr || children.size() > 0);
	int r = 0;
	if (children.size() == 0) {
		memory_node* m = new memory_node(this);
		children.push_back(*m);
		r += m->append(strdata,length);
	} else {
		r += children.back().append(strdata,length);		
	}

	return r;
}


template <typename CharT>
int tree_buffer<CharT>::memory_node::append (const CharT* strdata, int length)
{
	span_node* parent = this->parent;
	memory_node* m = this;

	int r = raw_append(strdata,length);
	length -= r;
	strdata += r;

	while (length > 0) {
		memory_node* sib = new memory_node(parent);
		parent->children.push_back(*sib);
		sib->offset_start = this->offset_start + this->size();
		m->next = sib;
		r += sib->append(strdata,length);
		length -= r;
		strdata += r;
		m = sib;
	}
	
	parent->rebalance_after_insert();
	
	return r;
}



template <typename CharT>
void tree_buffer<CharT>::span_node::rebalance_after_insert ()
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

		span_node* sib = new span_node(this->parent);
		auto it = ++(this->parent->children.iterator_to(*this));
		this->parent->children.insert(it, *sib);
		int sz = 0;

		// pop_back some children and push_front them onto the new sibling
		for (int i=0; i < movn; i++) {
			auto &last = children.back();
			children.pop_back();
			sz += last.size();
			sib->children.push_front(last);
			last.parent = sib;
		}
		offset_end -= sz;
		sib->set_size(sz);
		
		// fixup offset_start and size in new sibling's children
		sib->fixup_child_extents();		
		
		movn = SPAN_NODE_FANOUT;
		
	}
	
	// fixup offset_start in all created siblings
	this->fixup_siblings_extents();
	
	// now there may be too many siblings, so rebalance up the tree
	this->parent->rebalance_after_insert();
	
}


template <typename CharT>
int tree_buffer<CharT>::memory_node::raw_insert (const iterator& at, const CharT* strdata, int length, CharT* carry_data, int* carry_length)
{
	// The second half of the existing text might need to be bumped out and saved (carried).
	int bumped = this->siz - at.offset;
	if (bumped > 0) {
		if (carry_data == nullptr) {
			carry_data = (CharT*)alloca(sizeof(CharT) * bumped);
		}
		std::copy(buf + at.offset, buf + this->siz, carry_data);
	}
	
	// This is the most characters that we can fit in this node.
	int effective_length = std::min(length, capacity - at.offset);
	std::copy(strdata, strdata + effective_length, buf + at.offset);
	
	// If there was carried text, some of it might still fit
	if (bumped > 0) {
		
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
	
	return effective_length;
}


template <typename CharT>
void tree_buffer<CharT>::remove (int from, int to)
{
	root->remove(from,to);
}


template <typename CharT>
void tree_buffer<CharT>::remove (iterator& from, iterator& to)
{
	/* just use the iterators to obtain absolute positions, then remove using the absolute positions  */
	remove(pos(from), pos(to));
}

template <typename CharT>
void tree_buffer<CharT>::span_node::remove (int from, int to)
{
	// fast case: [from,to) spans this entire node, so remove all children
	if (from <= this->offset_start && to >= this->offset_end) {
		children.clear_and_dispose(node_disposer());
	}
	
	// more general case
	auto it = children.cbegin();
	while (it != children.cend()) {
		int a = it->offset_start;
		int b = a + it->size();
		int c = from;
		int d = to;
    if (overlaps(a,b,c,d)) {
			it->remove(c-a,d-a);
			if (it->size() == 0) {
				it = children.erase(it);
				// memory_node->next pointers need to be updated --- AHH single-linked list pains --- might need a doubly linked list among leaves
				continue;
			} 
		}
		it++;
	}
	rebalance_after_remove();
}

template <typename CharT>
void tree_buffer<CharT>::memory_node::remove (int from, int to)
{
	from = std::max(0, from);
	to = std::min(to, this->siz);
	std::copy(buf + to, buf + this->siz, buf + from);
	this->siz -= (to - from);
}


template <typename CharT>
void tree_buffer<CharT>::span_node::rebalance_after_remove ()
{
	// check for zero-length children and remove them
	for (auto &it: children) {
		if (it->size() == 0) {
			children.erase_and_dispose(it, node_disposer());
		}
	}
	// todo: check for redundant ancestor chains and remove them
}

template <typename CharT>
void tree_buffer<CharT>::span_node::fixup_my_extents ()
{
	int offset_end = this->offset_start;
	for (auto &it : children) {
		offset_end += it.size();
	}
	this->offset_end = offset_end;
}

template <typename CharT>
void tree_buffer<CharT>::span_node::fixup_child_extents ()
{
	if (children.empty()) return;
	children.front().fixup_siblings_extents();
}


template <typename CharT>
void tree_buffer<CharT>::span_node::fixup_ancestors_extents ()
{
	span_node* ma = this;
	while (ma) {
		span_node* grandma = ma->parent;
		if (grandma != nullptr) {
 			ma->fixup_siblings_extents();
			grandma->fixup_my_extents();
		}
		ma = grandma;
	}
}


template<typename CharT>
std::ostream& operator<< (std::ostream& os, typename liveparse::tree_buffer<CharT>::node& n)
{
	return n.printTo(os);
}


std::ostream& operator<< (std::ostream& os, tree_buffer<char>::node& n)
{
	return n.printTo(os);
}

std::ostream& operator<< (std::ostream& os, tree_buffer<char16_t>::node& n)
{
	return n.printTo(os);
}



template<typename CharT>
std::ostream& operator<< (std::ostream& os, tree_buffer<CharT>& b)
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
	for (int i=0; i < this->siz; i++) {
		os << this->buf[i];
	}
	return os;
}


}
