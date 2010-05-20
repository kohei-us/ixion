
#include <cstdlib>
#include <iostream>
#include <pthread.h>


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

void* thread_routine(void* arg)
{
    cout << "thread routine" << endl;
    return arg;
}

int main()
{
    StackPrinter __stack_printer__("::main");
    pthread_t thread_id;
    void* thread_result;

    int status = pthread_create(&thread_id, NULL, thread_routine, NULL);
    if (!status)
        return EXIT_FAILURE;

    status = pthread_join(thread_id, &thread_result);
    if (!status)
        return EXIT_FAILURE;

    return thread_result == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
