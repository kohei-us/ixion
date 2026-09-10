.. highlight:: cpp

.. _formula-syntax:

Formula syntax and name resolvers
=================================

As we saw on the :ref:`concepts` page, Ixion stores formulas as tokens, and a
:cpp:class:`~ixion::formula_name_resolver` is what translates between a
formula string and those tokens.  This page looks at the syntaxes Ixion
understands, what you can write in a formula, and how the same tokens come
out when printed in a different syntax.

The snippets on this page come from one example program, which starts by
creating a model with three sheets and a resolver for each of the syntaxes
we'll be using:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: setup
   :end-before: //!code-end: setup
   :dedent: 4

Passing the model context to :cpp:func:`~ixion::formula_name_resolver::get`
is what lets a resolver turn sheet names into sheet indices and back.  If
your formulas never mention sheet names, you can pass ``nullptr`` instead.


The five syntaxes
-----------------

Here is how a reference to cell B1 on a sheet named ``Sheet2`` looks in each
of the syntaxes, along with the value of
:cpp:enum:`~ixion::formula_name_resolver_t` that selects it:

======================  ================  ================
Syntax                  Enum value        Example
======================  ================  ================
Excel A1                ``excel_a1``      ``Sheet2!B1``
Excel R1C1              ``excel_r1c1``    ``Sheet2!R1C2``
LibreOffice Calc A1     ``calc_a1``       ``Sheet2.B1``
OpenFormula (ODFF)      ``odff``          ``[Sheet2.B1]``
ODF cell-range-address  ``odf_cra``       ``Sheet2.B1``
======================  ================  ================

The Excel syntaxes separate the sheet name from the address with ``!``,
whereas the Calc and ODF syntaxes use ``.``.  OpenFormula additionally wraps
every reference in square brackets.  The ODF cell-range-address syntax is
what the ODF file format uses wherever an attribute holds a cell range
outside of a formula, such as the source range of a chart, a pivot table or
a database range.  It is the same as Calc A1 except that a reference without
a sheet name starts with a bare ``.``, as in ``.B1``.  Since there is no
formula cell for such a range to be relative to, the address is always
understood as absolute, and the ``$`` markers are normally left out.

The R1C1 syntax spells out row and column numbers instead of column letters.
``R1C2`` is row 1, column 2, and a number in brackets is an offset from the
formula cell, so ``R[-1]C[2]`` is one row up and two columns to the right.
A bare ``R`` or ``C`` with no number at all means an offset of zero, so
``RC[2]`` is the same row as the formula cell, two columns to the right.


Parsing and printing
--------------------

Let's parse a formula written in Excel A1 syntax for cell C1 on the first
sheet, and take a look at the tokens we get back:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: parse
   :end-before: //!code-end: parse
   :dedent: 4

.. code-block:: text

    number of tokens: 8
      function token: (opcode=278; name='SUM')
      opcode token: (name=open; s='(')
      range ref token: (sheet:0 rel; row:0 rel; column:-2 rel)-(sheet:0 rel; row:9 rel; column:-2 rel)
      opcode token: (name=close; s=')')
      opcode token: (name=plus; s='+')
      single ref token: (sheet:1 abs; row:0 rel; column:-1 rel)
      opcode token: (name=multiply; s='*')
      value token: 2

Notice that the two references have been turned into offsets from C1, and
that neither token remembers which syntax it came from.  That's what lets us
print the same tokens in any of the other syntaxes:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: print
   :end-before: //!code-end: print
   :dedent: 4

.. code-block:: text

    excel_a1:   SUM(A1:A10)+Sheet2!B1*2
    excel_r1c1: SUM(RC[-2]:R[9]C[-2])+Sheet2!RC[-1]*2
    calc_a1:    SUM(A1:A10)+$Sheet2.B1*2
    odff:       SUM([.A1:.A10])+[$Sheet2.B1]*2

Both :cpp:func:`~ixion::parse_formula_string` and
:cpp:func:`~ixion::print_formula_tokens` take the position of the formula
cell, since relative references only make sense in relation to it.  You
don't have to print from the same position you parsed at, and we'll make use
of that below.

.. todo::

   Diagram: One set of tokens in the middle, with the four printed strings
   fanning out from it, and the parsed Excel A1 string feeding into it.

   File: images/formula-syntax-round-trip.svg


References
----------

A reference has up to three parts: a sheet, a column and a row.  Each part
can be *relative*, meaning it is stored as an offset from the formula cell
and follows the formula around when it moves, or *absolute*, meaning it
always points at the same place.

In the A1 syntaxes, a ``$`` in front of the column letters or the row number
makes that part absolute.  The R1C1 syntax has no ``$``; a plain number is
absolute and a bracketed number is relative:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: absolute
   :end-before: //!code-end: absolute
   :dedent: 4

.. code-block:: text

    excel_a1:   $A$1+A$1+$A1+A1
    excel_r1c1: R1C1+R1C[-1]+R[-1]C1+R[-1]C[-1]

A range is two cell addresses joined by ``:``, as in ``A1:B10``.  You can
also refer to whole columns or rows with ``A:A`` or ``2:2``.  In the Excel
syntaxes a range can span sheets too, as in ``Sheet1:Sheet3!A1``, and in the
Calc A1 and OpenFormula syntaxes each end of a range can carry its own sheet
name, as in ``Sheet1.A1:Sheet3.A1``.

