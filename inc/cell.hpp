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

#ifndef __CELL_HPP__
#define __CELL_HPP__

#include "tokens.hpp"
#include "global.hpp"

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

class base_cell
{
public:
    base_cell(const ::std::string& name);
    base_cell(const base_cell& r);
    virtual ~base_cell() = 0;

    virtual const char* print() const = 0;

    const ::std::string& get_name() const;

private:
    ::std::string m_name;
};

// ============================================================================

class string_cell : public base_cell
{
public:
    string_cell(const ::std::string& name, const ::std::string& formula);
    string_cell(const string_cell& r);
    virtual ~string_cell();

    const char* print() const;

private:
    string_cell();

    ::std::string m_formula;
};

// ============================================================================

class formula_cell : public base_cell
{
public:
    formula_cell(const ::std::string& name, tokens_t& tokens);
    formula_cell(const formula_cell& r);
    virtual ~formula_cell();

    const char* print() const;

private:
    tokens_t m_tokens;
};

// ============================================================================

}

#endif
