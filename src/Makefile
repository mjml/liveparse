
@default: @tests

CXXFLAGS=-g -std=c++17
LDFLAGS=-g

@tests:
	make -C tests

%.obj: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.c: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) -rf *.o *.obj liveparse
	$(RM) -rf tests/test?? tests/test?
	make -C tests clean
