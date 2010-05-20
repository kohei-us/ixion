
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread.hpp>

#include <stdio.h>
#include <string>
#include <sys/time.h>

namespace {

class StackPrinter
{
public:
    explicit StackPrinter(const char* msg) :
        msMsg(msg)
    {
        fprintf(stdout, "%s: --begin\n", msMsg.c_str());
        mfStartTime = getTime();
    }

    ~StackPrinter()
    {
        double fEndTime = getTime();
        fprintf(stdout, "%s: --end (duration: %g sec)\n", msMsg.c_str(), (fEndTime-mfStartTime));
    }

    void printTime(int line) const
    {
        double fEndTime = getTime();
        fprintf(stdout, "%s: --(%d) (duration: %g sec)\n", msMsg.c_str(), line, (fEndTime-mfStartTime));
    }

private:
    double getTime() const
    {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }

    ::std::string msMsg;
    double mfStartTime;
};

}

using namespace std;

void thread_routine()
{
    StackPrinter __stack_printer__("::thread_routine");
    sleep(1);
}

class formula_cell
{
public:
    formula_cell() : mp_thread(NULL) {}

    void interpret_thread()
    {
        mp_thread = new ::boost::thread(::boost::bind(&formula_cell::interpret, this));
    }

    void interpret()
    {
        StackPrinter __stack_printer__("formula_cell::interpret");
        sleep(1);
    }

    void interpret_join()
    {
        mp_thread->join();
        delete mp_thread;
        mp_thread = NULL;
    }
private:
    ::boost::thread* mp_thread;
};

int main()
{
    StackPrinter __stack_printer__("::main");
    ::boost::ptr_vector<formula_cell> cells;
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);

    ::boost::ptr_vector<formula_cell>::iterator itr, itr_beg = cells.begin(), itr_end = cells.end();
    for (itr = itr_beg; itr != itr_end; ++itr)
        itr->interpret_thread();

    for (itr = itr_beg; itr != itr_end; ++itr)
        itr->interpret_join();
}
