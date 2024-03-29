#pragma once

#include <alloca.h>
#include <type_traits>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace util::detail {

// returns true if [a,b) overlaps [c,d)
bool overlaps (int a, int b, int c, int d)
{
	return (a>=c||b>c) && (a<d||b<=d);
}

template<typename T>
class node;
template<typename T>
class inner;
template<typename T>
class leaf;

template<typename T>
struct subtree_context
{
	inner<T>* subroot = nullptr;
	inner<T>* from = nullptr;
	inner<T>* to = nullptr;
};

namespace bi = boost::intrusive;

template<typename T>
class node
{
public:
	friend class util::skiparraylist<T>;
	
	inner<T>* parent;
	node<T>* _prev;
	node<T>* _next;
	offset_type offset;
	int siz;
	
	node () : parent(nullptr), offset(0), _prev(nullptr), _next(nullptr), siz(0) {}
	virtual ~node() { }
  
	virtual iterator<T> at (int pos) = 0;
	virtual offset_type size() const = 0;
	virtual void set_size (offset_type) = 0;
	virtual int insert (const iterator<T>& it, const T* strdata, int length) = 0;
	virtual int append (const T* strdata, int length) = 0;
	virtual void remove (int from, int to, subtree_context<T>* ctx) = 0;
	virtual bool check() const = 0;
	virtual std::ostream& printTo (std::ostream& os) const = 0;
	virtual std::ostream& dot (std::ostream& os, offset_type ofs) const = 0;

	virtual node<T>* next() { return _next; }
	virtual node<T>* prev() { return _prev; }

	void fixup_all_siblings_extents ();
	
	template<typename N>
	static void link (inner<T>* parent, N* from, N* middle, N* to) {
		assert(from != to);
		if (to && from) {
			assert(from->_next == to);
			assert(to->_prev == from);
		}
		middle->parent = parent;
		middle->_prev = from;
		middle->_next = to;
		if (from) {
			from->_next = middle;
		}
		if (to) {
			to->_prev = middle;
		}
	}
	template <typename N>
	static void unlink (N* n) {
		if (n->parent && n->parent->child == n) {
			if (n->_next && n->_next->parent == n->parent) {
				n->parent->child = n->_next;
			} else {
				n->parent->child = nullptr;
			}
		}
		n->parent = nullptr;
		auto savedprev = n->_prev;
		if (n->_prev) {
			n->_prev->_next = n->_next;
			n->_prev = nullptr;
		}
		if (n->_next) {
			n->_next->_prev = savedprev;
			n->_next = nullptr;
		}
	}
	
};


template<typename T>
class inner : public node<T>
{
public:
	
	node<T>* child;

	inner() : node<T>(), child(nullptr) {}
	virtual ~inner();

	iterator<T> at (int pos);
	offset_type size() const { return this->siz; }
	void set_size (offset_type n) { this->siz = n; }
	int insert (const iterator<T>& it, const T* strdata, int length);
	int append (const T* strdata, int length);
	void remove (int from, int to, subtree_context<T>* ctx);
	bool check() const;
	std::ostream& printTo (std::ostream& os) const;
	std::ostream& dot (std::ostream& os, offset_type ofs) const;

	void merge_small_nodes ();
	void rebalance_after_insert (bool full_height);
	static void rebalance_after_remove (inner<T>* left, inner<T>* right, subtree_context<T>* ctx);
	void rejoin (int nfrom, int nto, node<T>* to, subtree_context<T>* ctx);

	int num_children () const;

	inner<T>* prev() const { return reinterpret_cast<inner<T>*>(this->_prev); }
	inner<T>* next() const { return reinterpret_cast<inner<T>*>(this->_next); }
	
	bool empty () { return child == nullptr; }
	
	node<T>* front_child() { return child; }
	node<T>* back_child() {
		node<T>* n = child;
		while (n && n->_next && n->_next->parent == this) {
			n = n->_next;
		}
		return n;
  };

	template<typename N>
	void adopt_forward (N* n) {
		auto oldparent = n->parent;
		assert(oldparent == prev());
		if (oldparent->child == n) {
			oldparent->child = nullptr;
		}
		n->parent = this;
		this->child = n;
		// no updating of n->prev, n->next -- this node is already linked laterally
	}
	
