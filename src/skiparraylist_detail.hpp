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
class leaf;

namespace bi = boost::intrusive;

template<typename T>
class node
{
public:
	friend class util::tree_buffer<T>;
	typedef node<T> node_t;
	
	node() : parent(nullptr), offset_start(0) {}
	node(span_node<T>* p) : parent(p), offset_start(0) {}
	virtual ~node() { }
  
	virtual iterator<T> at (int pos);
	virtual offset_type size() const;
	virtual void set_size (offset_type);
	virtual std::ostream& printTo (std::ostream& os) const;
	virtual std::ostream& dot (std::ostream& os) const;
	virtual int insert (const iterator<T>& it, const T* strdata, int length);
	virtual int append (const T* strdata, int length);
	virtual void remove (int from, int to, subtree_context<T>* ctx);
	virtual void merge_small_nodes (node<T>* to);
	void fixup_all_siblings_extents ();
	virtual bool check() const;

	static void insert (node<T>* from, node<T>* to, node<T>* middle) {
		middle->prev = from;
		middle->next = to;
		if (from) {
			from->next = middle;
		}
		if (to) {
			to->prev = middle;
		}
	}
	
	node<T>* parent;
	node<T>* prev;
	node<T>* next;
	node<T>* child;
	offset_type offset;
	int siz;
};


/**
 * Refers to some contiguous range of memory.
 */
template <typename T>
class leaf : public node<T>
{
public:
	static constexpr int metadata_size() {
		return sizeof(node<T>) + sizeof(leaf_metadata<T>);
	}
	static const int capacity = (LEAF_CAPACITY - metadata_size()) / sizeof(T);
	static_assert (capacity > 0, "Capacity is too small");
		
public:
	leaf () {}
	leaf (span_node<T>* p) : node<T>(p) {}
	virtual ~leaf () { }
	
	iterator<T> at (int pos);
	offset_type size() const { return this->siz; } 
	void set_size (offset_type ofs) { this->siz = ofs; }
	int insert (const iterator<T>& it, const T* strdata, int length);
	int append (const T* strdata, int length);
	void remove (int from, int to, subtree_context<T>* ctx);
	void rejoin (int nfrom, int nto, node<T>* to, subtree_context<T>* ctx);
	std::ostream& printTo (std::ostream& os) const;
	std::ostream& dot (std::ostream& os) const;
	bool check() const;
	
	int raw_insert (const iterator<T>& it, const T* strdata, int length, T* carry_data, int* carry_length);
	int raw_prepend (const T* strdata, int length) { return raw_insert(iterator<T>{this,0}, strdata, length, nullptr, nullptr); }
	int raw_append (const T* strdata, int length) { return raw_insert(iterator<T>{this,this->siz}, strdata, length, nullptr, nullptr); }
	
	T  buf[capacity];
};


#ifdef DEBUG_UTIL
#define CHECK_AND_THROW(cond) if (!(cond)) { throw std::logic_error(#cond); } b &= (cond)
#else
#define CHECK_AND_THROW(cond) b &= ( cond )
#endif

template <typename T>
bool node<T>::check() const
{
	bool b = true;
	
	CHECK_AND_THROW( (this->parent != nullptr) || this->child != nullptr );
	CHECK_AND_THROW( this->child->offset == 0 );
	
	auto last = this->child;
	auto cur = this->child->next;
	int size_count = 0;
	
	while (cur->parent == last->parent) {
		size_count += curnode->size();
		last = last->next;
		cut = cur->next;
	}
	
	for (auto &n : children) {
		CHECK_AND_THROW( n.check() );
	}
	return b;
}

template <typename T>
bool leaf<T>::check() const
{
	return this->siz > 0;
}

template <typename T>
iterator<T> node<T>::at (int pos)
{
	if (pos > siz) {
		throw std::range_error ("pos > siz");
	}
	auto node = child;
	decltype(node) last = nullptr;
	while (node && node->parent == this) {
		if (child->offset <= pos) {
			last = child;
		} else {
			break;
		}
		node = node->next;
	}
	
	assert(last);
	assert(last->parent == this);
	
	return last->at(pos - last->offset);
}


template <typename T>
iterator<T> leaf<T>::at (int pos)
{
	if (pos >= siz) {
		std::cerr << "pos: " << pos << " siz: " << siz << std::endl << std::flush;
		throw std::range_error ("pos >= siz");
	}
	return { this, pos, true };
}


/**
 * Precondition: at.mnode is a child of this node.
 */
