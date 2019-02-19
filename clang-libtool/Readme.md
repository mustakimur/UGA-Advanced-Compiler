# Clang Libtool
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

## Workspace Structure
It is a standard procedure to build the Clang Libtool from the Clang tools directory (i.e. llvm/tools/clang/tools/). So, we create a directory (clang-wrapper) for the tool and add the following line at the end of CMakeLists.txt (in llvm/tools/clang/tools/). 

```txt
add_clang_subdirectory(clang-wrapper)
```

Inside our tool workspace, we will have another CMakeLists.txt which usually looks like following:

```txt
cmake_minimum_required(VERSION 2.8.8)
project(syssec-workshop)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
set(LLVM_LINK_COMPONENTS support)

add_clang_executable(clang-wrapper
  wrap-method.cpp
  )
target_link_libraries(clang-wrapper
  clangTooling
  clangBasic
  clangASTMatchers
  )
install(TARGETS clang-wrapper RUNTIME DESTINATION bin)
```

At the beginning, it ensures the cmake minimum version to build this tool. Then we give a project name (in this case, syssec-workshop). Next, `include_directories` is for the clang source code path. The `set` basic LLVM link support.

The `add_clang_executable` is important, we first give the executable name (or tool name) and than the list of dependent source code to compile. The `target_link_libraries` is also important which mentions what clang runtime support will be required for this tool (i.e. clang-wrapper). We will use clangTooling (libtool support), clangBasic (clang basic support for user input handling), and clangASTMatchers (for clang ASTMatcher api). `install` will set the path where to install the tool.

Next to the CMakeLLists.txt, we should add the C++ source code for the libtool (we will have one i.e. wrap-method.cpp). Once we have everything ready to compile, we can build the tool the same way we build the clang/llvm from its build directory. The libtool will be available in the build directory (i.e. build/bin/).

## Basic Code Structure
The code structure of a Clang libtool can be divided into four parts. We definitely require a main function which will process command line user input and prepare the next phase. The next phase is  responsible to preapare the Rewriter (metaphore a pen) for the Compiler processed AST buffer. In the third step, we will prepare the AST matcher (metaphore pattern recognization engine). Finally, we will write handler to use the Rewritter for matched AST.

**Command Line Parser:** Inside the `main()`, we process the command line options. We can do extra verfication on user input here. It also creates the ClangTool instance and runs the tool with a customized `ASTFrontendAction`. Libtool API use the factory design pattern to return the instance of the customized `ASTFrontendAction`.

**SYSSECFrontEndAction:** This class extends from `ASTFrontendAction` where we can override the `CreateASTConsumer()` and prepare the `Rewriter` for the AST buffer. Later, we subvert the control flow to our customized `ASTConsumer`. Besides that, we also override `EndSourceFileAction()` to inform the compiler to commit the change in buffer to source file at the end of process.

**SYSSECASTConsumer:** This class extends from `ASTConsumer` where we can finally have the `ASTContext` and define-use the `MatchFinder`. In its private member, we have the `MatchFinder` and two handlers. In the class constructor, we define the match pattern and set their respective callback handler. We override `HandleTranslationUnit()` to request the compiler to start the `MatchFinder` process.

**Handlers:** The handlers are automatically called when `MatchFinder` finds a match. We have two handler, first one to create wrap function immediate to target function with identical function signature, the later one redirect every call expression to original target function to the wrap function.

So, overall the code structure looks like:
```C++
class SYSSECWrapper : public MatchFinder::MatchCallback {
public:
  SYSSECWrapper(Rewriter &Rewrite) : Rewrite(Rewrite) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    // action for the matched pattern
  }
private:
  Rewriter &Rewrite;
};
class SYSSECRedirect : public MatchFinder::MatchCallback {
private:
public:
  SYSSECRedirect(Rewriter &Rewrite) : Rewrite(Rewrite) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    // action for the matched pattern
  }
private:
  Rewriter &Rewrite;
};
class SYSSECASTConsumer : public ASTConsumer {
public:
  SYSSECASTConsumer(Rewriter &R) : handleWrapper(R), handleRedirect(R) {
    // define MatchFinder pattern
  }
  void HandleTranslationUnit(ASTContext &Context) override {
    Matcher.matchAST(Context);
  }

private:
  SYSSECWrapper handleWrapper;
  SYSSECRedirect handleRedirect;
  MatchFinder Matcher;
};
class SYSSECFrontEndAction : public ASTFrontendAction {
public:
  SYSSECFrontEndAction() {}
  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<SYSSECASTConsumer>(TheRewriter);
  }
private:
  Rewriter TheRewriter;
};
int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, SYSSEC_COMPILER_WORKSHOP);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  // process command line option
  return Tool.run(newFrontendActionFactory<SYSSECFrontEndAction>().get());
}
```

