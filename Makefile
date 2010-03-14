
EXEC=inputparser
OBJDIR=./obj
SRCDIR=./src
INCDIR=./inc

CPPFLAGS=-I$(INCDIR) -g -Wall
LDFLAGS=

HEADERS= \
	$(INCDIR)/inputparser.hpp

OBJFILES= \
	$(OBJDIR)/main.o \
	$(OBJDIR)/cell.o \
	$(OBJDIR)/tokens.o \
	$(OBJDIR)/formula_lexer.o \
	$(OBJDIR)/inputparser.o

all: $(EXEC)

pre:
	mkdir $(OBJDIR) 2>/dev/null || /bin/true

$(OBJDIR)/main.o: $(SRCDIR)/main.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/main.cpp

$(OBJDIR)/cell.o: $(SRCDIR)/cell.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/cell.cpp

$(OBJDIR)/tokens.o: $(SRCDIR)/tokens.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/tokens.cpp

$(OBJDIR)/formula_lexer.o: $(SRCDIR)/formula_lexer.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/formula_lexer.cpp

$(OBJDIR)/inputparser.o: $(SRCDIR)/inputparser.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/inputparser.cpp

$(EXEC): pre $(OBJFILES)
	$(CXX) $(LDFLAGS) $(OBJFILES) -o $(EXEC)

test: $(EXEC)
	./$(EXEC) ./test/simple-model.txt

clean:
	rm -rf $(OBJDIR)
	rm $(EXEC)

