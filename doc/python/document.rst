
.. py:currentmodule:: ixion

Document
========

.. class:: Document()

   Class :class:`~ixion.Document` represents an entire document which consists
   of one or more :class:`~ixion.Sheet` objects.

.. method:: Document.append_sheet(sheet_name)

   Append a new sheet to the document object and return the newly created
   :class:`~ixion.Sheet` object. The *sheet name* will become the name of the
   sheet object being appended. Each :class:`Sheet` object must have a name,
   and the name must be unique within the document.

   :param str sheet_name: name of the sheet to be appended to the document.

.. method:: Document.get_sheet(arg)

   Get a sheet object either by the position or by the name.  When the ``arg``
   is an integer, it returns the sheet object at specified position (0-based).
   When the ``arg`` is a string, it returns the sheet object whose name
   matches the specified string.

   :param arg: either the name of a sheet if it's of type ``str``, or the
      index of a sheet if it's of type ``int``.
   :rtype: :class:`ixion.Sheet`
   :return: sheet object representing the specified sheet.

.. warning:: Prefer passing a sheet index to :meth:`~ixion.Document.get_sheet`
             than passing a sheet name.  When passing a sheet name as an
             argument, the current :meth:`~ixion.Document.get_sheet`
             implementation has to iterate through the sheet objects in the
             document to find a matching one.

.. method:: Document.calculate()

   Calculate all formula cells within the document that are marked "dirty" i.e.
   either those formula cells whose direct or indirect references have changed
   their values, or those formula cells that have been entered into the
   document.

.. attribute:: Document.sheet_names

   A read-only attribute that provides a tuple of the names of the sheets that
   belong to the document. The order of the sheet names signifies the order
   of the sheets in the document.