template <typename T>
int node<T>::insert (const iterator<T>& it, const T* strdata, int length)
{
	const int capacity = leaf<T>::capacity;
	auto from = it->leaf;
	
	assert(from->parent == this);
	
	// Determine how many characters we can fit into the leaf pointed to by 'at'.
	int carry_length = from->siz - it->offset;
	T* carry_data = (T*) alloca(sizeof(T) * carry_length);
	int first_segment_length = std::min(length,capacity - it->offset);
	int second_segment_length = length - first_segment_length;
	int remaining = second_segment_length;
	
	// Insert these first characters in 'at'
	from->raw_insert(it, strdata, first_segment_length, carry_data, &carry_length);
	
	// Treat the remainder of strdata using a new pointer, 'data', with 'remaining_length'.
	const T* data = strdata + first_segment_length;
	leaf<T> *last = from,	*to = from->next, *m = from;
	bool same_parent = to && (to->parent == from->parent);
	
	if (to && (to->siz + carry_length <= capacity)) { 
		to->insert(iterator<T>{to,0,true}, carry_data, carry_length);
		carry_length = 0;
	}
	
	while (remaining > 0) {
		
		int amt = 0;
		
		m = new leaf<T>(this);
		
		node<T>::insert(from,to,m);
		amt = std::min(remaining,capacity);
		
		m->raw_prepend(data, amt);
		
		data += amt;
		remaining -= amt;
		m->offset = last->offset + last->siz;
		
		last->next = m;
		last = m;
	}
	
	// if there's still carry data:
	if (carry_length > 0) {
		if (carry_length + m->siz <= capacity) { // put in last node
			last->raw_append(carry_data, carry_length);
		} else { // create a separate node
			m = new leaf<T>(this);
			if (same_parent) {
				children.insert(children.iterator_to(*to), *m);
			} else {
				children.push_back(*m);
			}
			m->raw_prepend(carry_data, carry_length);
			m->offset_start = last->offset_start + last->siz;
			m->prev = last;
			m->next = last->next;
			last->next = m;
			last = m;
		}
	}
	last->next = to;
	
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
	
	this->parent->fixup_my_extents();
	this->parent->fixup_child_extents();	
	this->parent->fixup_ancestors_extents();
	
	return inserted;
}


template <typename T>
int node<T>::append (const T* strdata, int length)
{
	assert(this->parent == nullptr || children.size() > 0);
	int r = 0;
	if (children.size() == 0) {
		leaf<T>* m = new leaf<T>(this);
		children.push_back(*m);
		r += m->append(strdata,length);
	} else {
		r += children.back().append(strdata,length);		
	}

	return r;
}


template <typename T>
int leaf<T>::append (const T* strdata, int length)
{
	node<T>* parent = this->parent;
	leaf<T>* m = this;
	leaf<T>* next = this->next;

	int r = raw_append(strdata,length);
	length -= r;
	strdata += r;

	while (length > 0) {
		leaf<T>* sib = new leaf<T>(parent);
		parent->children.push_back(*sib);
		sib->offset_start = this->offset_start + this->size();
		m->next = sib;
		sib->prev = m;
		r += sib->append(strdata,length);
		length -= r;
		strdata += r;
		m = sib;
	}
	m->next = next;
	this->fixup_all_siblings_extents();
	parent->rebalance_after_insert(false);
	parent->fixup_ancestors_extents();
	
	return r;
}



