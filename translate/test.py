# import sys
# sys.path.insert(0, "../..")

# if sys.version_info[0] >= 3:
#     raw_input = input

tokens = (
    'NAME', 'NUMBER',
)

literals = ['=', '+', '-', '*', '/', '(', ')']

# Tokens

t_NAME = r'[a-zA-Z_][a-zA-Z0-9_]*'


def t_NUMBER(t):
    r'\d+'
    t.value = int(t.value)
    return t

t_ignore = " \t"


def t_newline(t):
    r'\n+'
    t.lexer.lineno += t.value.count("\n")


def t_error(t):
    print("Illegal character '%s'" % t.value[0])
    t.lexer.skip(1)

# Build the lexer
import lex
lex.lex()

# Parsing rules

precedence = (
    ('left', '+', '-'),
    ('left', '*', '/'),
    ('right', 'UMINUS'),
)

# dictionary of names
names = {}


def p_statement_assign(p):
    'statement : NAME "=" expression'
    names[p[1]] = p[3]


def p_statement_expr(p):
    'statement : expression'
    print(p[1])


def p_expression_binop(p):
    '''expression : expression '+' expression
                  | expression '-' expression
                  | expression '*' expression
                  | expression '/' expression'''
    p[0] = (p[2], p[1], p[3])


def p_expression_uminus(p):
    "expression : '-' expression %prec UMINUS"
    p[0] = -p[2]


def p_expression_group(p):
    "expression : '(' expression ')'"
    p[0] = p[2]


def p_expression_number(p):
    "expression : NUMBER"
    p[0] = p[1]


def p_expression_name(p):
    "expression : NAME"
    p[0] = p[1]


def p_error(p):
    if p:
        print("Syntax error at '%s'" % p.value)
    else:
        print("Syntax error at EOF")

import yacc
yacc.yacc()

OPS = ['*', '-', '+', '/']

"""
Parses the text line
DEST = ARG1 (OP)  ... (OP) ARGN
"""
def parse_line(line):
    line = line.replace(' ', '').replace(';', '').split('=')
    dst = line[0]
    args_op = line[1]
    if dst[-1] in OPS:
        args_op = dst + '(' + args_op + ')'
        dst = dst[:-1]
    yacc.parse(args_op)
    print(yacc.source)
    return (dst, yacc.parse(args_op))

if __name__ == '__main__':
    print(parse_line('y = 1 + 1'))
