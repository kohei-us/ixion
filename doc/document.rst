
Document
========

.. class:: Document()

   Class :class:`Document` represents a whole document which may consist of one or more :class:`Sheet`
   objects.

.. method:: Document.append_sheet(sheet name)

   Append a new sheet to the document object and return the newly created :class:`Sheet` object.
   The *sheet name* will become the name of the sheet object being appended.
   Each :class:`Sheet` object must have a name, and the name must be unique
   within the document.

.. method:: Document.get_sheet_names()

   Get a tuple of string objects that represent the names of the sheets that
   belong to the document.  The order of the sheet names represents the order
   of the sheets in the document.

.. method:: Document.get_sheet(arg)

   Get a sheet object either by the position or by the name.  When the *arg* is
   an integer, it returns the sheet object at specified position (0-based).  When
   the *arg* is a string, it returns the sheet object whose name matches that string.

.. warning:: Prefer passing a sheet index to :meth:`get_sheet` than passing a
             sheet name.  When passing a sheet name as an argument, the current
             :meth:`get_sheet` implementation has to iterate through the sheet
             objects in the document to find a matching one.

.. method:: Document.calculate()

   Calculate all formula cells within the document that are marked "dirty" i.e. either those formula cells whose direct 
   or indirect references have changed their values, or those formula cells that have been entered into the document.


