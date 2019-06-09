
@default: @tests

CXXFLAGS=-g -std=c++17
LDFLAGS=-g

@tests: tests/test1 tests/test2

tests/%: tests/%.cpp 
	$(CXX) -I. $(LDFLAGS) -o $@ $<

%.obj: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.c: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) -rf *.o *.obj liveparse
	$(RM) -rf tests/test?? tests/test?
