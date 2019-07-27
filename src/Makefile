CXXFLAGS=-g -std=c++17
LDFLAGS=-g

EDITOR_HDRS=$(shell ls *.hpp)
EDITOR_SRCS=$(shell ls *.cpp)
EDITOR_OBJS=$(patsubst %.cpp,%.obj,$(EDITOR_SRCS))


@default: @tests

@tests:
	make -C tests

%.obj: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c  $<

%.c: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) -rf *.o *.obj liveparse
	$(RM) -rf tests/test?? tests/test?
	make -C tests clean

liveparse: $(EDITOR_OBJS) $(EDITOR_HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(EDITOR_OBJS)

test:
	echo $(EDITOR_SRCS)
	echo $(EDITOR_OBJS)

