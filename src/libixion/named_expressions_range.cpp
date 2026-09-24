/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/named_expressions_range.hpp>
#include "model_types.hpp"
#include "model_context_impl.hpp"

#include <optional>

namespace ixion {

namespace {

const detail::named_expressions_t& get_empty_named_expressions()
{
    static const detail::named_expressions_t empty;
    return empty;
}

} // anonymous namespace

class named_expressions_range::const_iterator::impl
{
    detail::named_expressions_t::const_iterator m_pos;
    std::optional<entry> m_current;

public:
    explicit impl(detail::named_expressions_t::const_iterator pos) : m_pos(pos) {}

    void next()
    {
        ++m_pos;
    }

    const entry& get()
    {
        m_current.emplace(m_pos->first, m_pos->second);
        return *m_current;
    }

    bool equals(const impl& other) const
    {
        return m_pos == other.m_pos;
    }
};

named_expressions_range::const_iterator::const_iterator(std::unique_ptr<impl> p) :
    mp_impl(std::move(p)) {}

named_expressions_range::const_iterator::const_iterator() :
    mp_impl(std::make_unique<impl>(get_empty_named_expressions().cend())) {}

named_expressions_range::const_iterator::const_iterator(const const_iterator& other) :
    mp_impl(std::make_unique<impl>(*other.mp_impl)) {}

named_expressions_range::const_iterator::const_iterator(const_iterator&& other) = default;

named_expressions_range::const_iterator::~const_iterator() = default;

named_expressions_range::const_iterator&
named_expressions_range::const_iterator::operator= (const const_iterator& other)
{
    mp_impl = std::make_unique<impl>(*other.mp_impl);
    return *this;
}

named_expressions_range::const_iterator&
named_expressions_range::const_iterator::operator= (const_iterator&& other) = default;

named_expressions_range::const_iterator& named_expressions_range::const_iterator::operator++()
{
    mp_impl->next();
    return *this;
}

named_expressions_range::const_iterator named_expressions_range::const_iterator::operator++(int)
{
    const_iterator previous(*this);
    ++*this;
    return previous;
}

named_expressions_range::const_iterator::reference
named_expressions_range::const_iterator::operator*() const
{
    return mp_impl->get();
}

named_expressions_range::const_iterator::pointer
named_expressions_range::const_iterator::operator->() const
{
    return &mp_impl->get();
}

bool named_expressions_range::const_iterator::operator== (const const_iterator& other) const
{
    return mp_impl->equals(*other.mp_impl);
}

class named_expressions_range::impl
{
public:
    const detail::named_expressions_t* named_exps;

    impl() : named_exps(&get_empty_named_expressions()) {}

    impl(const detail::model_context_impl& cxt, sheet_t scope) : named_exps(nullptr)
    {
        if (scope == global_scope)
            named_exps = &cxt.get_named_expressions();
        else
            named_exps = &cxt.get_named_expressions(scope);
    }
};

named_expressions_range::named_expressions_range() :
    mp_impl(std::make_unique<impl>()) {}

named_expressions_range::named_expressions_range(
    const detail::model_context_impl& cxt, sheet_t scope) :
    mp_impl(std::make_unique<impl>(cxt, scope)) {}

named_expressions_range::named_expressions_range(const named_expressions_range& other) :
    mp_impl(std::make_unique<impl>(*other.mp_impl)) {}

named_expressions_range::named_expressions_range(named_expressions_range&& other) = default;

named_expressions_range::~named_expressions_range() = default;

named_expressions_range& named_expressions_range::operator= (const named_expressions_range& other)
{
    mp_impl = std::make_unique<impl>(*other.mp_impl);
    return *this;
}

named_expressions_range&
named_expressions_range::operator= (named_expressions_range&& other) = default;

named_expressions_range::const_iterator named_expressions_range::begin() const
{
    auto pos = mp_impl->named_exps->cbegin();
    return const_iterator{std::make_unique<const_iterator::impl>(pos)};
}

named_expressions_range::const_iterator named_expressions_range::end() const
{
    auto pos = mp_impl->named_exps->cend();
    return const_iterator{std::make_unique<const_iterator::impl>(pos)};
}

named_expressions_range::const_iterator named_expressions_range::cbegin() const
{
    return begin();
}

named_expressions_range::const_iterator named_expressions_range::cend() const
{
    return end();
}

std::size_t named_expressions_range::size() const
{
    return mp_impl->named_exps->size();
}

bool named_expressions_range::empty() const
{
    return mp_impl->named_exps->empty();
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