	template<typename N>
	void adopt_backward (N* n) {
		auto oldparent = n->parent;
		assert(oldparent == next());
		assert(oldparent->child == n);
		if (n->next() && n->next()->parent == oldparent) {
			oldparent->child = n->next();
		} else {
			oldparent->child = nullptr;
		}
		n->parent = this;
		// no updating of n->prev, n->next -- this node is already linked laterally
	}
	
	template<typename N>
	void push_front (N* n) {
		n->_next = child;
		if (child) {
			n->_prev = child->_prev;
			if (child->_prev) {
				child->_prev->_next = n;
			}
			child->_prev = n;
		} else {
			if (prev()) {
				auto pc = prev()->back_child();
				n->_prev = pc;
				pc->_next = n;
			}
			if (next()) {
				auto nc = next()->front_child();
				n->_next = nc;
				nc->_prev = n;
			}
		}
		this->child = n;
		n->parent = this;
		this->siz += n->siz;
	}
	
	
	template<typename N>
	void push_back (N* n) {
		auto b = dynamic_cast<N*>(back_child());
		if (b) {
			n->_prev = b;
			n->_next = b->_next;
			if (b->_next) {
				b->_next->_prev = n;
			}
			b->_next = n;
		} else {
			if (next() && next()->front_child()) {
				auto nf = next()->front_child();
				n->_next = nf;
				nf->_prev = n;
			}
			if (prev() && prev()->back_child()) {
				auto pb = prev()->back_child();
				n->_prev = pb;
				pb->_next = n;
			}
			this->child = n;
		}
		n->parent = this;
		this->siz += n->siz;
	}

	
	node<T>* pop_back () {
		auto c = back_child();
		if (!c) return nullptr;
		int csz = c->siz;
		node<T>::unlink(c);
		this->siz -= csz;
		return c;
	}

	
	node<T>* pop_front () {
		auto c = front_child();
		if (!c) return nullptr;
		int csz = c->siz;
		node<T>::unlink(c);
		this->siz -= csz;
		return c;
	}
	
	
	void clear_and_delete_children () {
		auto c = child;
		while (c && c->parent == this) {
			auto next_child = c->_next;
			node<T>::unlink(c);
			delete c;
			c = next_child;
		}
	}

	
	void erase_and_delete (node<T>* n) {
		assert(n->parent == this);
		node<T>::unlink(n);
		delete(n);
	}

	
	void fixup_my_size ();
	void fixup_child_extents ();
	void fixup_ancestors_extents ();

};


/**
 * Refers to some contiguous range of memory.
 */
template <typename T>
class leaf : public node<T>
{
public:
	static constexpr int metadata_size() {
		return sizeof(node<T>);
	}
	static const int capacity = (LEAF_CAPACITY - metadata_size()) / sizeof(T);
	static_assert (capacity > 0, "Capacity is too small");

public:
	leaf () : node<T>() {}
	virtual ~leaf () { }
	
	iterator<T> at (int pos);
	offset_type size() const { return this->siz; }
	void set_size (offset_type ofs) { this->siz = ofs; }
	int insert (const iterator<T>& it, const T* strdata, int length);
	int append (const T* strdata, int length);
	void remove (int from, int to, subtree_context<T>* ctx);
	std::ostream& printTo (std::ostream& os) const;
	std::ostream& dot (std::ostream& os, offset_type ofs) const;
	bool check() const;

	leaf<T>* prev() const { return reinterpret_cast<leaf<T>*>(this->_prev); }
	leaf<T>* next() const { return reinterpret_cast<leaf<T>*>(this->_next); }
	
	int raw_insert (const iterator<T>& it, const T* strdata, int length, T* carry_data, int* carry_length);
	int raw_prepend (const T* strdata, int length) { return raw_insert(iterator<T>{this,0}, strdata, length, nullptr, nullptr); }
	int raw_append (const T* strdata, int length) { return raw_insert(iterator<T>{this,this->siz}, strdata, length, nullptr, nullptr); }
	
	T  data[capacity];
};


#ifdef DEBUG_UTIL
#define CHECK_AND_THROW(cond) if (!(cond)) { throw std::logic_error(#cond); } b &= (cond)
#else
#define CHECK_AND_THROW(cond) b &= ( cond )
#endif


