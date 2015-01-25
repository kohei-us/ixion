
import sys
sys.path.append("./.libs")

import ixion

ixion.info()

doc = ixion.Document()
print(doc)
sheet1 = doc.append_sheet("Sheet1")
print(sheet1)
sheet2 = doc.append_sheet("Sheet2")
print(sheet2)

#sheet1.set_numeric_cell(row=1, column=8, value=10.2)
#sheet1.set_formula_cell(row=2, column=8, value="A1+A2")
#sheet1.set_string_cell(row=0, column=1, value="Test")

#doc.calculate()

class MyDoc(ixion.Document):
    pass

doc = MyDoc()
print(doc)

