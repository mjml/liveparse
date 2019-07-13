#pragma once

#include <iostream>

#define GRAPHVIZ_ID_MASK 0xffffff

static int level = 0;

namespace util {

using namespace util::detail;
using namespace std;


template <typename T>
std::ostream& operator<< (std::ostream& os, const node<T>& n)
{
	return n.printTo(os);
}


template<typename T>
std::ostream& operator<< (std::ostream& os, skiparraylist<T>& b)
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
std::ostream& inner<T>::dot (std::ostream& os) const
{
	os << "inner" << std::hex <<  ((unsigned long)(this) & GRAPHVIZ_ID_MASK)  << "[";
	os << "shape=folder, color=grey, ";
	os << "label=\"" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << endl;
	os << "start=" << std::dec << this->offset << ", ";
	os << "end=" << std::dec << this->offset + this->siz << ", ";
	os << "size=" << std::dec << this->siz << "\"];" << endl;
	
	for (auto n = child; n && n->parent == this; n = n->next()) {
		n->dot(os);
	}
	
	if (num_children() > 0) {
		os << "{ rank=same";
		for (auto n = child; n && n->parent == this; n = n->next()) {
			os << "; node" << std::hex <<  ((unsigned long)(n) & GRAPHVIZ_ID_MASK);
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
std::ostream& leaf<T>::dot (std::ostream& os) const
{
	os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << "[";
	os << "shape=box3d, ";
	os << "rank=min, ";
	os << "label=" << "\"" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << endl;
	os << "start=" << std::dec << this->offset << endl;
	os << "size=" << std::dec << this->siz <<"\"];"<< endl;
	
	if (this->parent) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->parent) & GRAPHVIZ_ID_MASK) << " ;" << endl;
	}

	if (this->next()) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->next()) & GRAPHVIZ_ID_MASK);
		os << "[color=red];" << endl;
		
	}
	return os << std::dec;
}


template<typename T>
std::ostream& inner<T>::printTo (std::ostream& os) const
{	
	for (auto n = child; n && n->parent == this; n = n->next()) {
	  n->printTo(os);
	}
	return os;
}


template<typename T>
std::ostream& leaf<T>::printTo (std::ostream& os) const
{
	for (int i=0; i < this->siz; i++) {
		os << this->data[i];
	}
	return os;
}

}
