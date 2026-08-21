#include "test_global.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

namespace ixion { namespace test {

namespace {

std::vector<std::string> split_lines(std::string_view s)
{
    std::vector<std::string> lines;
    std::istringstream is{std::string{s}};
    std::string line;
    while (std::getline(is, line))
        lines.push_back(line);
    return lines;
}

} // anonymous namespace

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

bool check_expected_output(std::string_view output, const std::string& expected_path)
{
    const char* regen = std::getenv("IXION_TEST_REGENERATE");
    if (regen && *regen)
    {
        std::ofstream ofs{expected_path, std::ios::binary};
        if (!ofs)
        {
            std::cerr << "failed to write to '" << expected_path << "'" << std::endl;
            return false;
        }

        // Terminate the output with a line break to keep the file a
        // well-formed text file.
        ofs << output << '\n';
        std::cerr << "regenerated '" << expected_path << "'" << std::endl;
        return true;
    }

    std::ifstream ifs{expected_path, std::ios::binary};
    if (!ifs)
    {
        std::cerr << "failed to open '" << expected_path
            << "' (run with IXION_TEST_REGENERATE=1 to generate it)" << std::endl;
        return false;
    }

    std::ostringstream os;
    os << ifs.rdbuf();
    std::string expected = os.str();
    if (expected.ends_with('\n'))
        expected.pop_back();

    if (output == expected)
        return true;

    std::vector<std::string> output_lines = split_lines(output);
    std::vector<std::string> expected_lines = split_lines(expected);

    std::size_t n = std::min(output_lines.size(), expected_lines.size());
    std::size_t i = 0;
    while (i < n && output_lines[i] == expected_lines[i])
        ++i;

    std::cerr << "output differs from '" << expected_path << "' at line " << (i + 1) << std::endl;
    std::cerr << "  expected: " << (i < expected_lines.size() ? expected_lines[i] : "<none>") << std::endl;
    std::cerr << "    actual: " << (i < output_lines.size() ? output_lines[i] : "<none>") << std::endl;
    return false;
}

}}
