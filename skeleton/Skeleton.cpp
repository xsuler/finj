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
#include "llvm/Transforms/Instrumentation/AddressSanitizer.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/BinaryFormat/MachO.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Comdat.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/InstrTypes.h"
#include <fstream>

using namespace llvm;

// remember to use -fno-discard-value-names flag

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {
    }


    virtual bool runOnFunction(Function &F) {
      if (F.getName()=="willInject"||F.getName()=="add_cov"){
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
	      std::ofstream fileof;
	      fileof.open("/root/faultfile",std::ios_base::app);
	      fileof<<F.getParent()->getSourceFileName()<<"\n";
	      fileof.close();


              break;
            }
            else if(isErrorHandlingBlock(FalseDest)){
              insertFunc(BB, BI, context, FalseDest, TrueDest);
              std::ofstream fileof;
	      fileof.open("/root/faultfile",std::ios_base::app);
	      fileof<<F.getParent()->getSourceFileName()<<"\n";
	      fileof.close();

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

      std::ifstream uidf;
      uidf.open("/root/fuid");
      int uid;
      uidf>>uid;
      uidf.close();

      std::ofstream uidof;
      uidof.open("/root/fuid");
      uidof<<uid+1;
      uidof.close();


      errs()<<"---------------find one fault: "<<uid<< " -----------------\n";
      IRBuilder<> builder(&BB);
      ConstantInt *cuid = builder.getInt32(uid);

      CallInst *inst = CallInst::Create(callee, {cuid}, "",BI);

      BranchInst* toEHC=BranchInst::Create(ehc);
      Instruction* inst_t=SplitBlockAndInsertIfThen((Value*) inst, BI, true);
      ReplaceInstWithInst(inst_t,toEHC);

    }

            //GlobalVariable* file= builder.CreateGlobalString();

    bool isErrorHandlingBlock(BasicBlock *BB){
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I){
        if (BranchInst *BI = dyn_cast<BranchInst>(I)){
          if (!BI->isConditional()) {
            if(BI->getOperand(0)->getName().startswith("fail")){
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
