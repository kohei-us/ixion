
Document
========

.. class:: Document()

   Class :class:`Document` represents a whole document which may consist of one or more :class:`Sheet`
   objects.

.. method:: Document.append_sheet(sheet name)

   Append a new sheet to the document object and return the newly created :class:`Sheet` object.

   * sheet name - name to assign to the sheet object.  Each :class:`Sheet`
     object must have a name associated with it.  The name must be unique within
     the document it belongs to.

.. method:: Document.get_sheet_names()

   Get a tuple of string objects that represent the names of the sheets that
   belong to the document.  The order of the sheet names represents the order
   of the sheets.

.. method:: Document.calculate()

   Calculate all formula cells within the document that are marked "dirty" i.e. either those formula cells whose direct 
   or indirect references have changed their values, or those formula cells that have been entered into the document.


