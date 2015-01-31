
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

col = 1
sheet1.set_numeric_cell(row=1, column=col, value=10.2)
sheet1.set_numeric_cell(column=col, row=0, value=11)
sheet1.set_numeric_cell(2, col, 12.2) # row, column, value in this order

for row in xrange(0, 3):
    val = sheet1.get_numeric_value(row, col)
    print("(row={_row},col={_col}) = {_val}".format(_row=row, _col=col, _val=val))

sheet1.set_formula_cell(row=3, column=1, value="SUM(B1:B3)")
s = sheet1.get_formula_expression(row=3, column=1)
print("(row=3,col=1) = {}".format(s))

col = 2
sheet1.set_string_cell(row=0, column=col, value="Test")
s = sheet1.get_string_value(row=0, column=col)
print("(row=0,col={}) = {}".format(col, s))

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

print("end of script")
