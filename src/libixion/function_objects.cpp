/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#include "ixion/function_objects.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/address.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/interface/model_context.hpp"

namespace ixion {

formula_cell_listener_handler::formula_cell_listener_handler(
    interface::model_context& cxt, const abs_address_t& addr, mode_t mode) :
    m_context(cxt),
    m_listener_tracker(cell_listener_tracker::get(cxt)),
    m_addr(addr),
    m_mode(mode)
{
#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << "formula_cell_listener_handler: cell position=" << m_addr.get_name() << endl;
#endif
}

void formula_cell_listener_handler::operator() (const formula_token_base* p) const
{
    switch (p->get_opcode())
    {
        case fop_single_ref:
        {
            abs_address_t addr = p->get_single_ref().to_abs(m_addr);
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "formula_cell_listener_handler: ref address=" << addr.get_name() << endl;
#endif
            if (m_mode == mode_add)
            {
                m_listener_tracker.add(m_addr, addr);
            }
            else
            {
                assert(m_mode == mode_remove);
                m_listener_tracker.remove(m_addr, addr);
            }
        }
        break;
        case fop_range_ref:
        {
            abs_range_t range = p->get_range_ref().to_abs(m_addr);
            if (m_mode == mode_add)
                cell_listener_tracker::get(m_context).add(m_addr, range);
            else
            {
                assert(m_mode == mode_remove);
                cell_listener_tracker::get(m_context).remove(m_addr, range);
            }
        }
        break;
        default:
            ; // ignore the rest.
    }
}

}
