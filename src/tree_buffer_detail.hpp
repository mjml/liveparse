#pragma once

#include <alloca.h>
#include <type_traits>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace treebuffer::detail {

// returns true if [a,b) overlaps [c,d)
bool overlaps (int a, int b, int c, int d)
{
	return (a>=c||b>c) && (a<d||b<=d);
}

template<typename T>
class node;
template<typename T>
class span_node;
template<typename T>
class memory_node;

template<typename T>
class node_disposer {
public:
	void operator()(node<T>* p) {
		//std::cout << p << std::endl << std::flush;
		delete p;

	}
};

namespace bi = boost::intrusive;

template<typename T>
class node
{
public:
	friend class treebuffer::tree_buffer<T>;
	typedef node<T> node_t;
	
	node() : parent(nullptr), offset_start(0) {}
	node(span_node<T>* p) : parent(p), offset_start(0) {}
	virtual ~node() { }
  
	virtual iterator<T> at (int pos) = 0;
	virtual offset_type size() const = 0;
	virtual void set_size (offset_type) = 0;
	virtual std::ostream& printTo (std::ostream& os) const = 0;
	virtual std::ostream& dot (std::ostream& os) const = 0;
	virtual int insert (const iterator<T>& it, const T* strdata, int length) = 0;
	virtual int append (const T* strdata, int length) = 0;
	virtual void remove (int from, int to, memory_node<T>** from_n, memory_node<T>** to_n) = 0;
	virtual void fixup_siblings_extents ();
	
	span_node<T>* parent;
	offset_type offset_start;
	
	bi::list_member_hook<> list_member;
	
	typedef bi::member_hook < node<T>,
														bi::list_member_hook<>,
														&node::list_member >  list_member_type;
};


template <typename T>
class span_node : public node<T>
{
public:
	friend class node<T>;
	typedef bi::list < node<T>, typename node<T>::list_member_type > child_list;
		
	span_node() : node<T>(), offset_end() {}
	span_node(span_node<T>* p) : node<T>(p), offset_end() {}
	virtual ~span_node() { children.clear_and_dispose(node_disposer<T>());  }
		
	int get_num_children() const { return children.size(); }
	iterator<T> at (int pos);
	offset_type size() const { return offset_end - node<T>::offset_start; }
	void set_size (offset_type ofs) { offset_end = node<T>::offset_start + ofs; }
	int insert (const iterator<T>& it, const T* strdata, int length);
	int append (const T* strdata, int length);
	void remove (int from, int to, memory_node<T>** from_n, memory_node<T>** to_n);
	std::ostream& printTo (std::ostream& os) const;
	std::ostream& dot (std::ostream& os) const;
	
	void fixup_my_extents ();
	void fixup_child_extents ();
	void fixup_ancestors_extents ();
		
	void rebalance_after_insert (bool full_height);
	static void rebalance_after_remove (span_node<T>* from, span_node<T>* to, span_node<T>** subroot);
	static void rebalance_upward (span_node<T>* node);
	
	child_list  children;
	offset_type offset_end;
		
};


template<typename T>
class memory_node_metadata {
public:
	memory_node_metadata() : siz(0), next(nullptr) {}
	offset_type  siz;
	//memory_node* prev;
	memory_node<T>* next;
};


/**
 * Refers to some contiguous range of memory.
 */
template <typename T>
class memory_node : public node<T>, public memory_node_metadata<T>
{
public:
	friend class treebuffer::tree_buffer<T>;
	friend class span_node<T>;
	
	static constexpr int metadata_size() {
		return sizeof(node<T>) + sizeof(memory_node_metadata<T>);
	}
	static const int capacity = (MEMORY_NODE_CAPACITY - metadata_size()) / sizeof(T);
	static_assert (capacity > 0, "Capacity is too small");
		
public:
	memory_node () {}
	memory_node (span_node<T>* p) : node<T>(p) {}
	virtual ~memory_node () { }

	iterator<T> at (int pos);
	offset_type size() const { return this->siz; } 
	void set_size (offset_type ofs) { this->siz = ofs; }
	int insert (const iterator<T>& it, const T* strdata, int length);
	int append (const T* strdata, int length);
	void remove (int from, int to, memory_node<T>** from_n, memory_node<T>** to_n);
	std::ostream& printTo (std::ostream& os) const;
	std::ostream& dot (std::ostream& os) const;
 		
	int raw_insert (const iterator<T>& it, const T* strdata, int length, T* carry_data, int* carry_length);
	int raw_prepend (const T* strdata, int length) { return raw_insert(iterator<T>{this,0}, strdata, length, nullptr, nullptr); }
	int raw_append (const T* strdata, int length) { return raw_insert(iterator<T>{this,this->siz}, strdata, length, nullptr, nullptr); }
		
	T  buf[capacity];
};


template <typename T>
iterator<T> span_node<T>::at (int pos)
{
	if (pos > this->size()) {
		throw std::range_error ("pos > siz");
	}
	typename span_node<T>::child_list::iterator last;
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
			return iterator<T>{nullptr,0,false};
		}
	}
	return last->at(pos - last->offset_start);
}


