/*
 * Clang LibTooling Example
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

const string type_literal[][2] = {
    {"int", "%d"},   {"long", "%ld"}, {"long long", "%lld"}, {"double", "%lf"},
    {"float", "%f"}, {"char", "%c"},  {"string", "%s"}};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory
    SYSSEC_COMPILER_WORKSHOP("SYSSEC-CLANG-WRAPPER");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

static llvm::cl::opt<bool>
    anyWrap("wrap", llvm::cl::desc("Do you want to wrap a function?"),
            llvm::cl::Required, llvm::cl::cat(SYSSEC_COMPILER_WORKSHOP));

static llvm::cl::opt<std::string>
    wrapPrefix("wrap-prefix",
               llvm::cl::desc("Select the prefix of the wrapper."),
               llvm::cl::Optional, llvm::cl::cat(SYSSEC_COMPILER_WORKSHOP));

static llvm::cl::opt<std::string>
    wrapMethod("wrap-target", llvm::cl::desc("Name the function to wrap."),
               llvm::cl::Optional, llvm::cl::cat(SYSSEC_COMPILER_WORKSHOP));

// A help message for this specific tool can be added afterwards.
static llvm::cl::extrahelp MoreHelp("\nA Clang Libtool to create a wrapper for "
                                    "a function to show its input values\n");

// Called when the Match registered for it was successfully found in the AST
class SYSSECWrapper : public MatchFinder::MatchCallback {
private:
  string getTypeLiteral(string index) {
    for (unsigned int i = 0; i < sizeof(type_literal) / sizeof(type_literal[0]);
         i++) {
      if (type_literal[i][0] == index) {
        return type_literal[i][1];
      }
    }
    return "%p";
  }

public:
  SYSSECWrapper(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  // matched with main method
  // wrap the main method to avoid return to system while there is an indirect
  // call inside the main
  virtual void run(const MatchFinder::MatchResult &Result) {
    // This class handles loading and caching of source files into memory
    SourceManager &sourceManager = Result.Context->getSourceManager();
    // Represents a function declaration or definition
    const FunctionDecl *func =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("wrapFunc");
    if (func->hasBody()) {
      string retType = func->getReturnType().getAsString();
      unsigned int paramNum = func->getNumParams();
      string funcParamSignature = "";
      string funcBody = "";
      string argString = "";
      for (unsigned int i = 0; i < paramNum; i++) {
        funcParamSignature += func->getParamDecl(i)->getType().getAsString() +
                              " " + func->getParamDecl(i)->getName().str();
        argString += func->getParamDecl(i)->getName().str();
        funcBody +=
            "    printf(\"" +
            getTypeLiteral(func->getParamDecl(i)->getType().getAsString()) +
            "\\n\", " + func->getParamDecl(i)->getName().str() + ");\n";
        if (i < paramNum - 1) {
          funcParamSignature += ", ";
          argString += ", ";
        }
      }

      funcBody += "    return " + wrapMethod + "(" + argString + ");";

      SourceLocation TARGET_END = func->getEndLoc().getLocWithOffset(1);
      std::stringstream wrapFunction;
      string wrapFunctionName = wrapPrefix + "_" + wrapMethod;
      wrapFunction << "\n" + retType + " " + wrapFunctionName + +"(" +
                          funcParamSignature + ")\n{\n" + funcBody + "\n}";
      Rewrite.InsertText(TARGET_END, wrapFunction.str(), true, true);
    }
  }

private:
  Rewriter &Rewrite;
};

// Called when the Match registered for it was successfully found in the AST
class SYSSECRedirect : public MatchFinder::MatchCallback {
private:
public:
  SYSSECRedirect(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  // matched with main method
  // wrap the main method to avoid return to system while there is an indirect
  // call inside the main
  virtual void run(const MatchFinder::MatchResult &Result) {
    SourceManager &sourceManager = Result.Context->getSourceManager();
    // Represents a function declaration or definition
    const CallExpr *cexpr =
        Result.Nodes.getNodeAs<clang::CallExpr>("callMatched");
    string wrapFunctionName = wrapPrefix + "_" + wrapMethod;

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
    Matcher.addMatcher(functionDecl(hasName(wrapMethod)).bind("wrapFunc"),
                       &handleWrapper);
    Matcher.addMatcher(
        callExpr(callee(functionDecl(hasName(wrapMethod)))).bind("callMatched"),
        &handleRedirect);
  }

  // call the ASTMatch
  void HandleTranslationUnit(ASTContext &Context) override {
    Matcher.matchAST(Context);
  }

private:
  SYSSECWrapper
      handleWrapper; // This is our handler when a match found, it has
                     // rewriter instance to modify source in matched code
  SYSSECRedirect handleRedirect;

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
parse the command line options and create a libtool
*/
int main(int argc, const char **argv) {
  /// CommonOptionsParser constructor will parse arguments and create a
  // CompilationDatabase.  In case of error it will terminate the program.
  CommonOptionsParser op(argc, argv, SYSSEC_COMPILER_WORKSHOP);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  if (anyWrap) {
    if (wrapMethod.length()) {
      llvm::errs() << "The target wrap function: " << wrapMethod << "\n";
      if (wrapPrefix.length()) {
        llvm::errs() << "Prefix (User): " << wrapPrefix << "\n";
      } else {
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