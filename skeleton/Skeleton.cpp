#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


using namespace llvm;

// remember to use -fno-discard-value-names flag

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}
    int uid{0};

    virtual bool runOnFunction(Function &F) {
      if (F.getName()=="willInject"){
          return false;
      }
      LLVMContext &context = F.getParent()->getContext();
      for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
      {
        BasicBlock &BB = *I;
        if (BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator())){
          if(BI->isConditional()){
            BasicBlock *TrueDest = BI->getSuccessor(0);
            BasicBlock *FalseDest = BI->getSuccessor(1);
            if(isErrorHandlingBlock(TrueDest)){
              insertFunc(BB, BI, context, TrueDest, FalseDest);
              break;
            }
            else if(isErrorHandlingBlock(FalseDest)){
              insertFunc(BB, BI, context, FalseDest, TrueDest);
              break;
            }
          }
        }
      }
      return false;
    }

    void insertFunc(BasicBlock &BB, BranchInst *BI, LLVMContext &context, BasicBlock* ehc, BasicBlock* nehc){
      FunctionType *type = FunctionType::get(Type::getInt32Ty(context), {Type::getInt32PtrTy(context)}, false);
      auto callee = BB.getModule()->getOrInsertFunction("willInject", type);
      Function *c=(Function*)callee.getCallee();

      IRBuilder<> builder(&BB);
      ConstantInt *cuid = builder.getInt32((this->uid)++);

      CallInst *inst = CallInst::Create(callee, {cuid}, "",BI);

      BranchInst* toEHC=BranchInst::Create(ehc);
      Instruction* inst_t=SplitBlockAndInsertIfThen((Value*) inst, BI, true);
      ReplaceInstWithInst(inst_t,toEHC);

    }

    bool isErrorHandlingBlock(BasicBlock *BB){
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I){
        if (BranchInst *BI = dyn_cast<BranchInst>(I)){
          if (!BI->isConditional()) {
            if(BI->getOperand(0)->getName() == "fail"){
              errs()<<"---------------find one fault-----------------\n";
              return true;
            }
          }
        }
      }
      return false;
    }
  };
}

char SkeletonPass::ID = 0;

static void registerSkeletonPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new SkeletonPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerSkeletonPass);
