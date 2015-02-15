
Document
========

.. class:: Document()

   Class :class:`Document` represents a whole document which may consist of one or more :class:`Sheet`
   objects.

The append_sheet() method
-------------------------
.. method:: Document.append_sheet(sheet name)

   Append a new sheet to the document object and return the newly created :class:`Sheet` object.

   * `sheet name`_ - name to assign to the sheet object.

sheet name
^^^^^^^^^^

Each :class:`Sheet` object must have a name associated with it.  The name must be unique within the document 
it belongs to.

The calculate() method
----------------------
.. method:: Document.calculate()

   Calculate all formula cells within the document that are marked "dirty" i.e. either those formula cells whose direct 
   or indirect references have changed their values, or those formula cells that have been entered into the document.


