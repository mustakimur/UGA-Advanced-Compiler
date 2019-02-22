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
typedef std::map<Function *, FnSignaure>::iterator
    FnSignaureMapIt; // map<function, function_complete_signature>
typedef std::map<unsigned long, std::set<Function *>> CFG;
typedef std::map<unsigned long, std::set<Function *>>::iterator CFGIt;

class CICFI : public ModulePass {
private:
  // Let's check inner items of a structure
  /* void innerItems(ConstantStruct *cStruct) {
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
  } */

  // Let's check inner items of an array
  /* void innerItems(ConstantArray *cArray) {
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
  } */

  void replaceGLBUsage(GlobalVariable *New, GlobalVariable *Old) {
    std::set<User *> users;
    for (Value::user_iterator u = Old->user_begin(); u != Old->user_end();
         ++u) {
      User *user = *u;
      users.insert(user);
    }
    for (std::set<User *>::iterator u = users.begin(); u != users.end(); ++u) {
      User *user = *u;
      if (isa<GetElementPtrInst>(user)) {
        GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(user);
        gep->setSourceElementType(New->getValueType());
        for (unsigned I = 0, E = user->getNumOperands(); I < E; ++I) {
          if (user->getOperand(I) == Old) {
            user->setOperand(I, New);
          }
        }
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
    /*for (Module::global_iterator gIt = M.global_begin(); gIt !=
    M.global_end();
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

            // handle pointer to member function
            if (isa<ConstantAggregate>(vop)) {
              ConstantAggregate *cagr = dyn_cast<ConstantAggregate>(vop);
              if (Constant *cn = cagr->getAggregateElement((unsigned)0)) {
                if (isa<ConstantExpr>(cn)) {
                  ConstantExpr *cexp = dyn_cast<ConstantExpr>(cn);
                  if (isa<PtrToIntOperator>(cexp)) {
                    PtrToIntOperator *opr = dyn_cast<PtrToIntOperator>(cexp);
                    vop = opr->getPointerOperand();
                  }
                }
              }
            }

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
    }*/

    for (Function &Fn : M) {
      if (Fn.hasAddressTaken()) {
        Function *fn = &Fn;
        fnList.insert(fn);
        FnParamSignature fnParams;
        for (Function::arg_iterator it = fn->arg_begin(); it != fn->arg_end();
             ++it) {
          if (isa<Value>(it)) {
            Value *arg = dyn_cast<Value>(it);
            fnParams.push_back(arg->getType());
          }
        }
        FnSignaure cSign = std::make_pair(fn->getReturnType(), fnParams);
        fnSignMap[fn] = cSign;
      }
    }

    // print the address-taken function list
    /*for (AddrTakenFnListIt it = fnList.begin(); it != fnList.end(); ++it) {
      errs() << (*it)->getName() << "\n";
    }*/

    unsigned int maxMatched = 0;
    unsigned long totalICT = 0;
    unsigned long totalAllow = 0;
    for (Function &Fn : M) {
      for (BasicBlock &BB : Fn) {
        for (Instruction &Inst : BB) {
          Instruction *inst = &Inst;
          if (isa<CallBase>(inst)) {
            unsigned long callID = -1;
            CallBase *call = dyn_cast<CallBase>(inst);
            if (call->getCalledFunction() &&
                call->getCalledFunction()->getName() ==
                    "pCall_reference_monitor") {
              Value *idValue = call->getArgOperand(0);
              if (isa<ConstantInt>(idValue)) {
                ConstantInt *cint = dyn_cast<ConstantInt>(idValue);
                callID = cint->getZExtValue();
              }
              if (isa<CallBase>(call->getNextNonDebugInstruction())) {
                call = dyn_cast<CallBase>(call->getNextNonDebugInstruction());
                if (!call->getCalledFunction()) {
                  FnParamSignature fnParams;
                  for (User::op_iterator op = call->arg_begin();
                       op != call->arg_end(); ++op) {
                    if (isa<Value>(op)) {
                      Value *arg = dyn_cast<Value>(op);
                      fnParams.push_back(arg->getType());
                    }
                  }
                  FnSignaure cSign = std::make_pair(call->getType(), fnParams);

                  unsigned int matched = 0;
                  for (FnSignaureMapIt mIt = fnSignMap.begin();
                       mIt != fnSignMap.end(); ++mIt) {
                    if (mIt->second == cSign) {
                      /*errs() << "Inst: " << *inst << " => " <<
                         mIt->first->getName()
                             << "\n";*/
                      cfg[callID].insert(mIt->first);
                      matched++;
                    }
                  }
                  if (maxMatched < matched) {
                    maxMatched = matched;
                  }
                  totalICT++;
                  totalAllow += matched;
                  // errs() << "Inst: " << *inst << " => " << matched << "\n";
                }
              }
            }
          }
        }
      }
    }
    errs() << "Max Matched: " << maxMatched << "\n";
    errs() << "Total ICT: " << totalICT << "\n";
    errs() << "Total Allowed Target: " << totalAllow << "\n";

    PointerType *int32PtTy = Type::getInt32PtrTy(M.getContext());
    IntegerType *int32Ty = Type::getInt32Ty(M.getContext());

    // list the BlockAddress from BasicBlock
    std::vector<Constant *> listBA;
    for (CFGIt it = cfg.begin(); it != cfg.end(); ++it) {
      unsigned long id = it->first;
      for (std::set<Function *>::iterator fit = (it->second).begin();
           fit != (it->second).end(); ++fit) {
        Function *fn = *fit;

        Constant *fnConst =
            ConstantExpr::getCast(Instruction::BitCast, fn, int32PtTy);

        Constant *tag_id = ConstantInt::get(int32Ty, id, false);
        Constant *tag = ConstantFolder().CreateIntToPtr(tag_id, int32PtTy);

        listBA.push_back(tag);
        listBA.push_back(fnConst);
      }
    }
    ArrayRef<Constant *> blockArray(listBA);
    // create the constant type and array
    ArrayType *pArrTy = ArrayType::get(int32PtTy, listBA.size());
    Constant *blockItems = ConstantArray::get(pArrTy, blockArray);

    // Global Variable Declarations
    GlobalVariable *gCFG_old = M.getGlobalVariable("CFG_TABLE");
    gCFG_old->setDSOLocal(false);

    GlobalVariable *gvar_cfg_data = new GlobalVariable(
        M, blockItems->getType(), true, GlobalValue::ExternalLinkage,
        blockItems, "CFG_TABLE");

    replaceGLBUsage(gvar_cfg_data, gCFG_old);

    Constant *cfgLen = ConstantInt::get(int32Ty, listBA.size(), false);
    GlobalVariable *gCFG_len = M.getGlobalVariable("CFG_LENGTH");
    gCFG_len->setInitializer(cfgLen);

    return false; // must return true if module is modified
  }

private:
  AddrTakenFnList fnList; // list all address-taken function value
  FnSignaureMap fnSignMap;
  CFG cfg;
};

char CICFI::ID = 0;
static RegisterPass<CICFI>
    Trans("llvm-ci-cfi",
          "LLVM Context-insensitive Address-taken Type Match CFI");
