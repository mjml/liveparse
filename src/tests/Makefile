CXXFLAGS=-g -std=c++17
LDFLAGS=-g
PROJECT_HOME=../..
CREATE_TEST_MATRIX=$(PROJECT_HOME)/ext/testmatrix/create_test_matrix.pl6

@default: Makefile.matrix
	make -f Makefile.matrix

Makefile.matrix:
	$(CREATE_TEST_MATRIX)

%: %.cpp ../tree_buffer.hpp
	$(CXX) $(CXXFLAGS) -I.. $(LDFLAGS) -o $@ $<

%.svg: %.dot
	dot -Tsvg $< > $@

clean:
	$(RM) -rf test? *.svg *.dot
