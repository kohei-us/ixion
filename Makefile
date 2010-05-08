#************************************************************************
#
# Copyright (c) 2010 Kohei Yoshida
# 
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
#***********************************************************************

EXEC=ixion-parser
OBJDIR=./obj
SRCDIR=./src
INCDIR=./inc

CPPFLAGS=-I$(INCDIR) -O2 -g -Wall -std=c++0x
LDFLAGS=

HEADERS= \
	$(INCDIR)/cell.hpp \
	$(INCDIR)/depends_tracker.hpp \
	$(INCDIR)/depth_first_search.hpp \
	$(INCDIR)/formula_lexer.hpp \
	$(INCDIR)/formula_parser.hpp \
	$(INCDIR)/formula_tokens.hpp \
	$(INCDIR)/formula_interpreter.hpp \
	$(INCDIR)/global.hpp \
	$(INCDIR)/inputparser.hpp \
	$(INCDIR)/lexer_tokens.hpp

OBJFILES= \
	$(OBJDIR)/main.o \
	$(OBJDIR)/cell.o \
	$(OBJDIR)/lexer_tokens.o \
	$(OBJDIR)/global.o \
	$(OBJDIR)/formula_lexer.o \
	$(OBJDIR)/formula_parser.o \
	$(OBJDIR)/formula_tokens.o \
	$(OBJDIR)/formula_interpreter.o \
	$(OBJDIR)/inputparser.o \
	$(OBJDIR)/depends_tracker.o \
	$(OBJDIR)/depth_first_search.o

DEPENDS= \
	$(HEADERS)

all: $(EXEC)

pre:
	mkdir $(OBJDIR) 2>/dev/null || /bin/true

$(OBJDIR)/main.o: $(SRCDIR)/main.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/main.cpp

$(OBJDIR)/cell.o: $(SRCDIR)/cell.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/cell.cpp

$(OBJDIR)/lexer_tokens.o: $(SRCDIR)/lexer_tokens.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/lexer_tokens.cpp

$(OBJDIR)/global.o: $(SRCDIR)/global.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/global.cpp

$(OBJDIR)/formula_lexer.o: $(SRCDIR)/formula_lexer.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_lexer.cpp

$(OBJDIR)/formula_parser.o: $(SRCDIR)/formula_parser.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_parser.cpp

$(OBJDIR)/formula_tokens.o: $(SRCDIR)/formula_tokens.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_tokens.cpp

$(OBJDIR)/formula_interpreter.o: $(SRCDIR)/formula_interpreter.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_interpreter.cpp

$(OBJDIR)/inputparser.o: $(SRCDIR)/inputparser.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/inputparser.cpp

$(OBJDIR)/depends_tracker.o: $(SRCDIR)/depends_tracker.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/depends_tracker.cpp

$(OBJDIR)/depth_first_search.o: $(SRCDIR)/depth_first_search.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/depth_first_search.cpp

$(EXEC): pre $(OBJFILES)
	$(CXX) $(LDFLAGS) $(OBJFILES) -o $(EXEC)

test: $(EXEC)
	./$(EXEC) -d $(OBJDIR)/simple-arithmetic.dot ./test/*.txt

test.expr: $(EXEC)
	./$(EXEC) -d $(OBJDIR)/expression-test.dot ./test/expression-test.txt

clean:
	rm -rf $(OBJDIR)
	rm $(EXEC)

