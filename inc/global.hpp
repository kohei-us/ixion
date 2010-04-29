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

#ifndef __IXION_GLOBAL_HPP__
#define __IXION_GLOBAL_HPP__

#include <string>
#include <unordered_map>
#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

class base_cell;

typedef ::boost::ptr_map< ::std::string, base_cell>             cell_name_ptr_map_t;
typedef ::std::unordered_map<const base_cell*, ::std::string>   cell_ptr_name_map_t;

class general_error : public ::std::exception
{
public:
    explicit general_error(const ::std::string& msg);
    ~general_error() throw();
    virtual const char* what() const throw();
private:
    ::std::string m_msg;
};

// ============================================================================

enum formula_error_t
{
    fe_ref_result_not_available,
    fe_division_by_zero,
    fe_no_error
};

class formula_error : public ::std::exception
{
public:
    explicit formula_error(formula_error_t fe);
    ~formula_error() throw();
    virtual const char* what() const throw();

    formula_error_t get_error() const;
private:
    formula_error_t m_ferror;
};

}

#endif