Notice, once we have defined `Rewriter` in `SYSSECFrontEndAction`, we always carry it in to the upper steps of the code structure.

## Command Line Option
In the beginning of the source code (global space), we have defined multiple `cl` fields for different command-line operations.
```C++
// creates a option category to show what functionality available in this tool
// try clang-wrapper -help
static llvm::cl::OptionCategory
    SYSSEC_COMPILER_WORKSHOP("SYSSEC-CLANG-WRAPPER");

// creates multiple options to feed user inputs
// -wrap takes true/false and required
static llvm::cl::opt<bool>
    wFlag("wrap", llvm::cl::desc("Do you want to wrap a function?"),
          llvm::cl::Required, llvm::cl::cat(SYSSEC_COMPILER_WORKSHOP));
// -wrap-prefix takes a string and optional
static llvm::cl::opt<std::string>
    wrapPrefix("wrap-prefix",
               llvm::cl::desc("Select the prefix of the wrapper."),
               llvm::cl::Optional, llvm::cl::cat(SYSSEC_COMPILER_WORKSHOP));
// -wrap-prefix takes a string and optional
static llvm::cl::opt<std::string>
    targetMethod("wrap-target", llvm::cl::desc("Name the function to wrap."),
                 llvm::cl::Optional, llvm::cl::cat(SYSSEC_COMPILER_WORKSHOP));

// it is possible to show enhance message about the tool
static llvm::cl::extrahelp MoreHelp("\nA Clang Libtool to create a wrapper for "
                                    "a function to show its input values\n");
```

Inside the `main()`, we can do further sanitization on user provided command line input.

```C++
  // we can do simple extra user input validation
  if (wFlag) {
    // we atleast need to know the target function name
    if (targetMethod.length()) {
      llvm::errs() << "The target wrap function: " << targetMethod << "\n";
      if (wrapPrefix.length()) {
        llvm::errs() << "Prefix (User): " << wrapPrefix << "\n";
      } else {
        // this is default prefix if user does not provide
        wrapPrefix = "syssec";
        llvm::errs() << "Prefix (Default): " << wrapPrefix << "\n";
      }
    } else {
      llvm::errs() << "Please, input a target function name.\n";
      return 0;
    }
  }
```

As an independent compiler tool, user interaction is important and libtool gives enough flexibility to handle them.

## AST Matcher
We have AST matcher to match two different patterns: 1) function declaration of target function, 2) call expression with target function as callee.

To match following AST node:

```text
|-FunctionDecl 0xc778450 <../target_test.c:3:1, line:7:1> line:3:5 used doSum 'int (int, int)'
```

We have following straightforward matcher:

```C++
    // we ask AST to match the function declaration with the target function
    // name, if so, callback the hangleWrapper
    Matcher.addMatcher(functionDecl(hasName(targetMethod)).bind("wrapFunc"),
                       &handleWrapper);
```

The `.bind(tag)` basically tag the findings so that we can use the tag later to retrieve the instance of matched pattern.

To match the following AST:

```text
-CallExpr 0xc778860 <col:12, col:24> 'int'
        |-ImplicitCastExpr 0xc778848 <col:12> 'int (*)(int, int)' <FunctionToPointerDecay>
        | `-DeclRefExpr 0xc7787b8 <col:12> 'int (int, int)' Function 0xc778450 'doSum' 'int (int, int)'
```

We write the following matcher:

```C++
    // we ask AST to match the call expression with target function as callee
    // and if so, callback the handleRedirect
    Matcher.addMatcher(callExpr(callee(functionDecl(hasName(targetMethod))))
                           .bind("callMatched"),
                       &handleRedirect);
```

This may seem confusing because there is no function declaration under call expression. Basically, `callExpr(callee)` returns the `DeclRefExpr` which we have fine-grained by `functionDecl` as callee must be a functionDecl (it could be cxxMethodDecl for C++ code).

Details about AST matcher is available here: https://clang.llvm.org/docs/LibASTMatchersReference.html

