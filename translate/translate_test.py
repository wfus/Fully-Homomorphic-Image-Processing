import sys
import pycparser as pyc 

text = r"""
void func(void)
{
  x = 1;
}
"""

parser = pyc.c_parser.CParser()
ast = parser.parse(text)
print("Before:")
ast.show(offset=2)

assign = ast.ext[0].body.block_items[0]
assign.lvalue.name = "y"
assign.rvalue.value = 2

print("After:")
ast.show(offset=2)