template<typename T>
inner<T>::~inner<T> ()
{
	if (child) {
		clear_and_delete_children();
	}
}

template <typename T>
bool inner<T>::check() const
{
	bool b = true;
	
	CHECK_AND_THROW( (this->parent != nullptr) || this->child != nullptr );
	CHECK_AND_THROW( this->child->offset == 0 );
	
	auto last = this->child;
	auto cur = this->child->_next;
	int size_count = 0;
	
	while (cur && cur->parent == last->parent) {
		size_count += cur->size();
		last = last->_next;
		cur = cur->_next;
	}
	
	cur = this->child;
	while (cur && cur->parent == this) {
		CHECK_AND_THROW( cur->check() );
		cur = cur->_next;
	}

	CHECK_AND_THROW(this->num_children() <= NODE_FANOUT);

	CHECK_AND_THROW(this->child->offset == 0);
	return b;
}


template <typename T>
bool leaf<T>::check() const
{
	return this->siz > 0;
}


template <typename T>
iterator<T> inner<T>::at (int pos)
{
	if (pos > this->siz) {
		throw std::range_error ("pos > siz");
	}
	auto n = child;
	decltype(n) last = nullptr;
	while (n && n->parent == this) {
		if (n->offset <= pos) {
			last = n;
		} else {
			break;
		}
		n = n->_next;
	}
	
	assert(last);
	assert(last->parent == this);
	
	return last->at(pos - last->offset);
}


template <typename T>
iterator<T> leaf<T>::at (int pos)
{
	if (pos > this->siz) {
		std::cerr << "pos: " << pos << " siz: " << this->siz << std::endl << std::flush;
		throw std::range_error ("pos >= siz");
	}
	return { this, pos, true };
}


template <typename T>
int inner<T>::num_children () const
{
	int m = 0;
	auto n = child;
	while (n && n->parent == this) {
		n = n->_next;
		m++;
	}
	return m;
}


/**
 * Precondition: it.leaf is a child of this node.
 */
template <typename T>
int inner<T>::insert (const iterator<T>& it, const T* strdata, int length)
{
	const int capacity = leaf<T>::capacity;
	auto from = it.leaf;
	
	assert(from->parent == this);
	
	// Determine how many characters we can fit into the leaf pointed to by 'it'.
	int carry_length = from->siz - it.offset;
	T*  carry_data = (T*) alloca(sizeof(T) * carry_length);
	int first_segment_length = std::min(length,capacity - it.offset);
	int second_segment_length = length - first_segment_length;
	int remaining = second_segment_length;
	
	// Insert these first characters in 'at'
	from->raw_insert(it, strdata, first_segment_length, carry_data, &carry_length);
	
	// Treat the remainder of strdata using a new pointer, 'data', with 'remaining_length'.
	const T* data = strdata + first_segment_length;
	leaf<T> *last = from,	*to = from->next(), *m = from;
	bool same_parent = to && (to->parent == from->parent);
	
	if (to && (to->siz + carry_length <= capacity)) { 
		to->insert(iterator<T>{to,0,true}, carry_data, carry_length);
		carry_length = 0;
	}
	
	while (remaining > 0) {
		int amt = 0;
		m = new leaf<T>();
		node<T>::link(this,last,m,to);
		amt = std::min(remaining,capacity);
		
		m->raw_prepend(data, amt);
		
		data += amt;
		remaining -= amt;
		m->offset = last->offset + last->siz;
		last = m;
	}
	
	// if there's still carry data:
	if (carry_length > 0) {
		if (carry_length + last->siz <= capacity) { // put in last node
			last->raw_append(carry_data, carry_length);
		} else { // create a separate node
			m = new leaf<T>();
			node<T>::link(this, last, m, to);
			m->raw_prepend(carry_data, carry_length);
			m->offset = last->offset + last->siz;
			last = m;
		}
	}
	
	m->fixup_all_siblings_extents();
	
	// rebalance the tree
	this->rebalance_after_insert(false);

	// update forward ancestor extents
	fixup_ancestors_extents();
	
	return length;	
}


template <typename T>
int leaf<T>::insert (const iterator<T>& at_, const T* strdata, int length)
{
	iterator at = at_;
	int inserted = 0;
	
	inserted += raw_insert(at,strdata,length,nullptr,nullptr);
	
	this->parent->fixup_my_size();
	this->parent->fixup_child_extents();	
	this->parent->fixup_ancestors_extents();
	
	return inserted;
}


