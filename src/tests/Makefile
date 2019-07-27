CXXFLAGS=-g -std=c++17
LDFLAGS=-g
PROJECT_HOME=../..
CREATE_TEST_MATRIX=$(PROJECT_HOME)/ext/testmatrix/create_test_matrix.pl6
TEST_SOURCES=$(wildcard *.cpp)
PERL6=perl6

@default: @tests

@test: Makefile.matrix $(TEST_SOURCES)
	make -f Makefile.matrix @test

@tests: Makefile.matrix $(TEST_SOURCES)
	make -f Makefile.matrix @tests

Makefile.matrix: Makefile
	$(PERL6) -I $(PROJECT_HOME)/ext/testmatrix  $(CREATE_TEST_MATRIX) > Makefile.matrix

%.svg: %.dot
	dot -Tsvg $< > $@

@clean:
	$(RM) -rf *.svg *.dot
	$(RM) -rf *.obj *.o
	make -f Makefile.matrix @clean
	$(RM) -rf Makefile.matrix

