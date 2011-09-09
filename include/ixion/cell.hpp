/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#include "ixion/formula_tokens.hpp"
#include "ixion/global.hpp"
#include "ixion/address.hpp"
#include "ixion/hash_container/set.hpp"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>

namespace ixion {

class formula_result;
class formula_cell;

namespace interface {

class model_context;

}

enum celltype_t
{
    celltype_unknown = 0x0000,
    celltype_string  = 0x0001,
    celltype_numeric = 0x0002,
    celltype_formula = 0x0003,
    celltype_mask    = 0x000F
};

/**
 * You can't delete a base_cell instance with the normal delete operator;
 * you must call base_cell::delete_instance() to delete it.
 */
class base_cell : boost::noncopyable
{
public:
    static void delete_instance(const base_cell* p);

    double get_value() const;
    size_t get_identifier() const;
    void set_identifier(size_t identifier);
    celltype_t get_celltype() const;

private:
    base_cell(); // disabled

    union {
        int m_raw_bits:32;
        struct {
            int flag:24;
            int celltype:8;
        } m_data;
    };

protected:
    base_cell(celltype_t celltype, double value);
    base_cell(celltype_t celltype, size_t identifier);
    ~base_cell();

    void set_flag(int mask, bool value);
    bool get_flag(int mask) const;
    void reset_flag();

    union {
        double m_value;
        size_t m_identifier;
    };
};

class string_cell : public base_cell
{
    string_cell();
public:
    string_cell(size_t identifier);
};

class numeric_cell : public base_cell
{
    numeric_cell();
public:
    numeric_cell(double value);
};

class formula_cell : public base_cell
{
    struct interpret_status : boost::noncopyable
    {
        ::boost::mutex mtx;
        ::boost::condition_variable cond;

        formula_result* result;

        interpret_status();
        ~interpret_status();
    };

public:
    formula_cell();
    formula_cell(size_t tokens_identifier);
    ~formula_cell();

    double get_value() const;
    void interpret(const interface::model_context& context);

    /**
     * Determine if this cell contains circular reference by walking through
     * all its reference tokens.
     */
    void check_circular(const interface::model_context& cxt);

    /**
     * Reset cell's internal state.
     */
    void reset();

    void get_ref_tokens(interface::model_context& cxt, std::vector<formula_token_base*>& tokens);

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

    bool check_ref_for_circular_safety(const base_cell& ref);

private:
    mutable interpret_status m_interpret_status;
};

// ============================================================================

inline base_cell* new_clone(const base_cell& r)
{
    // TODO: Implement cloning and make this function usable.
    switch (r.get_celltype())
    {
        case celltype_formula:
        case celltype_string:
        case celltype_unknown:
        default:
            ;
    }
    return NULL;
}

inline void delete_clone(const base_cell* p)
{
    base_cell::delete_instance(p);
}

inline bool operator <(const base_cell& l, const base_cell& r)
{
    return &l < &r;
}

}

#endif
