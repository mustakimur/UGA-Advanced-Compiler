#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Bitcode/BitcodeReader.h> /// for isBitcode
#include <llvm/IRReader/IRReader.h>     /// for isIRFile

#include <map>
#include <set>
#include <vector>

using namespace llvm;

typedef std::set<Function *>
    AddrTakenFnList; // list of function which are address-taken
typedef AddrTakenFnList::iterator AddrTakenFnListIt; // list iterator

typedef std::vector<Type *> FnParamSignature; // function params type list
typedef std::pair<Type *, FnParamSignature>
    FnSignaure; // pair<return_type, function_params_type_list>
typedef std::map<Function *, FnSignaure>
    FnSignaureMap; // map<function, function_complete_signature>

class CICFI : public ModulePass {
private:
  // Let's check inner items of a structure
  void innerItems(ConstantStruct *cStruct) {
    StructType *str = cStruct->getType();
    unsigned int nn = str->getNumElements();

    for (unsigned int i = 0; i < nn; i++) {
      Constant *item = cStruct->getAggregateElement(i)->stripPointerCasts();
      if (isa<Function>(item)) {
        Function *fn = dyn_cast<Function>(item);
        fnList.insert(fn);
      } else if (isa<ConstantStruct>(item)) {
        ConstantStruct *cstr = dyn_cast<ConstantStruct>(item);
        innerItems(cstr);
      } else if (isa<ConstantArray>(item)) {
        ConstantArray *carr = dyn_cast<ConstantArray>(item);
        innerItems(carr);
      }
    }
  }

  // Let's check inner items of an array
  void innerItems(ConstantArray *cArray) {
    ArrayType *arr = cArray->getType();
    unsigned int nArr = arr->getNumElements();

    for (unsigned int i = 0; i < nArr; i++) {
      Constant *item = cArray->getAggregateElement(i)->stripPointerCasts();
      if (isa<Function>(item)) {
        Function *fn = dyn_cast<Function>(item);
        fnList.insert(fn);
      } else if (isa<ConstantStruct>(item)) {
        ConstantStruct *cstr = dyn_cast<ConstantStruct>(item);
        innerItems(cstr);
      } else if (isa<ConstantArray>(item)) {
        ConstantArray *carr = dyn_cast<ConstantArray>(item);
        innerItems(carr);
      }
    }
  }

public:
  static char ID;
  CICFI() : ModulePass(ID) {}

  virtual inline void getAnalysisUsage(llvm::AnalysisUsage &au) const {
    // declare your dependencies here.
    /// do not intend to change the IR in this pass,
    au.setPreservesAll();
  }

  bool runOnModule(Module &M) override {
    // Let's first check the all the global variables
    // Note: function local array is also part of global variables, just their
    // attribute set as private
    for (Module::global_iterator gIt = M.global_begin(); gIt != M.global_end();
         ++gIt) {
      if (isa<GlobalVariable>(gIt)) {
        GlobalVariable *gVar = dyn_cast<GlobalVariable>(gIt);
        // check the global variable if it has initializer
        if (gVar->hasInitializer()) {
          Constant *items = gVar->getInitializer();
          // a global variable could be an array, structure or a plain constant
          if (isa<ConstantArray>(items)) {
            ConstantArray *carr = dyn_cast<ConstantArray>(items);
            // check array inner items
            innerItems(carr);
          } else if (isa<ConstantStruct>(items)) {
            ConstantStruct *cstr = dyn_cast<ConstantStruct>(items);
            // check structure inner fields
            innerItems(cstr);
          } else if (isa<Constant>(items)) {
            Constant *item = dyn_cast<Constant>(items);
            // if the item is a Function, it is an address-taken
            if (isa<Function>(item)) {
              Function *fn = dyn_cast<Function>(item);
              fnList.insert(fn);
            }
          }
        }
      }
    }

    // Let's check if any instruction is address-taken
    for (Function &Fn : M) {
      for (BasicBlock &BB : Fn) {
        for (Instruction &Inst : BB) {
          Instruction *inst = &Inst;
          // the instruction either a store instruction or a call/invoke
          // (CallBase) instruction
          if (isa<StoreInst>(inst)) {
            StoreInst *store = dyn_cast<StoreInst>(inst);
            Value *vop = store->getValueOperand()->stripPointerCasts();
            if (isa<Function>(vop)) {
              Function *fn = dyn_cast<Function>(vop);
              fnList.insert(fn);
            }
          } else if (isa<CallBase>(inst)) {
            CallBase *call = dyn_cast<CallBase>(inst);
            for (User::op_iterator op = call->arg_begin();
                 op != call->arg_end(); ++op) {
              if (isa<Function>((*op)->stripPointerCasts())) {
                Function *fn = dyn_cast<Function>((*op)->stripPointerCasts());
                fnList.insert(fn);
              }
            }
          }
        }
      }
    }

    // print the address-taken function list
    for (AddrTakenFnListIt it = fnList.begin(); it != fnList.end(); ++it) {
      outs() << (*it)->getName() << "\n";
    }

    return false; // must return true if module is modified
  }

private:
  AddrTakenFnList fnList; // list all address-taken function value
};

char CICFI::ID = 0;
static RegisterPass<CICFI>
    Trans("llvm-ci-cfi",
          "LLVM Context-insensitive Address-taken Type Match CFI");
