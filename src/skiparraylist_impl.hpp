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
		root = new node<T>();
		auto l = new leaf<T>(root);
		root->child = l;
		l->parent = root;
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
	
	if (leaf<T>::capacity >= at->mnode->siz + length) {
		it->leaf->insert(at, strdata, length);
	} else {
		it->leaf->parent->insert(at, strdata, length);
	}
	
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
		root = new node<T>();
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
	
	int initial_size = root->size();
	subtree_context<T> ctx;
	root->remove(from,to,&ctx);
	
	if (ctx.from || ctx.to) {
		node<T>::rebalance_after_remove(ctx.from,ctx.to,&ctx);
	}
	
	node<T>::rebalance_upward(ctx.subroot);
	node<T>* bottom = nullptr;
	
	if (ctx.from) {
		ctx.from->rebalance_after_insert(true);
	}
	if (ctx.to) {
		ctx.to->rebalance_after_insert(true);
	}
	
	if (to - from == initial_size) {
		if (root) delete root;
		root = nullptr;
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