template <typename T>
void node<T>::rebalance_after_insert (bool full_height)
{
	if (this->children.size() > NODE_FANOUT) {
	
		int nc = this->children.size();
		
		if (this->parent == nullptr) {
			// special case: root pivot
			node<T>* new_root = new node<T>;
			this->parent = new_root;
			new_root->children.push_back(*this);
		}
		
		int movn = (this->children.size() < (NODE_FANOUT * 2)) ? this->children.size() / 2 : NODE_FANOUT;
		
		// add new siblings to the current node, then transfer children to those siblings
		while (children.size() > NODE_FANOUT) {
		
			node<T>* sib = new node<T>(this->parent);
			auto it = ++(this->parent->children.iterator_to(*this));
			this->parent->children.insert(it, *sib);
			int sz;
			sz = 0;
			
			// pop_back some children (at least 2) and push_front them onto the new sibling
			for (int i=0; i < movn; i++) {
				auto &last = children.back();
				children.pop_back();
				sz += last.size();
				sib->children.push_front(last);
				last.parent = sib;
			}
			this->offset_end -= sz;
			sib->offset_start = this->offset_end;
			sib->offset_end = sib->offset_start + sz;
			sib->fixup_child_extents();
			
			movn = (this->children.size() < (NODE_FANOUT * 2)) ? this->children.size() / 2 : NODE_FANOUT;
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
int leaf<T>::raw_insert (const iterator<T>& at, const T* strdata, int length, T* carry_data, int* carry_length)
{
	// The second half of the existing text might need to be bumped out and saved (carried).
	int bumped = this->siz - at.offset;
	if (bumped > 0) {
		if (carry_data == nullptr) {
			carry_data = (T*)alloca(sizeof(T) * bumped);
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


template <typename T>
void node<T>::remove (int from, int to, subtree_context<T>* ctx)
{
	// fast case: [from,to) spans this entire node, so remove all children
	if (from <= 0 && to >= size()) {
		children.clear_and_dispose(node_disposer<T>());
	}
	
	// more general case
	auto it = children.begin();
	bool in_overlap = false;
	bool found_from = false;
	bool found_to = false;
	while (it != children.end()) {
		int a = it->offset_start;
		int b = a + it->size();
		int c = from;
		int d = to;
		if (overlaps(a,b,c,d)) {
			
			auto from_pre = ctx->from;
			auto to_pre = ctx->to;
			it->remove(c-a,d-a,ctx);
			if (ctx->from != from_pre) { found_from = true; }
			if (ctx->to != to_pre) { found_to = true; }
			
			if (it->size() == 0) {
				if (&(*it) == ctx->from) {
					ctx->from = nullptr;
				}
				if (&(*it) == ctx->to) {
					ctx->to = nullptr;
				}
				typename child_list::const_iterator cit = this->children.iterator_to(*it);
				it++;
				// bug: when a memory node is reclaimed, the new last node contains a dangling 'next' pointer
				children.erase_and_dispose(cit, node_disposer<T>());
				continue;
			} 
		}
		it++;
	}
	if (children.size() == 0) {
		this->offset_end = this->offset_start;
	}
	
	if (ctx->subroot == nullptr && found_from && found_to) {
		ctx->subroot = this;
	}
	
}

template <typename T>
void leaf<T>::remove (int from, int to, subtree_context<T>* ctx)
{
	
	if (from >= 0 && from < this->siz) ctx->from = this->parent;
	if (to > 0 && to <= this->siz) ctx->to = this->parent;
	from = std::max(0, from);
	to = std::min(to, this->siz);
	std::copy(buf + to, buf + this->siz, buf + from);
	this->siz -= (to - from);
	
}


template <typename T>
void node<T>::rejoin (int nfrom, int nto, node<T>* to, subtree_context<T>* ctx)
{ 
	node<T>* next_left = nullptr;
	node<T>* next_right = nullptr;
	bool bottom = false;
	
	
	if (this == to) {
		ctx->subroot = this;
	}
	
	if (!ctx->from) {
		auto it = this->children.begin();
		while (it != this->children.end()) {
			if (it->offset_start + it->size() <= nfrom) {
				next_left = &(*it); 
			}
			it++;
		}
	}
	if (!next_left) {
		ctx->from = this;
	}
	
	if (!ctx->to) {
		auto it = to->children.rbegin();
		while (it != to->children.rend()) {
			if (it->offset_start >= nto) {
				next_right = &(*it);
			}
			it++;
		}
	}
	if (!next_right) {
		ctx->to = to;
	}
	
	if (!next_left && next_right) {
		if (this->children.size() > 0) {
			next_left = &(*this->children.back());
		} else {
			next_left = next_right;
		}
	}
	if (next_left && !next_right) {
		if (to->children.size() > 0) {
			next_right = &(*to->children.front());
		} else {
			next_right = next_left;
		}
	}
	
	if (next_left && next_right) {
		next_left->rejoin(nfrom - next_left->offset_start, nto - next_right->offset_start, next_right, ctx);
	}
	
	if (this->parent && this->children.size() == 0) {
		typename node<T>::child_list::const_iterator cit = this->parent->children.iterator_to(*this);
		this->parent->children.erase_and_dispose(cit,node_disposer<T>());
	}
	if (this->parent && to->children.size() == 0) {
		typename node<T>::child_list::const_iterator cit = to->parent->children.iterator_to(*to);
		to->parent->children.erase_and_dispose(cit,node_disposer<T>());
	}
	if (this->children.size() == 1 || to->children.size() == 1) {
		//this->merge_small_nodes(to);
	}
	
}


void leaf<T>::rejoin (int nfrom, int nto, node<T>* to, subtree_context<T>* ctx)
{
	leaf<T>* mto = dynamic_cast<leaf<T>*>(to);
	assert(mto != nullptr);

	// todo
	
}


template <typename T>
void node<T>::fixup_all_siblings_extents()
{
	if (parent == nullptr) return;
	int ofs = 0;
	int saved_size = 0;
	for (auto it = parent->children.begin(); it != parent->children.end(); it++) {
		saved_size = it->size();
		it->offset_start = ofs;
		it->set_size(saved_size);
		ofs += saved_size;
	}
}


template <typename T>
void node<T>::fixup_my_extents ()
{
	int offset_end = this->offset_start;
	for (auto &it : children) {
		node<T>& n = it;
		offset_end += n.size();
	}
	this->offset_end = offset_end;
}


template <typename T>
void node<T>::fixup_child_extents ()
{
	if (children.empty()) return;
	node<T>& n = children.front();
	int size = n.size();
	n.offset_start = 0;
	n.set_size(size);
	n.fixup_all_siblings_extents();
}


template <typename T>
void node<T>::fixup_ancestors_extents ()
{
	node<T>* ma = this;
	ma->fixup_my_extents();
	while (ma) {
		node<T>* grandma = ma->parent;
		if (grandma != nullptr) {
 			ma->fixup_all_siblings_extents();
			grandma->fixup_my_extents();
		}
		ma = grandma;
	}
}

} // namespace util::detail
