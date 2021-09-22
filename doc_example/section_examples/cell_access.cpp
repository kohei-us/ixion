#include <iostream>
#include <ixion/document.hpp>
#include <ixion/model_context.hpp>
#include <ixion/cell_access.hpp>
#include <ixion/address.hpp>

using namespace std;

void access(const ixion::cell_access& ca)
{
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
}

void from_document()
{
    ixion::document doc;
    doc.append_sheet("Sheet");

    // fill this document

    ixion::cell_access ca = doc.get_cell_access("A1");

    access(ca);
}

void from_model_context()
{
    ixion::model_context cxt;
    cxt.append_sheet("Sheet");

    // fill this model context

    ixion::abs_address_t A1(0, 0, 0);
    ixion::cell_access ca = cxt.get_cell_access(A1);

    access(ca);
}

int main(int argc, char** argv)
{
    from_document();
    from_model_context();

    return EXIT_SUCCESS;
}
