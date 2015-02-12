
Overview
========

Let's go over very quickly how to create a document and populate some cells inside spreadsheet.

First, you need to import ixion module and create a new document.

::

    import ixion

    doc = ixion.Document()

Since your newly-created document has no sheet at all, you need to insert one.

::

    sheet1 = doc.append_sheet("MySheet1")