template <typename T>
int inner<T>::append (const T* strdata, int length)
{
	assert(this->parent == nullptr || num_children() > 0);
	int r = 0;
	if (num_children() == 0) {
		leaf<T>* m = new leaf<T>();
		push_back(m);
		r += m->append(strdata,length);
	} else {
		auto b = back_child();
		assert(b);
		r += b->append(strdata,length);
	}
	
	return r;
}


template <typename T>
int leaf<T>::append (const T* strdata, int length)
{
	inner<T>* parent = this->parent;
	leaf<T>* m = this;
	leaf<T>* saved_next = this->next();
	
	int r = raw_append(strdata,length);
	length -= r;
	strdata += r;
	
	while (length > 0) {
		leaf<T>* sib = new leaf<T>();
		parent->push_back(sib);
		sib->offset = this->offset + this->size();
		m->_next = sib;
		sib->_prev = m;
		r += sib->append(strdata,length);
		length -= r;
		strdata += r;
		m = sib;
	}
	m->_next = saved_next;
	this->fixup_all_siblings_extents();
	parent->rebalance_after_insert(false);
	parent->fixup_ancestors_extents();
	
	return r;
}


template <typename T>
void inner<T>::rebalance_after_insert (bool full_height)
{
	if (num_children() > NODE_FANOUT) {
		
		int nc = num_children();
		int movn = (nc < (NODE_FANOUT * 2)) ? nc / 2 : NODE_FANOUT;
		
		if (this->parent == nullptr) {
			// special case: root pivot
			inner<T>* new_root = new inner<T>;
			this->parent = new_root;
			new_root->push_back(this);
		}
		
		// add new siblings to the current node, then transfer children to those siblings
		while (num_children() > NODE_FANOUT) {
			
			inner<T>* sib = new inner<T>();
			node<T>::link(this->parent, this, sib, this->next());
			
			// transfer some children to the new sibling
			for (int i=0; i < movn; i++) {
				auto last = pop_back();
				sib->push_front(last);
			}
			sib->offset = this->offset + this->siz;
			sib->fixup_child_extents();
			sib->fixup_my_size();
			
			movn = (this->num_children() < (NODE_FANOUT * 2)) ? this->num_children() / 2 : NODE_FANOUT;
		}
		
	} else if (!full_height) {
		return; // end the recursion if we're not doing the full height
	}
	
	// now there may be too many siblings, so rebalance up the tree
	if (this->parent != nullptr) {
		this->parent->rebalance_after_insert(full_height);
	}
	
}


