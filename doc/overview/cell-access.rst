
Use cell_access
===============

You can obtain a :cpp:class:`~ixion::cell_access` instance either from
:cpp:class:`~ixion::model_context` or :cpp:class:`~ixion::document` class.

Here is an example of how to obtain it from a :cpp:class:`~ixion::model_context` instance::

    ixion::model_context cxt;
    cxt.append_sheet("Sheet");

    // fill this model context

    ixion::abs_address_t A1(0, 0, 0);
    ixion::cell_access ca = cxt.get_cell_access(A1);


Here is an example of how to obtain it from a :cpp:class:`~ixion::document` instance::

    ixion::document doc;
    doc.append_sheet("Sheet");

    // fill this document

    ixion::cell_access ca = doc.get_cell_access("A1");


Once you have your :cpp:class:`~ixion::cell_access` instance, you can, for instance,
print the value of the cell as follows::

    switch (ca.get_value_type())
    {
        case ixion::cell_value_t::numeric:
        {
            double v = ca.get_numeric_value();
            cout << "numeric value: " << v << endl;
            break;
        }
        case ixion::cell_value_t::string:
        {
            std::string_view s = ca.get_string_value();
            cout << "string value: " << s << endl;
            break;
        }
        case ixion::cell_value_t::boolean:
        {
            cout << "boolean value: " << ca.get_boolean_value() << endl;
            break;
        }
        case ixion::cell_value_t::error:
        {
            ixion::formula_error_t err = ca.get_error_value();
            cout << "error value: " << ixion::get_formula_error_name(err) << endl;
            break;
        }
        case ixion::cell_value_t::empty:
        {
            cout << "empty cell" << endl;
            break;
        }
        default:
            cout << "???" << endl;
    }

The complete source code of this example is avaiable
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/section_examples/cell_access.cpp>`_.
