#include "test_global.hpp"

namespace ixion { namespace test {

stack_printer::stack_printer(std::string msg) :
    m_msg(std::move(msg))
{
    std::cerr << m_msg << ": --begin" << std::endl;
    m_start_time = get_time();
}

stack_printer::~stack_printer()
{
    double end_time = get_time();
    std::cerr << m_msg << ": --end (duration: " << (end_time-m_start_time) << " sec)" << std::endl;
}

double stack_printer::get_time() const
{
    double v = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    return v / 1000.0;
}

}}