template <typename T>
int leaf<T>::raw_insert (const iterator<T>& it, const T* strdata, int length, T* carry_data, int* carry_length)
{
	// The second half of the existing text might need to be bumped out and saved (carried).
	int bumped = this->siz - it.offset;
	if (bumped > 0) {
		if (carry_data == nullptr) {
			carry_data = (T*)alloca(sizeof(T) * bumped);
		}
		std::copy(data + it.offset, data + this->siz, carry_data);
	}
	
	// This is the most characters that we can fit in this node.
	int effective_length = std::min(length, capacity - it.offset);
	std::copy(strdata, strdata + effective_length, data + it.offset);
	
	// If there was carried text, some of it might still fit
	if (bumped > 0) {
		
		// This is the amount of space that remains
		offset_type remainder = capacity - (it.offset + effective_length);
		if (remainder > 0) {
			
			// This is how much text we'll insert back in from the carry
			int reinsert = std::min(bumped,remainder);
			std::copy(carry_data, carry_data + reinsert, data + it.offset + effective_length);

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


template <typename T>
void inner<T>::remove (int from, int to, subtree_context<T>* ctx)
{
	auto n = front_child();
	bool in_overlap = false;
	bool found_from = false;
	bool found_to = false;
	while (n && n->parent == this) {
		int a = n->offset;
		int b = a + n->size();
		int c = from;
		int d = to;
		if (overlaps(a,b,c,d)) {
			
			n->remove(c-a,d-a,ctx);
			
			if (n->size() == 0) {
				bool delete_n = true;
				if (ctx->from == n) {
					delete_n = false;;
				}
				if (ctx->to == n) {
					delete_n = false;
				}
				if (delete_n) {
					auto m = n->_next;
					node<T>::unlink(n);
					this->siz -= n->siz; // optional, will be recalculated later since the entire ancestor chain needs it too.
					delete n;
					n = m;
					continue;
				}
			}
		}
		n = n->_next;
	}
	if (num_children() == 0) {
		this->siz = 0;
	}
	
}

template <typename T>
void leaf<T>::remove (int from, int to, subtree_context<T>* ctx)
{
	if (from >= 0 && from < this->siz) {
		ctx->from = this->parent;
	}
	if (to > 0 && to <= this->siz) {
		ctx->to = this->parent;
	}
	from = std::max(0, from);
	to = std::min(to, this->siz);
	std::copy(data + to, data + this->siz, data + from);
	this->siz -= (to - from);
}

template<typename T>
void inner<T>::rebalance_after_remove (inner<T>* left, inner<T>* right, subtree_context<T>* ctx)
{
	inner<T>* next_left = left ? left->parent : nullptr;
	inner<T>* next_right = right ? right->parent : nullptr;

	int lc = left->num_children();
	int rc = (left==right) ? lc : right->num_children();
	
	if (left == right) {
		if (lc == 1) {
			auto only = left->front_child();
			if (left->next()) {
				right = left->next();
				rc = right->num_children();
				next_right = right->parent;
			} else if (left->prev()) {
				left = left->prev();
				lc = left->num_children();
				next_left = left->parent;
		  } else {
				// this can happen near the root for "remnant chains".
			}
		}
	} 

	if (left != right) {
		if ((lc==1 && rc==3) || (rc==1 && lc>0 && lc<3)) { // shift-left condition according to table
			auto node = right->front_child();
			left->adopt_backward(node);
			rc--; lc++;
		} else if ((lc==3 && rc==1) || (lc==1 && rc==2)) { // shift-right condition according to table
			auto node = left->back_child();
			right->adopt_forward(node);
			lc--; rc++;
		}
	} else {
		// again, this happens in the remnant chain case.
	}
	
	if (lc == 0) {
		node<T>::unlink(left);
		delete left;
	} else {
		left->fixup_child_extents();
		left->fixup_my_size();
	}

	if (left != right) {
		if (rc == 0) {
			node<T>::unlink(right);
			delete right;
		} else  {
			right->fixup_child_extents();
			right->fixup_my_size();
		}
	}
	
	if (next_left || next_right) {
		rebalance_after_remove(next_left, next_right, ctx);
	}
	
}


template<typename T>
void inner<T>::merge_small_nodes ()
{
	int nc = this->num_children();
	int nnc = this->next() ? this->next()->num_children() : 0;
	
	if (nc + nnc <= NODE_FANOUT && nnc > 0) {
		auto sib = this->next();
		for (auto n = sib->child; n && n->parent == sib; n = n->next()) {
			n->parent = this;
		}
		node<T>::unlink(sib);
		this->fixup_child_extents();
		this->fixup_my_size();
	}
}


template <typename T>
void node<T>::fixup_all_siblings_extents()
{
	if (this->parent == nullptr) return;
	int ofs = 0;
	int saved_size = 0;
	for (auto n = this->parent->front_child(); n && n->parent == this->parent; n = n->_next) {
		saved_size = n->size();
		n->offset = ofs;
		n->set_size(saved_size);
		ofs += saved_size;
	}
}


template <typename T>
void inner<T>::fixup_my_size ()
{
	int sz = 0;
	for (auto n = child; n && n->parent == this; n = n->_next) {
		sz += n->size();
	}
	this->siz = sz;
}


template <typename T>
void inner<T>::fixup_child_extents ()
{
	if (empty()) return;
	node<T>* n = child;
	int size = n->size();
	n->offset = 0;
	n->set_size(size);
	n->fixup_all_siblings_extents();
}


template <typename T>
void inner<T>::fixup_ancestors_extents ()
{
	inner<T>* ma = this;
	ma->fixup_my_size();
	while (ma) {
		inner<T>* grandma = ma->parent;
		if (grandma != nullptr) {
 			ma->fixup_all_siblings_extents();
			grandma->fixup_my_size();
		}
		ma = grandma;
	}
}

} // namespace util::detail
