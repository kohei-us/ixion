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
#include "ixion/address.hpp"
#include "ixion/hash_container/set.hpp"
#include "ixion/types.hpp"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>

namespace ixion {

class formula_result;
class formula_cell;

namespace iface {

class model_context;

}

/**
 * This class itself cannot be directly instantiated; you always need to
 * instantiate its child classes.
 *
 * You can't delete a base_cell instance with the normal delete operator
 * either; call base_cell::delete_instance() to delete it instead.
 */
class base_cell : boost::noncopyable
{
private:
    base_cell(); // disabled

protected:

    union {
        int m_raw_bits:32;
        struct {
            int flag:24;
            int celltype:8;
        } m_data;
    };

    base_cell(celltype_t celltype);
    ~base_cell();
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
    IXION_DLLPUBLIC formula_cell();
    IXION_DLLPUBLIC formula_cell(size_t tokens_identifier);
    IXION_DLLPUBLIC ~formula_cell();

    size_t get_identifier() const;
    void set_identifier(size_t identifier);

    void set_flag(int mask, bool value);
    bool get_flag(int mask) const;
    void reset_flag();

    IXION_DLLPUBLIC double get_value() const;
    IXION_DLLPUBLIC void interpret(iface::model_context& context, const abs_address_t& pos);

    /**
     * Determine if this cell contains circular reference by walking through
     * all its reference tokens.
     */
    void check_circular(const iface::model_context& cxt, const abs_address_t& pos);

    /**
     * Reset cell's internal state.
     */
    IXION_DLLPUBLIC void reset();

    IXION_DLLPUBLIC void get_ref_tokens(
        const iface::model_context& cxt, const abs_address_t& pos, std::vector<const formula_token_base*>& tokens);

    IXION_DLLPUBLIC const formula_result* get_result_cache() const;

    IXION_DLLPUBLIC bool is_shared() const;
    IXION_DLLPUBLIC void set_shared(bool b);

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

    bool check_ref_for_circular_safety(const formula_cell& ref, const abs_address_t& pos);

private:
    mutable interpret_status m_interpret_status;
    size_t m_identifier;
};

}

#endif