template <typename T>
iterator<T> memory_node<T>::at (int pos)
{
	if (pos > this->siz) {
		std::cerr << "pos: " << pos << " siz: " << this->siz << std::endl << std::flush;
		throw std::range_error ("pos > siz");
	}
	return { this, pos, true };
}


/**
 * Precondition: at.mnode is a child of this span_node.
 */
template <typename T>
int span_node<T>::insert (const iterator<T>& at, const T* strdata, int length)
{
	const int capacity = memory_node<T>::capacity;
	auto from = at->mnode;
	
	assert(from->parent == this);
	
	// Determine how many characters we can fit into the memory_node pointed to by 'at'.
	int carry_length = from->siz - at->offset;
	T* carry_data = (T*) alloca(sizeof(T) * carry_length);
	int first_segment_length = std::min(length,capacity - at->offset);
	int second_segment_length = length - first_segment_length;
	int remaining = second_segment_length;
		
	// Insert these first characters in 'at'
	from->raw_insert(at, strdata, first_segment_length, carry_data, &carry_length);
		
	// Treat the remainder of strdata using a new pointer, 'data', with 'remaining_length'.
	const T* data = strdata + first_segment_length;
	memory_node<T> *last = from,	*to = from->next, *m = from;
	bool same_parent = to && (to->parent == from->parent);
		
	if (to && (to->siz + carry_length <= capacity)) {
		to->insert(iterator<T>{to,0}, carry_data, carry_length);
		carry_length = 0;
	}
	  
	while (remaining > 0) {
			
		int amt = 0;
			
		m = new memory_node<T>(this);
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
			m = new memory_node<T>(this);
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
	last->next = to;
	
	m->fixup_siblings_extents();
	
	// update forward ancestor extents
	fixup_ancestors_extents();
	
	// rebalance the tree
	this->rebalance_after_insert(false);

	return length;	
}


template <typename T>
int memory_node<T>::insert (const iterator<T>& at_, const T* strdata, int length)
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
int span_node<T>::append (const T* strdata, int length)
{
	assert(this->parent == nullptr || children.size() > 0);
	int r = 0;
	if (children.size() == 0) {
		memory_node<T>* m = new memory_node<T>(this);
		children.push_back(*m);
		r += m->append(strdata,length);
	} else {
		r += children.back().append(strdata,length);		
	}

	return r;
}


template <typename T>
int memory_node<T>::append (const T* strdata, int length)
{
	span_node<T>* parent = this->parent;
	memory_node<T>* m = this;
	memory_node<T>* next = this->next;

	int r = raw_append(strdata,length);
	length -= r;
	strdata += r;

	while (length > 0) {
		memory_node<T>* sib = new memory_node<T>(parent);
		parent->children.push_back(*sib);
		sib->offset_start = this->offset_start + this->size();
		m->next = sib;
		r += sib->append(strdata,length);
		length -= r;
		strdata += r;
		m = sib;
	}
	m->next = next;
	this->fixup_siblings_extents();
	parent->rebalance_after_insert(false);
	parent->fixup_ancestors_extents();
	
	return r;
}



template <typename T>
void span_node<T>::rebalance_after_insert (bool full_height)
{
	if (this->children.size() > SPAN_NODE_FANOUT) {
	
		int c = SPAN_NODE_FANOUT;
		int nc = this->children.size();
		
		if (this->parent == nullptr) {
			// special case: root pivot
			span_node<T>* new_root = new span_node<T>;
			this->parent = new_root;
			new_root->children.push_back(*this);
		}
		
		int movn = (this->children.size() < 6) ? 2 : c;
		
		// add new siblings to the current span_node, then transfer children to those siblings
		while (children.size() > c) {
		
			span_node<T>* sib = new span_node<T>(this->parent);
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
			offset_end -= sz;
			sib->set_size(sz);
			
			// fixup offset_start and size in new sibling's children
			sib->fixup_child_extents();		
			
			movn = (this->children.size() < 6) ? 2 : c;
		}
		
		// fixup offset_start in all created siblings
		this->fixup_siblings_extents();
		
	} else if (!full_height) {
		return; // end the recursion if we're not doing the full height
	}
	
	// now there may be too many siblings, so rebalance up the tree
	if (this->parent != nullptr) {
		this->parent->rebalance_after_insert(full_height);
	}
	
}


template <typename T>
int memory_node<T>::raw_insert (const iterator<T>& at, const T* strdata, int length, T* carry_data, int* carry_length)
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
void span_node<T>::remove (int from, int to, memory_node<T>** from_n, memory_node<T>** to_n)
{
	// fast case: [from,to) spans this entire node, so remove all children
	if (from <= this->offset_start && to >= this->offset_end) {
		children.clear_and_dispose(node_disposer<T>());
	}
	
	// more general case
	auto it = children.begin();
	auto from_it = children.end();
	auto to_it = children.end();
	bool in_overlap = false;
	while (it != children.end()) {
		int a = it->offset_start;
		int b = a + it->size();
		int c = from;
		int d = to;
		if (overlaps(a,b,c,d)) {
			if (!in_overlap) {
				from_it = it;
			}
			to_it = it;
			in_overlap = true;
			it->remove(c-a,d-a,from_n,to_n);
			if (it->size() == 0) {
				typename child_list::const_iterator cit = this->children.iterator_to(*it);
				it++;
				children.erase(cit);
				continue;
			} 
		} else {
			in_overlap = false;
		}
		it++;
	}
}

template <typename T>
void memory_node<T>::remove (int from, int to, memory_node<T>** from_n, memory_node<T>** to_n)
{
	if (from >= 0 && from < this->siz) *from_n = this;
	if (to > 0 && to <= this->siz) *to_n = this;
	from = std::max(0, from);
	to = std::min(to, this->siz);
	std::copy(buf + to, buf + this->siz, buf + from);
	this->siz -= (to - from);
}


template <typename T>
void span_node<T>::rebalance_after_remove (span_node<T>* from, span_node<T>* to, span_node<T>** subroot)
{
	if (from == to) {
		*subroot = from;
		return;
	}
	rebalance_after_remove(from->parent,to->parent, subroot);
	
	// cases:
	int fn = from->children.size();
	int tn = to->children.size();
	
	if (fn == 0 || tn == 0) {
		if (fn == 0) {
			typename child_list::const_iterator it = from->parent->children.iterator_to(*from);
			from->parent->children.erase_and_dispose(it, node_disposer<T>());
		}
		if (tn == 0) {
			typename child_list::const_iterator it = to->parent->children.iterator_to(*to);
			to->parent->children.erase_and_dispose(it, node_disposer<T>());			
		}
	} else if (fn == 1 && tn > 2) {
		auto& n = to->children.front();
		to->children.pop_front();
		from->children.push_back(n);
		n.parent = from;
	} else if (tn == 1 && fn > 2) {
		auto& n = from->children.back();
		from->children.pop_back();
		to->children.push_front(n);
		n.parent = to;
	} else if (tn == 1) {
		for (auto& n : to->children) {
			n.parent = from;
		}
		from->children.splice(from->children.end(),to->children); // this may trigger the need for rebalance_after_insert
		// Delay this until later! This will happen during rebalance_after insert
		//from->fixup_child_extents();
		//from->fixup_my_extents();
	} else if (fn == 1) {
		for (auto& n: from->children) {
			n.parent = to;
		}
		to->children.splice(to->children.begin(),from->children); // this may trigger the need for rebalance_after_insert
		// Delay this until later! This will happen during rebalance_after insert
		//to->fixup_child_extents();
		//to->fixup_my_extents();
	}
}


template <typename T>
void span_node<T>::rebalance_upward (span_node<T>* node)
{
	span_node<T>* parent = node->parent;
	
	if (node->children.size() == 1) {
		// if we're at the root, then leave -- we will root pivot later
		if (parent == nullptr) {
			return;
		}
		
		// merge rightward: node->parent is guaranteed to have at least one other child on the right
		typename span_node<T>::child_list::iterator it = node->parent->children.iterator_to(*node);
		it++;
		assert(it != parent->children.end());
		span_node<T>& sib = dynamic_cast<span_node<T>&>(*it);
		auto& onlychild = node->children.front();
		onlychild.parent = &sib;
		node->children.pop_front();
		sib.children.push_front(onlychild);
		
		// purge this node
		typename span_node<T>::child_list::const_iterator cit = parent->children.iterator_to(*node);
		parent->children.erase_and_dispose(cit, node_disposer<T>());
	}
	
	if (parent) {
		rebalance_upward(parent);
	}
}


template <typename T>
void node<T>::fixup_siblings_extents()
{
	if (parent == nullptr) return;
	int ofs = offset_start + size();
	for (auto &it = ++parent->children.iterator_to(*this); it != parent->children.end(); it++) {
		int saved_size = it->size();
		it->offset_start = ofs;
		it->set_size(saved_size);
		ofs += saved_size;
	}
}

template <typename T>
void span_node<T>::fixup_my_extents ()
{
	int offset_end = this->offset_start;
	for (auto &it : children) {
		node<T>& n = it;
		offset_end += n.size();
	}
	this->offset_end = offset_end;
}

template <typename T>
void span_node<T>::fixup_child_extents ()
{
	if (children.empty()) return;
	children.front().fixup_siblings_extents();
}


template <typename T>
void span_node<T>::fixup_ancestors_extents ()
{
	span_node<T>* ma = this;
	ma->fixup_my_extents();
	while (ma) {
		span_node<T>* grandma = ma->parent;
		if (grandma != nullptr) {
 			ma->fixup_siblings_extents();
			grandma->fixup_my_extents();
		}
		ma = grandma;
	}
}

} // namespace treebuffer::detail
