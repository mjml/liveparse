
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
	if (b.root) {
		return os << *(b.root);
	} else {
		return os;
	}
}


template<typename T>
std::ostream& skiparraylist<T>::dot (std::ostream& os) const
{
	os << "digraph { " << endl;
	os << "node [ fontname=\"Liberation Sans\" ];" << endl;
	os << "rankdir=TB;" << endl;

	node<T>* n = root;
	do {
		int ofs = 0;
		auto m = n;
		do {
			m->dot(os,ofs);
			ofs += m->size();
			m = m->next();
		} while (m);

		os << "{ rank=same;" << endl;
		m = n;
		do {
			os << "node" << std::hex << ((unsigned long)(m) & GRAPHVIZ_ID_MASK) << ";" << endl;
			m = m->next();
		} while (m);
		os << "}" << endl;
		
		auto p = dynamic_cast<inner<T>*>(n);
		if (p) {
			n = p->child;
		} else {
			n = nullptr;
		}
	} while (n);

	os << "}" << endl;
	return os << dec;
}

template<typename T>
std::ostream& inner<T>::dot (std::ostream& os, offset_type ofs) const
{
	os << "node" << std::hex <<  ((unsigned long)(this) & GRAPHVIZ_ID_MASK)  << "[";
	os << "shape=folder, color=grey, ";
	os << "label=\"" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << endl;
	os << "start=" << std::dec << this->offset << ", ";
	os << "end=" << std::dec << this->offset + this->siz << ", ";
	os << "size=" << std::dec << this->siz << ", ";
	os << "abs=" << std::dec << ofs << "\"];" << endl;

	if (this->parent) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->parent) & GRAPHVIZ_ID_MASK) << " ;" << endl;
	}
	
	if (this->child) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->child) & GRAPHVIZ_ID_MASK);
		os << "[color=red];" << endl;
	}
	
	if (this->next()) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->next()) & GRAPHVIZ_ID_MASK);
		os << "[color=green];" << endl;
	}
	
	if (this->prev()) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->prev()) & GRAPHVIZ_ID_MASK);
		os << "[color=blue];" << endl;
	}
	return os << std::dec;
}


template<typename T>
std::ostream& leaf<T>::dot (std::ostream& os, offset_type ofs) const
{
	os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << "[";
	os << "shape=box3d, ";
	os << "rank=min, ";
	os << "label=" << "\"" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK) << endl;
	os << "start=" << std::dec << this->offset << endl;
	os << "size=" << std::dec << this->siz << endl;
	os << "abs=" << std::dec << ofs <<"\"];"<< endl;
	
	if (this->parent) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->parent) & GRAPHVIZ_ID_MASK);
		os << "[color=black];" << endl;
	}
	
	if (this->next()) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->next()) & GRAPHVIZ_ID_MASK);
		os << "[color=green];" << endl;
	}

	if (this->prev()) {
		os << "node" << std::hex << ((unsigned long)(this) & GRAPHVIZ_ID_MASK)
			 << " -> node" << std::hex << ((unsigned long)(this->prev()) & GRAPHVIZ_ID_MASK);
		os << "[color=blue];" << endl;
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
