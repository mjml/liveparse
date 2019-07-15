#pragma once


namespace util {

using namespace util::detail;

template <typename T>
skiparraylist<T>::skiparraylist() : root(nullptr)
{
}

template <typename T>
skiparraylist<T>::~skiparraylist()
{
	if (root) { delete root; }
}

template <typename T>
int skiparraylist<T>::size () const
{
	if (!root) return 0;
	return root->size();
}

template <typename T>
iterator<T> skiparraylist<T>::begin ()
{
	return at(0);
}
	
template <typename T>
iterator<T> skiparraylist<T>::end ()
{
	return iterator<T>{nullptr,0,false};
}
	

template <typename T>
iterator<T> skiparraylist<T>::at (int pos)
{
	if (!root) { return iterator<T>::end(); }
	if (pos == root->size()) { return iterator<T>::end(); }
	return root->at(pos);
}


template <typename T>
void skiparraylist<T>::insert (int pos, const T* strdata, int length)
{
	if (root == nullptr) {
		assert(pos == 0);
		root = new inner<T>();
		auto l = new leaf<T>();
		root->push_back(l);
	}
	auto it = at(pos);
	insert(it, strdata, length);
}


template <typename T>
void skiparraylist<T>::insert (const iterator<T>& it, const T* strdata, int length)
{
	if (iterator<T>::is_end(it)) {
		root->append(strdata,length);
		return;
	}
	
	it.leaf->parent->insert(it, strdata, length);
	
	while (root->parent != nullptr) {
		root = root->parent;
	}
	
	#ifdef DEBUG_UTIL
	root->check();
	#endif

}

template <typename T>
void skiparraylist<T>::append (const T* strdata, int length)
{
	if (!root) {
		root = new inner<T>();
	}
	int r = root->append(strdata,length);
	
	assert(r == length);
	
	// last step: adjust to possibility of pivoted root
	while (root->parent != nullptr) {
		root = root->parent;
	}

	#ifdef DEBUG_UTIL
	root->check();
	#endif
	
}


template <typename T>
void skiparraylist<T>::remove (int from, int to)
{
	if (to == from) { return; }
	if (to < from)  { throw std::domain_error("Cannot remove with to < from"); }
	
	if (to - from == root->size()) {
		if (root) delete root;
		root = nullptr;
		return;
	}

	subtree_context<T> ctx;
	
	root->remove(from,to,&ctx);
	
	inner<T>::rebalance_after_remove(ctx.from, ctx.to, &ctx);

	// root compaction
	while (root->num_children() == 1) {
		auto oldroot = root;
		auto inner_child = dynamic_cast<inner<T>*>(root->child);
		if (inner_child) {
			root = inner_child;
			root->parent = nullptr;
			oldroot->child = nullptr;
			delete oldroot;
		} else {
			break;
		}
	}
	
	#ifdef DEBUG_UTIL
	if (root) {
		root->check();
	}
	#endif
	
}


template <typename T>
void skiparraylist<T>::remove (iterator<T>& from, iterator<T>& to)
{
	/* just use the iterators to obtain absolute positions, then remove using the absolute positions  */
	remove(pos(from), pos(to));
}


}

