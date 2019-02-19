/*
 * Clang Libtool Sample for SYSSEC-FSU Workshop
 * Author: Mustakimur Rahman Khandaker
 * Affliation: Florida State University
 */
#include <sstream>
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;

const string type_format_specifier[][2] = {
    {"int", "%d"},   {"long", "%ld"}, {"long long", "%lld"}, {"double", "%lf"},
    {"float", "%f"}, {"char", "%c"},  {"string", "%s"}};

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

/*
SYSSECWrapper is responsible to create new wrap function and its body
*/
class SYSSECWrapper : public MatchFinder::MatchCallback {
private:
  // a simple method to return format_specifier for different types
  string getFormatSpecifier(string index) {
    for (unsigned int i = 0;
         i < sizeof(type_format_specifier) / sizeof(type_format_specifier[0]);
         i++) {
      if (type_format_specifier[i][0] == index) {
        return type_format_specifier[i][1];
      }
    }
    return "%p";
  }

public:
  SYSSECWrapper(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  // AST matcher match the function declaration with target function
  virtual void run(const MatchFinder::MatchResult &Result) {
    // you can use sourceManager to print debug information about sourceLocation
    // SourceManager &sourceManager = Result.Context->getSourceManager();

    // retrieve the matched function declaration
    const FunctionDecl *func =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("wrapFunc");

    // if function has a body
    if (func->hasBody()) {
      // collect the function return type
      string retType = func->getReturnType().getAsString();
      // collect number of params in the function
      unsigned int paramNum = func->getNumParams();

      // we create text for function body and signature
      string funcParamSignature = "";
      string funcBody = "";
      // we create text for call the target function from wrap function
      string argString = "";

      for (unsigned int i = 0; i < paramNum; i++) {
        // param_type param_name
        funcParamSignature += func->getParamDecl(i)->getType().getAsString() +
                              " " + func->getParamDecl(i)->getName().str();
        // argument_name
        argString += func->getParamDecl(i)->getName().str();
        if (i < paramNum - 1) {
          funcParamSignature += ", ";
          argString += ", ";
        }

        // creates a printf for every param to print their value
        string format_specifier =
            getFormatSpecifier(func->getParamDecl(i)->getType().getAsString());
        string pName;
        if (format_specifier == "%p") {
          // if not specified in format specifier array, then will print the
          // address
          pName = "&" + func->getParamDecl(i)->getName().str();
        } else {
          pName = func->getParamDecl(i)->getName().str();
        }
        funcBody +=
            "    printf(\"" + format_specifier + "\\n\", " + pName + ");\n";
      }
      // at the end of function body, we call the target function with return
      // statement
      funcBody += "    return " + targetMethod + "(" + argString + ");";

      // the target function end point is the '}', so we ask for +1 offset
      SourceLocation TARGET_END = func->getEndLoc().getLocWithOffset(1);
      std::stringstream wrapFunction;
      string wrapFunctionName = wrapPrefix + "_" + targetMethod;
      // we create the entire wrap function text
      wrapFunction << "\n" + retType + " " + wrapFunctionName + +"(" +
                          funcParamSignature + ")\n{\n" + funcBody + "\n}";
      // let's insert the wrap function at the end of target function
      Rewrite.InsertText(TARGET_END, wrapFunction.str(), true, true);
    }
  }

private:
  Rewriter &Rewrite;
};

/*
SYSSECRedirect is responsible to redirect call to target function to wrap
function
*/
class SYSSECRedirect : public MatchFinder::MatchCallback {
private:
public:
  SYSSECRedirect(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  // AST matcher match the call expression with callee to target function
  virtual void run(const MatchFinder::MatchResult &Result) {
    // retrieve the matched call expression
    const CallExpr *cexpr =
        Result.Nodes.getNodeAs<clang::CallExpr>("callMatched");

    string wrapFunctionName = wrapPrefix + "_" + targetMethod;
    // let's replace the callee from call expression with the wrap function
    Rewrite.ReplaceText(cexpr->getCallee()->getSourceRange(), wrapFunctionName);
  }

private:
  Rewriter &Rewrite;
};

// ASTConsumer: This is an abstract interface that should be implemented by
// clients that read ASTs
class SYSSECASTConsumer : public ASTConsumer {
public:
  SYSSECASTConsumer(Rewriter &R) : handleWrapper(R), handleRedirect(R) {
    // we ask AST to match the function declaration with the target function
    // name, if so, callback the hangleWrapper
    Matcher.addMatcher(functionDecl(hasName(targetMethod)).bind("wrapFunc"),
                       &handleWrapper);

    // we ask AST to match the call expression with target function as callee
    // and if so, callback the handleRedirect
    Matcher.addMatcher(callExpr(callee(functionDecl(hasName(targetMethod))))
                           .bind("callMatched"),
                       &handleRedirect);
  }

  // call the ASTMatch
  void HandleTranslationUnit(ASTContext &Context) override {
    Matcher.matchAST(Context);
  }

private:
  SYSSECWrapper handleWrapper; // This is our handler to create the new wrap
                               // function with required instructions inside
  SYSSECRedirect
      handleRedirect; // This is our handler to replace callExpr with target
                      // function to callExpr with wrap function

  MatchFinder Matcher; // A class to allow finding matches over the Clang AST
};

// ASTFrontendAction: Abstract base class to use for AST consumer-based
// frontend actions
class SYSSECFrontEndAction : public ASTFrontendAction {
public:
  SYSSECFrontEndAction() {}
  // rewrite done at the end of source file processing
  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

  // Create the AST consumer object for this action
  // CI - The current compiler instance
  // file - The current input file
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<SYSSECASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter; // This is the main interface to the rewrite buffers
};

/*
parse the command line options and instantiate the libtool
*/
int main(int argc, const char **argv) {
  /// CommonOptionsParser constructor will parse arguments and create a
  // CompilationDatabase.  In case of error it will terminate the program.
  CommonOptionsParser op(argc, argv, SYSSEC_COMPILER_WORKSHOP);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

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

  // The ClangTool needs a new FrontendAction for each translation unit we run
  // on.  Thus, it takes a FrontendActionFactory as parameter.  To create a
  // FrontendActionFactory from a given FrontendAction type, we call
  // newFrontendActionFactory<SYSSECFrontEndAction>().
  return Tool.run(newFrontendActionFactory<SYSSECFrontEndAction>().get());
}