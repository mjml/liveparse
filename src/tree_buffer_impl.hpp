#pragma once


namespace treebuffer {

using namespace treebuffer::detail;

template <typename T>
tree_buffer<T>::tree_buffer() : root(nullptr)
{
}

template <typename T>
tree_buffer<T>::~tree_buffer()
{
	if (root) { delete root; }
}

template <typename T>
int tree_buffer<T>::size () const
{
	if (!root) return 0;
	return root->size();
}

template <typename T>
iterator<T> tree_buffer<T>::begin ()
{
	return at(0);
}
	
template <typename T>
iterator<T> tree_buffer<T>::end ()
{
	return iterator<T>{nullptr,0,false};
}
	

template <typename T>
iterator<T> tree_buffer<T>::at (int pos)
{
	if (!root) { return iterator<T>::end(); }
	if (pos == root->size()) { return iterator<T>::end(); }
	return root->at(pos);
}


template <typename T>
void tree_buffer<T>::insert (int pos, const T* strdata, int length)
{
	if (root == nullptr) {
		root = new span_node<T>();
		auto mnode = new memory_node<T>(root);
		root->children.push_back(*mnode);
	}
	auto it = at(pos);
	insert(it, strdata, length);
}


template <typename T>
void tree_buffer<T>::insert (const iterator<T>& at, const T* strdata, int length)
{
	if (iterator<T>::is_end(at)) {
		root->append(strdata,length);
		return;
	}
	
	if (memory_node<T>::capacity >= at->mnode->siz + length) {
		at->mnode->insert(at, strdata, length);
	} else {
		at->mnode->parent->insert(at, strdata, length);
	}
	
	// last step: adjust to possibility of pivoted root
	while (root->parent != nullptr) {
		root = root->parent;
	}
	
}

template <typename T>
void tree_buffer<T>::append (const T* strdata, int length)
{
	if (!root) {
		root = new span_node<T>();
	}
	int r = root->append(strdata,length);
	
	assert(r == length);
	
	// last step: adjust to possibility of pivoted root
	while (root->parent != nullptr) {
		root = root->parent;
	}
	
}


template <typename T>
void tree_buffer<T>::remove (int from, int to)
{
	root->remove(from,to);
}


template <typename T>
void tree_buffer<T>::remove (iterator<T>& from, iterator<T>& to)
{
	/* just use the iterators to obtain absolute positions, then remove using the absolute positions  */
	remove(pos(from), pos(to));
}


}

