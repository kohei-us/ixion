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

#ifndef __IXION_CELL_HPP__
#define __IXION_CELL_HPP__

#include "formula_tokens.hpp"
#include "global.hpp"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <string>

namespace ixion {

// ============================================================================

class address
{
public:
    address();
    ~address();
private:
};

// ============================================================================

enum celltype_t
{
    celltype_string,
    celltype_formula,
    celltype_unknown
};

// ============================================================================

class base_cell
{
public:
    base_cell(celltype_t celltype);
    base_cell(const base_cell& r);
    virtual ~base_cell() = 0;

    virtual double get_value() const = 0;
    virtual const char* print() const = 0;

    celltype_t get_celltype() const;

private:
    base_cell(); // disabled

    celltype_t m_celltype;
};

// ============================================================================

class string_cell : public base_cell
{
public:
    string_cell(const ::std::string& formula);
    string_cell(const string_cell& r);
    virtual ~string_cell();

    virtual const char* print() const;

private:
    string_cell();

    ::std::string m_formula;
};

// ============================================================================

class formula_cell : public base_cell
{
    struct result_cache
    {
        double          value;
        ::std::string   text;
        formula_error_t error;

        result_cache();
        result_cache(const result_cache& r);
    };

    struct interpret_status
    {
        ::boost::mutex mtx;
        ::boost::condition_variable cond;

        result_cache* result;

        interpret_status();
        interpret_status(const interpret_status& r);
        ~interpret_status();
    };

    class interpret_guard
    {
    public:
        explicit interpret_guard(interpret_status& status);
        ~interpret_guard();
    private:
        interpret_guard();

        interpret_status& m_status;
    };

public:
    formula_cell();
    formula_cell(formula_tokens_t& tokens);
    formula_cell(const formula_cell& r);
    virtual ~formula_cell();

    virtual double get_value() const;
    virtual const char* print() const;
    const formula_tokens_t& get_tokens() const;
    void interpret(const cell_ptr_name_map_t& cell_ptr_name_map);
    bool is_circular_safe() const;
    void check_circular();
    void reset();
    void swap_tokens(formula_tokens_t& tokens);

private:
    void wait_for_interpreted_result() const;

private:
    formula_tokens_t    m_tokens;

    mutable interpret_status    m_interpret_status;

    bool m_circular_safe;
};

// ============================================================================

inline bool operator <(const base_cell& l, const base_cell& r)
{
    return &l < &r;
}

}

#endif
