SRCDIR=.
OBJDIR=.

CPPFLAGS=-Wall -D_REENTRANT
LDFLAGS=-lboost_thread

HEADERS=

DEPENDS= \
	$(HEADERS)

OBJFILES= \
	thread_test.o

all: thread_test
	./thread_test

$(OBJDIR)/thread_test.o: $(SRCDIR)/thread_test.cpp $(DEPENDS)
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRCDIR)/thread_test.cpp

thread_test: $(OBJFILES)
	$(CXX) $(LDFLAGS) -o $@ $(OBJFILES)

clean:
	rm -rf $(OBJFILES) thread_test

