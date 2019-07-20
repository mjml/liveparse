#pragma once

namespace util
{

template<typename T, int Size>
void shmfixedsegment<T,Size>::init_objects ()
{
	auto* cur = reinterpret_cast<shmobj<T>*>(data);
	free_meta* last = nullptr;
	
	for (int i=0; i < capacity(); i++) {
		cur->meta->prev = last;
		if (last) {
			last->next = &cur->meta;
		}
		last = cur->meta;
		cur++;
	}
	last->next = nullptr;
}




}
