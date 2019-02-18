#Clang Libtool
Technically, Clang receives an Abstract Syntax Tree (AST), build by Clang Parser (clang/Parse/*), not the input C/C++ code (although Parser is part of Clang code base). There is obviously a Lexer in between this process, but neither Lexer nor Parser is our focus in this tutorial. Clang is responsible to convert the AST to LLVM IR which is implemented in Clang CodeGen. Once source code is translated to LLVM IR, all instrumentation have to be in LLVM IR. It is certainly easy to instrument in fine-grain language like LLVM-IR then complicated top-level source code. But in certain cases (e.g. code formatter, code documentation etc.), it is expected to modify the source code (overwrite the source file).

**Clang Libtool** is an API to modify the AST while it is in buffer and write back it to the source file in input source language. So, a developer can insert/remove/modify any node in AST by the API functionality and all others will be handled by the libtool. This design gives the plugin developer flexibility to focus on specific purpose. Besides good understanding of C++ coding, a developer will require following knowledge:

* AST tree structure, understands their nodes including their attributes
* Basic code structure of Clang Libtool
* AST matcher (this is optional but it gives more flexibility in tool development)

*Note: Some may face various issues working on Clang Libtool. As an example: Clang Libtool only converts source code to AST but does not expand code with macros.*

## Writeup Explanation

In this write-up, we will write a Clang Libtool that will insert a different version of a target function in source code and replace all the call invokation of original function to the new function. Additionally, the newly defined function will print all the params value in it. Consider the following source code:

```C
int doSum(int a, int b){
    int sum;
    sum = a + b;
    return sum;
}
int main(){
    return doSum(10, 20);
}
```

The above code is a simple C code which have a method `doSum()` that accepts two integers, sum them, store it in a local integer variable, and finally returns it as integer. The function can be called from multiple places with different arguments (e.g. here inside `main()`). Let's consider, we want to print the params received by `doSum()` for every invokation and we do not want to modify the original function. So, we write a different version of the method just after the original:

```C
int doSum(int a, int b){
    int sum;
    sum = a + b;
    return sum;
}
int Wrap_doSum(int a, int b){
    printf("a = %d\n", a);
    printf("b = %d\n", b);
    return doSum(a, b);
}
int main(){
    return Wrap_doSum(10, 20);
}
```

**As we have mentioned earlier, the only user input will be the target function name, in this case `doSum`. But, the user may also input a prefix for the wrapper method.**

To achieve this, we have to insert a new function with two `printf()` for the two params and also call the original target function and return its results back to the call-site. Besides that, it will also have to replace every call invokation of the target function with the newly created function. We will split this job into following tasks:

* Identify the target function.
* Identify the function params and their types.
* Create a function (prefix + original name) just after the target function.
* Create the function body with two printf() to print the params value.
* Append it with call to target function with a return.
* Identify every call to the target function and redirect them to the newly created function.

## Abstract Syntax Tree

**Let's start with step 1:** understanding the AST. We use the following command to dump the AST and redirect it to a file:

```txt
clang-check -ast-dump target_test.c --extra-arg="-fno-color-diagnostics" -- > ast.out
```
We use the clang-check to do a basic error check in the input source and the -ast-dump to dump the AST. We add an extra argument to discard any format styler in AST (in plain file they don't work). The full file may look scary but the only part we want to look into is where the parser generates AST for `doSum()` and `main()`.

```txt
|-FunctionDecl 0xc778450 <../target_test.c:3:1, line:7:1> line:3:5 used doSum 'int (int, int)'
| |-ParmVarDecl 0xc778300 <col:11, col:15> col:15 used a 'int'
| |-ParmVarDecl 0xc778378 <col:18, col:22> col:22 used b 'int'
| `-CompoundStmt 0xc7786a0 <col:24, line:7:1>
|   |-DeclStmt 0xc778570 <line:4:5, col:12>
|   | `-VarDecl 0xc778510 <col:5, col:9> col:9 used sum 'int'
|   |-BinaryOperator 0xc778638 <line:5:5, col:15> 'int' '='
|   | |-DeclRefExpr 0xc778588 <col:5> 'int' lvalue Var 0xc778510 'sum' 'int'
|   | `-BinaryOperator 0xc778618 <col:11, col:15> 'int' '+'
|   |   |-ImplicitCastExpr 0xc7785e8 <col:11> 'int' <LValueToRValue>
|   |   | `-DeclRefExpr 0xc7785a8 <col:11> 'int' lvalue ParmVar 0xc778300 'a' 'int'
|   |   `-ImplicitCastExpr 0xc778600 <col:15> 'int' <LValueToRValue>
|   |     `-DeclRefExpr 0xc7785c8 <col:15> 'int' lvalue ParmVar 0xc778378 'b' 'int'
|   `-ReturnStmt 0xc778690 <line:6:5, col:12>
|     `-ImplicitCastExpr 0xc778678 <col:12> 'int' <LValueToRValue>
|       `-DeclRefExpr 0xc778658 <col:12> 'int' lvalue Var 0xc778510 'sum' 'int'
`-FunctionDecl 0xc778720 <line:8:1, line:10:1> line:8:5 main 'int ()'
  `-CompoundStmt 0xc7788a0 <col:11, line:10:1>
    `-ReturnStmt 0xc778890 <line:9:5, col:24>
      `-CallExpr 0xc778860 <col:12, col:24> 'int'
        |-ImplicitCastExpr 0xc778848 <col:12> 'int (*)(int, int)' <FunctionToPointerDecay>
        | `-DeclRefExpr 0xc7787b8 <col:12> 'int (int, int)' Function 0xc778450 'doSum' 'int (int, int)'
        |-IntegerLiteral 0xc7787d8 <col:18> 'int' 10
        `-IntegerLiteral 0xc7787f8 <col:22> 'int' 20
```

AST is afterall a regular tree structure with nodes and every node is derived from its parent node. In this code, we can see there are two siblings node in top-level. They are both `FunctionDecl` for two functions in user source code. We can notice the name of the function with its signature in this format: `func_name 'return_type (param_type, param_type)'`. Next, we can see `ParmVarDecl` for every param where they also have the variable naming with respective type. Finally, a `CompoundStmt` starts which is the function body that ends up with a `ReturnStmt`. For `doSum()`, there are more than `ReturnStmt` under the `CompoundStmt` e.g. we can see a `BinaryOperator` for `=` operator in `sum = a + b;` followed by a `DeclRefExpr` indicates the lValue `sum` and another `BinaryOperator` for the `+` operation that is also followed by two other `DeclRefExpr` for variable `a` and `b`, each of them are derived from `ImplicitCastExpr` to explain that they have to be converted to rValue from the lValue. Another important node for this job is the `CallExpr` in `main()`. We can see it has two `IntegerLiteral` which defines the call's two arguments.