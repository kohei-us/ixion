
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
	$(OBJDIR)/inputparser.o

all: $(EXEC)

pre:
	mkdir $(OBJDIR) 2>/dev/null || /bin/true

$(OBJDIR)/main.o: $(SRCDIR)/main.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/main.cpp

$(OBJDIR)/inputparser.o: $(SRCDIR)/inputparser.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/inputparser.cpp

$(EXEC): pre $(OBJFILES)
	$(CXX) $(LDFLAGS) $(OBJFILES) -o $(EXEC)

test: $(EXEC)
	./$(EXEC)

clean:
	rm -rf $(OBJDIR)
	rm $(EXEC)

