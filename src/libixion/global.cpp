/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "global.hpp"
#include "mem_str_buf.hpp"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <boost/thread/mutex.hpp>

using namespace std;

namespace ixion {

namespace {

struct _cell_name_data {
    const cell_ptr_name_map_t* store;
    ::boost::mutex mtx;
} cell_name_data;

}

const char* get_formula_result_output_separator()
{
    static const char* sep = 
        "---------------------------------------------------------";
    return sep;
}

void global::build_ptr_name_map(const cell_name_ptr_map_t& cells, cell_ptr_name_map_t& cell_names)
{
    cell_ptr_name_map_t _cell_names;
    cell_name_ptr_map_t::const_iterator itr = cells.begin(), itr_end = cells.end();
    for (; itr != itr_end; ++itr)
        _cell_names.insert(cell_ptr_name_map_t::value_type(itr->second, itr->first));
    cell_names.swap(_cell_names);
}

void global::set_cell_name_map(const cell_ptr_name_map_t* p)
{
    ::boost::mutex::scoped_lock lock(cell_name_data.mtx);
    cell_name_data.store = p;
}

string global::get_cell_name(const base_cell* cell)
{
    ::boost::mutex::scoped_lock lock(cell_name_data.mtx);
    if (cell_name_data.store)
    {
        cell_ptr_name_map_t::const_iterator itr = cell_name_data.store->find(cell);
        if (itr != cell_name_data.store->end())
            return itr->second;
    }
    return string("<unknown cell>");
}

double global::get_current_time()
{
#ifdef _WIN32
    FILETIME ft;
    __int64 *time64 = (__int64 *) &ft;
    GetSystemTimeAsFileTime (&ft);
    return *time64 / 10000000.0;
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

void global::sleep(unsigned int seconds)
{
#ifdef _WIN32
    ::Sleep(1000*seconds);
#else
    ::sleep(seconds);
#endif
}

void global::load_file_content(const string& filepath, string& content)
{
    ifstream file(filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(filepath);

    ostringstream os;
    os << file.rdbuf() << ' '; // extra char as the end position.
    file.close();

    os.str().swap(content);
}

const char* get_formula_error_name(formula_error_t fe)
{
    static const char* names[] = {
        "",        // no error
        "#REF!",   // result not available
        "#DIV/0!", // division by zero
        "#NUM!"    // invalid expression
    };
    static const size_t name_size = 4;
    if (static_cast<size_t>(fe) < name_size)
        return names[fe];

    return "#ERR!";
}

// ============================================================================

general_error::general_error(const string& msg) :
    m_msg(msg)
{
}

general_error::~general_error() throw()
{
}

const char* general_error::what() const throw()
{
    return m_msg.c_str();
}

// ============================================================================

file_not_found::file_not_found(const string& fpath) : 
    m_fpath(fpath)
{
}

file_not_found::~file_not_found() throw()
{
}

const char* file_not_found::what() const throw()
{
    ostringstream oss;
    oss << "specified file not found: " << m_fpath;
    return oss.str().c_str();
}

// ============================================================================

formula_error::formula_error(formula_error_t fe) :
    m_ferror(fe)
{
}

formula_error::~formula_error() throw()
{
}

const char* formula_error::what() const throw()
{
    return get_formula_error_name(m_ferror);
}

formula_error_t formula_error::get_error() const
{
    return m_ferror;
}

}
