#pragma once

#include <iostream>

#define GRAPHVIZ_ID_MASK 0xffffff



namespace util {

using namespace util::detail;
using namespace std;


template <typename T>
std::ostream& operator<< (std::ostream& os, const util::detail::node<T>& n)
{
	return n.printTo(os);
}


template<typename U>
std::ostream& operator<< (std::ostream& os, skiparraylist<U>& b)
{
	return os << *(b.root);
}


template<typename T>
std::ostream& skiparraylist<T>::dot (std::ostream& os) const
{
	os << "digraph { " << endl;
	os << "node [ fontname=\"Liberation Sans\" ];" << endl;
	os << "rankdir=BT;" << endl;
	root->dot(os);
	os << "}" << endl;
	return os << dec;
}

template<typename T>
std::ostream& span_node<T>::dot (std::ostream& os) const
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
			const node<T>& n = dynamic_cast<const node<T>&> (it);
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


template<typename T>
std::ostream& memory_node<T>::dot (std::ostream& os) const
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


template<typename T>
std::ostream& node<T>::printTo (std::ostream& os) const
{	
	for (auto it = this->children.cbegin(); it != this->children.cend(); it++) {
	  os << *it;
	}
	return os;
}


template<typename T>
std::ostream& leaf<T>::printTo (std::ostream& os) const
{
	for (int i=0; i < this->siz; i++) {
		os << this->buf[i];
	}
	return os;
}

}
