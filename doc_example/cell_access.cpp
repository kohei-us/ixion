#include <iostream>
#include <ixion/document.hpp>
#include <ixion/model_context.hpp>
#include <ixion/cell_access.hpp>
#include <ixion/address.hpp>

void access(const ixion::cell_access& ca)
{
    //!code-start: get-value
    switch (ca.get_value_type())
    {
        case ixion::cell_value_t::numeric:
        {
            double v = ca.get_numeric_value();
            std::cout << "numeric value: " << v << std::endl;
            break;
        }
        case ixion::cell_value_t::string:
        {
            std::string_view s = ca.get_string_value();
            std::cout << "string value: " << s << std::endl;
            break;
        }
        case ixion::cell_value_t::boolean:
        {
            std::cout << "boolean value: " << ca.get_boolean_value() << std::endl;
            break;
        }
        case ixion::cell_value_t::error:
        {
            ixion::formula_error_t err = ca.get_error_value();
            std::cout << "error value: " << ixion::get_formula_error_name(err) << std::endl;
            break;
        }
        case ixion::cell_value_t::empty:
        {
            std::cout << "empty cell" << std::endl;
            break;
        }
        default:
            std::cout << "???" << std::endl;
    }
    //!code-end: get-value
}

void from_document()
{
    //!code-start: get-from-document
    ixion::document doc;
    doc.append_sheet("Sheet");

    // fill this document

    ixion::cell_access ca = doc.get_cell_access("A1");
    //!code-end: get-from-document

    access(ca);
}

void from_model_context()
{
    //!code-start: get-from-model-context
    ixion::model_context cxt;
    cxt.append_sheet("Sheet");

    // fill this model context

    ixion::abs_address_t A1(0, 0, 0);
    ixion::cell_access ca = cxt.get_cell_access(A1);
    //!code-end: get-from-model-context

    access(ca);
}

int main(int argc, char** argv)
{
    from_document();
    from_model_context();

    return EXIT_SUCCESS;
}
