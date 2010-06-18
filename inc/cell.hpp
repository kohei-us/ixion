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
#include <unordered_set>

namespace ixion {

class formula_result;
class formula_cell;

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

    void register_listener(formula_cell* p);

    celltype_t get_celltype() const;

private:
    base_cell(); // disabled

private:
    /**
     * List of formula cells that reference this cell.
     */
    ::std::unordered_set<formula_cell*> m_listeners;

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
    struct interpret_status
    {
        ::boost::mutex mtx;
        ::boost::condition_variable cond;

        formula_result* result;

        interpret_status();
        interpret_status(const interpret_status& r);
        ~interpret_status();
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

    /**
     * Determine if this cell contains circular reference by walking through
     * all its reference tokens.
     */
    void check_circular();

    /**
     * Reset cell's internal state. 
     */
    void reset();

    void swap_tokens(formula_tokens_t& tokens);

    void get_ref_tokens(::std::vector<formula_token_base*>& tokens);

    const formula_result* get_result_cache() const;

private:
    /**
     * Block until the result becomes available.
     * 
     * @param lock mutex lock associated with the result cache data.
     */
    void wait_for_interpreted_result(::boost::mutex::scoped_lock& lock) const;

    /**
     * Check if this cell contains a circular reference. 
     *  
     * @return true if this cell contains no circular reference, hence 
     *         considered "safe", false otherwise.
     */
    bool is_circular_safe() const;

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
