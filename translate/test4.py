import re


pattern = re.compile('[\.\[\]a-zA-Z\d]+')
OPS = ['+', '-', '*', '/']


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
    
    print(expr)
    while len(expr) > 1:
        ind = 0
        for i in range(len(expr)):
            if expr[i] in OPS:
                ind = i
                break
        print(ind)
        if ind < 2:
            return 'ERROR'
        chunk = (expr[ind], expr[ind - 2], expr[ind - 1])
        expr = expr[:ind - 2] + [chunk] + expr[ind + 1:]
    return expr[0]

def parse_line(line):
    line = line.replace(' ', '').replace(';', '').split('=')
    dst = line[0]
    args_op = line[1]
    if dst[-1] in OPS:
        args_op = dst + '(' + args_op + ')'
        dst = dst[:-1]
    print(args_op)
    args_op = re.sub(r'\[(.*?)\]', '[]', args_op)
    args_op = args_op.replace('(', ' ( ')
    args_op = args_op.replace(')', ' ) ')
    args_op = args_op.replace('+', ' + ')
    args_op = args_op.replace('-', ' - ')
    args_op = args_op.replace('*', ' * ')
    args_op = args_op.replace('/', ' / ')
    print(args_op)
    return (dst, postfix_to_tree(infixToPostfix(args_op)))



print(parse_line("tmp7 = data_ptr[8*0] - data_ptr[8*7];"))