.. todo::

   Diagram: Anatomy of a reference, ``$Sheet1.$A$1`` and ``Sheet1!$A$1``
   side by side, with the sheet, column and row parts labelled and the
   absolute markers highlighted.

   File: images/formula-syntax-reference-parts.svg


Sheet-relative and sheet-absolute references
--------------------------------------------

The sheet part of a reference deserves a closer look, because the syntaxes
treat it differently.

In the Excel syntaxes, a reference without a sheet name is relative to the
sheet the formula is on.  If you copy the formula to another sheet, the
reference follows it there.  As soon as you write a sheet name, the
reference becomes absolute and stays pinned to that sheet, no matter where
the formula moves.

In the Calc A1 and OpenFormula syntaxes, writing a sheet name does *not*
make it absolute.  ``Sheet1.A1`` is still relative, and it takes a ``$`` in
front of the sheet name, as in ``$Sheet1.A1``, to pin it down.  In Calc A1,
a reference with no sheet name is relative, as in the Excel syntaxes.

We can see the difference by parsing a formula for a cell on the first sheet
and then printing it as if the formula had moved to the second sheet.  To
make the sheet part visible, we ask :cpp:func:`~ixion::print_formula_tokens`
to always print the sheet name via :cpp:struct:`~ixion::print_config`:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: sheet-relative
   :end-before: //!code-end: sheet-relative
   :dedent: 4

.. code-block:: text

    excel_a1: Sheet2!A1+Sheet1!A1
    calc_a1:  Sheet2.A1+Sheet2.A1+$Sheet1.A1

In the Excel A1 line, the unqualified ``A1`` now points at ``Sheet2`` while
``Sheet1!A1`` stayed put.  In the Calc A1 line, both ``A1`` and ``Sheet1.A1``
moved along to ``Sheet2``, and only ``$Sheet1.A1`` stayed put.

.. note::

    By default, :cpp:func:`~ixion::print_formula_tokens` prints a sheet
    name only when the referenced sheet differs from the one the formula
    cell is on.  Use the :cpp:enum:`~ixion::display_sheet_t` value in
    :cpp:struct:`~ixion::print_config` to always or never print it.

.. todo::

   Diagram: Two sheets side by side; a formula cell on Sheet1 with ``A1``
   and ``Sheet1!A1`` copied to Sheet2, arrows showing the first reference
   following to Sheet2!A1 and the second staying on Sheet1!A1.

   File: images/formula-syntax-sheet-relative.svg


Sheet names with spaces
-----------------------

A sheet name that contains a space or a quote has to be quoted with single
quotes, and a single quote inside the name is doubled.  Ixion applies the
same rule when printing:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: quoted-sheet
   :end-before: //!code-end: quoted-sheet
   :dedent: 4

.. code-block:: text

    excel_a1: 'Annual Report'!A1:B2
    calc_a1:  $'Annual Report'.A1:B2


Functions, literals and operators
---------------------------------

Function names are case-insensitive, and the arguments are separated by
the character stored in :cpp:member:`ixion::config::sep_function_arg`,
which is a comma by default.  ``TRUE()`` and ``FALSE()`` are functions too,
so they need their parentheses.

A string literal is enclosed in double quotes.  An inline array is enclosed
in braces, with the argument separator between columns and ``;`` between
rows.  Error values such as ``#DIV/0!``, ``#N/A``, ``#NAME?``, ``#REF!`` and
``#VALUE!`` can be written directly into a formula:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: literals
   :end-before: //!code-end: literals
   :dedent: 4

.. code-block:: text

    IF(A1>=10,"big","small")&{1,2;3,4}

The operators are the usual ones: ``+``, ``-``, ``*``, ``/`` and ``^`` for
arithmetic, ``&`` for string concatenation, ``=``, ``<>``, ``<``, ``<=``,
``>`` and ``>=`` for comparison, plus parentheses and a leading sign.


Named expressions and table references
--------------------------------------

Any name that is neither a reference nor a function is taken to be a named
expression.  The resolver doesn't check whether such a name exists; it is
looked up when the formula is calculated, and a name that isn't found gives
the ``#NAME?`` error at that point.  As a consequence, a misspelled function
name doesn't fail to parse either:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: names
   :end-before: //!code-end: names
   :dedent: 4

.. code-block:: text

      function token: (opcode=278; name='SUM')
      opcode token: (name=open; s='(')
      named expression token: 'MyRange'
      opcode token: (name=close; s=')')

Table references such as ``Table1[Value]`` or ``Table1[[#Headers],[Value]]``
are recognized by the Excel A1 resolver.  Named expressions and tables each
get their own treatment on a later page.


Parse errors
------------

When a formula string can't be turned into tokens at all,
:cpp:func:`~ixion::parse_formula_string` throws an exception derived from
:cpp:class:`~ixion::general_error`:

.. literalinclude:: ../../doc_example/formula_syntax.cpp
   :language: C++
   :start-after: //!code-start: parse-error
   :end-before: //!code-end: parse-error
   :dedent: 4

.. code-block:: text

    parse failed: failed to parse an error token in lexer tokenizer: '#'

The complete source code of this example is available
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/formula_syntax.cpp>`_.
