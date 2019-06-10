
@default: @tests

CXXFLAGS=-g -std=c++17
LDFLAGS=-g

@tests: tests/test1 tests/test2 tests/test3 tests/test3a.svg tests/test3b.svg tree_buffer.hpp

tests/%: tests/%.cpp tree_buffer.hpp
	$(CXX) -I. $(LDFLAGS) -o $@ $<

tests/test3a.dot: tests/test3
	cd tests && ./test3

tests/test3b.dot: tests/test3
	cd tests && ./test3

%.svg: %.dot
	dot -Tsvg $< > $@

%.obj: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.c: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) -rf *.o *.obj liveparse
	$(RM) -rf tests/test?? tests/test?
