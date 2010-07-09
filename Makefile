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

EXEC=ixion-parser ixion-sorter
OBJDIR=./obj
SRCDIR=./src
INCDIR=./inc

CPPFLAGS=-I$(INCDIR) -D_REENTRANT -O2 -g -Wall -std=c++0x
LDFLAGS=-lboost_thread -lboost_program_options

HEADERS= \
	$(INCDIR)/cell.hpp \
	$(INCDIR)/cell_queue_manager.hpp \
	$(INCDIR)/depends_tracker.hpp \
	$(INCDIR)/depth_first_search.hpp \
	$(INCDIR)/formula_functions.hpp \
	$(INCDIR)/formula_lexer.hpp \
	$(INCDIR)/formula_parser.hpp \
	$(INCDIR)/formula_result.hpp \
	$(INCDIR)/formula_tokens.hpp \
	$(INCDIR)/formula_interpreter.hpp \
	$(INCDIR)/global.hpp \
	$(INCDIR)/mem_str_buf.hpp \
	$(INCDIR)/model_parser.hpp \
	$(INCDIR)/lexer_tokens.hpp

OBJFILES= \
	$(OBJDIR)/ixion_parser.o \
	$(OBJDIR)/cell.o \
	$(OBJDIR)/cell_queue_manager.o \
	$(OBJDIR)/lexer_tokens.o \
	$(OBJDIR)/global.o \
	$(OBJDIR)/formula_functions.o \
	$(OBJDIR)/formula_lexer.o \
	$(OBJDIR)/formula_parser.o \
	$(OBJDIR)/formula_result.o \
	$(OBJDIR)/formula_tokens.o \
	$(OBJDIR)/formula_interpreter.o \
	$(OBJDIR)/mem_str_buf.o \
	$(OBJDIR)/model_parser.o \
	$(OBJDIR)/depends_tracker.o \
	$(OBJDIR)/depth_first_search.o

OBJ_SORTER= \
	$(OBJDIR)/ixion_sorter.o

DEPENDS= \
	$(HEADERS)

all: $(EXEC)

pre:
	mkdir $(OBJDIR) 2>/dev/null || /bin/true

$(OBJDIR)/ixion_parser.o: $(SRCDIR)/ixion_parser.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/ixion_parser.cpp

$(OBJDIR)/ixion_sorter.o: $(SRCDIR)/ixion_sorter.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/ixion_sorter.cpp

$(OBJDIR)/cell.o: $(SRCDIR)/cell.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/cell.cpp

$(OBJDIR)/cell_queue_manager.o: $(SRCDIR)/cell_queue_manager.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/cell_queue_manager.cpp

$(OBJDIR)/lexer_tokens.o: $(SRCDIR)/lexer_tokens.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/lexer_tokens.cpp

$(OBJDIR)/global.o: $(SRCDIR)/global.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/global.cpp

$(OBJDIR)/formula_functions.o: $(SRCDIR)/formula_functions.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_functions.cpp

$(OBJDIR)/formula_lexer.o: $(SRCDIR)/formula_lexer.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_lexer.cpp

$(OBJDIR)/formula_parser.o: $(SRCDIR)/formula_parser.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_parser.cpp

$(OBJDIR)/formula_result.o: $(SRCDIR)/formula_result.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_result.cpp

$(OBJDIR)/formula_tokens.o: $(SRCDIR)/formula_tokens.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_tokens.cpp

$(OBJDIR)/formula_interpreter.o: $(SRCDIR)/formula_interpreter.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_interpreter.cpp

$(OBJDIR)/mem_str_buf.o: $(SRCDIR)/mem_str_buf.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/mem_str_buf.cpp

$(OBJDIR)/model_parser.o: $(SRCDIR)/model_parser.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/model_parser.cpp

$(OBJDIR)/depends_tracker.o: $(SRCDIR)/depends_tracker.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/depends_tracker.cpp

$(OBJDIR)/depth_first_search.o: $(SRCDIR)/depth_first_search.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/depth_first_search.cpp

ixion-parser: pre $(OBJFILES)
	$(CXX) $(LDFLAGS) $(OBJFILES) -o $@

ixion-sorter: pre $(OBJ_SORTER)
	$(CXX) $(LDFLAGS) $(OBJ_SORTER) -o $@

test: $(EXEC)
	./$(EXEC) -d $(OBJDIR)/simple-arithmetic.dot ./test/*.txt

test.expr: $(EXEC)
	./$(EXEC) -d $(OBJDIR)/expression-test.dot ./test/expression-test.txt

test.parallel: $(EXEC)
	./$(EXEC) -d $(OBJDIR)/05-function-parallel.dot ./test/05-function-parallel.txt

clean:
	rm -rf $(OBJDIR) || /bin/true
	rm -f $(EXEC) pre || /bin/true

