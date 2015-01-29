
import sys
sys.path.append("./.libs")

import ixion

ixion.info()

doc = ixion.Document()
print(doc)
sheet1 = doc.append_sheet("Sheet1")
print(sheet1)
print("sheet name: {}".format(sheet1.name))
sheet2 = doc.append_sheet("Sheet2")
print(sheet2)
print("sheet name: {}".format(sheet2.name))

sheet1.set_numeric_cell(row=1, column=8, value=10.2)
sheet1.set_numeric_cell(column=8, row=0, value=11)
sheet1.set_numeric_cell(2, 8, 12.2) # row, column, value in this order

#sheet1.set_formula_cell(row=2, column=8, value="A1+A2")
#sheet1.set_string_cell(row=0, column=1, value="Test")

#doc.calculate()

class MyDoc(ixion.Document):
    pass

doc = MyDoc()
print(doc)

class MySheet(ixion.Sheet):
    pass

sheet = MySheet("my sheet")
print(sheet)
print(sheet.name)
