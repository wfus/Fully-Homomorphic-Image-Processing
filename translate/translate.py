import re

OPS = ['+', '-', '*', '/']
OP_TEXT = {
    '+': "evaluator.add",
    '-': "evaluator.sub",
    '*': "evaluator.multiply",
    '/': "evaluator.multiply"
}


def infixToPostfix(infixexpr):
    prec = {}
    prec["*"] = 3
    prec["/"] = 3
    prec["+"] = 2
    prec["-"] = 2
    prec["("] = 1
    opStack = []
    postfixList = []
    tokenList = infixexpr.split()
    pattern = re.compile('[\.\[\]a-zA-Z\d~]+')
    for token in tokenList:
        if pattern.match(token):
            postfixList.append(token)
        elif token == '(':
            opStack.append(token)
        elif token == ')':
            topToken = opStack.pop()
            while topToken != '(':
                postfixList.append(topToken)
                topToken = opStack.pop()
        else:
            while (len(opStack) > 0) and (prec[opStack[-1]] >= prec[token]):
                  postfixList.append(opStack.pop())
            opStack.append(token)
    while len(opStack) > 0:
        postfixList.append(opStack.pop())
    return ' '.join(postfixList)

def postfix_to_tree(postfixexpr):
    expr = postfixexpr.split()
    # print(expr)
    while len(expr) > 1:
        ind = 0
        for i in range(len(expr)):
            if expr[i] in OPS:
                ind = i
                break
        # print(ind)
        if ind < 2:
            return 'ERROR'
        chunk = (expr[ind], expr[ind - 2], expr[ind - 1])
        expr = expr[:ind - 2] + [chunk] + expr[ind + 1:]
    return expr[0]

def parse_line(line):
    if not '=' in line:
        return "ERROR: NO ASSIGNMENT OPERATOR DETECTED"
    line = line.replace(' ', '').replace(';', '').split('=')
    dst = line[0]
    args_op = line[1]
    if dst[-1] in OPS:
        args_op = dst + '(' + args_op + ')'
        dst = dst[:-1]
    # print(args_op)
    # args_op = re.sub(r'\[(.*?)\]', '[]', args_op)
    # replace negation with ~
    args_op = re.sub('([\+\*-/][\s\(]*)-', r'\1~', args_op)
    args_op = args_op.replace('(', ' ( ')
    args_op = args_op.replace(')', ' ) ')
    for op in OPS:
        args_op = args_op.replace(op, ' ' + op + ' ')
    
    return dst, postfix_to_tree(infixToPostfix(args_op))


def convert_expr(expr):
    if not isinstance(expr, tuple): 
        raise ConversionException("Expression wasn't a tuple for some reason!")
    dst, args = expr
    expr_buffer = []
    last_var = convert_expr_helper(args, expr_buffer)
    arg_string = "".join(expr_buffer)
    if len(arg_string) == 0:
        raise ConversionException("Huh")
    full_string = "{}{} = {};".format(arg_string, dst, last_var)
    return full_string

def isfloat(value):
    try:
        float(value)
        return True
    except ValueError:
        return False

def convert_expr_helper(expr, buf):
    if not expr:
        raise ConversionError("Returned NONE")
    elif isinstance(expr, tuple):
        if len(expr) == 3:
            a, b, c = expr
            if isinstance(b, tuple):
                b = convert_expr_helper(b, buf)
            if isinstance(c, tuple):
                c = convert_expr_helper(c, buf)
            if a in OP_TEXT and isinstance(b, str) and isinstance(c, str):
                b = b.replace('~', '-')
                c = c.replace('~', '-')
                new_var = random_variable()
                if isfloat(b) and isfloat(c):
                    print(b) 
                    print(c)
                    raise ConversionError("Two numbers multiplied together, shouldn't happen")
                elif isfloat(b):
                    output_string = "Ciphertext {}({}); {}_plain({}, encoder.encode({})); ".format(new_var, c, OP_TEXT[a], new_var, b)
                    buf.append(output_string)
                elif isfloat(c):
                    output_string = "Ciphertext {}({}); {}_plain({}, encoder.encode({})); ".format(new_var, b, OP_TEXT[a], new_var, c)
                    buf.append(output_string)
                else:
                    output_string = "Ciphertext {}({}); {}({}, {}); ".format(new_var, b, OP_TEXT[a], new_var, c)
                    buf.append(output_string)
                return new_var


TEST_LINES="""
y = 0.299      * r + 0.587     * g + 0.114     * b  -  128.0;
u =  (r * -0.168736) - (0.331264  * g) + (0.5       * b);
v =  0.5       * r - 0.418688  * g - 0.081312  * b;
"""

PREFIX = "boaz"
counter = 0;
def random_variable():
    global counter
    counter += 1 
    return "{}{}".format(PREFIX, counter)


if __name__ == '__main__':
    for line in TEST_LINES.splitlines():
        try:
            print(convert_expr(parse_line(line)))
        except:
            print("ERROR")
    
