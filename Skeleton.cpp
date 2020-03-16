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
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/Value.h"
#include "llvm/MC/MCSectionMachO.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/ScopedPrinter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/Utils/ASanStackFrameLayout.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>


using namespace llvm;


namespace {

  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      if(F.getName()=="mem_to_shadow"||F.getName()=="report_action"||F.getName()=="report_xasan"||F.getName()=="willInject"||F.getName()=="mark_valid"||F.getName()=="mark_invalid"){
	  return false;
     }

      LLVMContext &context = F.getParent()->getContext();
      DataLayout lt=F.getParent()->getDataLayout();
      for (auto &BB : F) {
	  
        for (auto &Inst : BB) {
          bool IsWrite;
          uint64_t TypeSize;
          unsigned Alignment;
          if(isInterestingMemoryAccess(&Inst,&IsWrite,&TypeSize,&Alignment)){

            FunctionType *type = FunctionType::get(Type::getVoidTy(context), {Type::getInt64PtrTy(context),Type::getInt64Ty(context),Type::getInt64Ty(context)}, false);
            auto callee = BB.getModule()->getOrInsertFunction("report_xasan", type);
	    IRBuilder<> builder(&BB);

	    ConstantInt *size = builder.getInt64(TypeSize/8);
	    ConstantInt *iswrite = builder.getInt64(1);
	    ConstantInt *isread = builder.getInt64(0);

        Value* addr=IsWrite?Inst.getOperand(1):Inst.getOperand(0);

	    if(IsWrite)
		    CallInst::Create(callee, {addr,size,iswrite}, "",&Inst);
	    else
		    CallInst::Create(callee, {addr,size,isread}, "",&Inst);
          }

          if(!Inst.getMetadata("isRedZone")){
            MDNode* N = MDNode::get(context, MDString::get(context, "false"));
            Inst.setMetadata("isRedZone",N);
          }

      if(isStaticAlloc(&Inst)){
        if(cast<MDString>(Inst.getMetadata("isRedZone")->getOperand(0))->getString()!="true"){
            errs()<<"one alloc\n";
            IRBuilder<> builder(&BB);

            //start inserting redzone

            Type* it = IntegerType::getInt8Ty(context);
            ArrayType* arrayType = ArrayType::get(it, 32);
            AllocaInst* arr_alloc = new AllocaInst(
                arrayType, 0, "rz" , &Inst);
            MDNode* N = MDNode::get(context, MDString::get(context, "true"));
            arr_alloc->setMetadata("isRedZone",N);
            
            //insert func for redzone
            FunctionType *typer = FunctionType::get(Type::getVoidTy(context), {Type::getInt64PtrTy(context),Type::getInt64Ty(context)}, false);
            auto calleer = BB.getModule()->getOrInsertFunction("mark_invalid", typer);
            long szr=32;
            ConstantInt *sizer = builder.getInt64(szr);
            CallInst::Create(calleer, {arr_alloc,sizer}, "",arr_alloc);
            

            //start inserting redzone

            AllocaInst* arr_alloc_a = new AllocaInst(
                arrayType, 0, "rz1");
            arr_alloc_a->insertAfter(&Inst);
            arr_alloc_a->setMetadata("isRedZone",N);
            //insert func for redzone
            auto calleer_a = BB.getModule()->getOrInsertFunction("mark_invalid", typer);
            CallInst::Create(calleer_a, {arr_alloc_a,sizer}, "",arr_alloc_a);

            //insert func for original
            AllocaInst *AI = dyn_cast<AllocaInst>(&Inst);
            FunctionType *type = FunctionType::get(Type::getVoidTy(context), {Type::getInt64PtrTy(context),Type::getInt64Ty(context)}, false);
            auto callee = BB.getModule()->getOrInsertFunction("mark_valid", type);
            long sz=AI->getAllocationSizeInBits(lt).getValue();
            ConstantInt *size = builder.getInt64(sz/8);

            CallInst::Create(callee, {AI,size}, "",&Inst);


          }
          else{
            errs()<<"meet one redzone\n";
          }
        }
    }
  }
      return false;
    }


    bool isStaticAlloc(Instruction *I){
      if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
        if(AI->isStaticAlloca()&&isInterestingAlloca(*AI)){
            return 1;
        }
      }
      return 0;
    }

    Value * isInterestingMemoryAccess(Instruction *I,
                                                      bool *IsWrite,
                                                      uint64_t *TypeSize,
                                                      unsigned *Alignment
                                                      ) {
      Value *PtrOperand = nullptr;
      const DataLayout &DL = I->getModule()->getDataLayout();
      if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
        *IsWrite = false;
        *TypeSize = DL.getTypeStoreSizeInBits(LI->getType());
        *Alignment = LI->getAlignment();
        PtrOperand = LI->getPointerOperand();
      } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(SI->getValueOperand()->getType());
        *Alignment = SI->getAlignment();
        PtrOperand = SI->getPointerOperand();
      } else if (AtomicRMWInst *RMW = dyn_cast<AtomicRMWInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(RMW->getValOperand()->getType());
        *Alignment = 0;
        PtrOperand = RMW->getPointerOperand();
      } else if (AtomicCmpXchgInst *XCHG = dyn_cast<AtomicCmpXchgInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(XCHG->getCompareOperand()->getType());
        *Alignment = 0;
        PtrOperand = XCHG->getPointerOperand();
      } else if (auto CI = dyn_cast<CallInst>(I)) {
        auto *F = dyn_cast<Function>(CI->getCalledValue());
        if (F && (F->getName().startswith("llvm.masked.load.") ||
                  F->getName().startswith("llvm.masked.store."))) {
          unsigned OpOffset = 0;
          if (F->getName().startswith("llvm.masked.store.")) {
            // Masked store has an initial operand for the value.
            OpOffset = 1;
            *IsWrite = true;
          } else {
            *IsWrite = false;
          }

          auto BasePtr = CI->getOperand(0 + OpOffset);
          auto Ty = cast<PointerType>(BasePtr->getType())->getElementType();
          *TypeSize = DL.getTypeStoreSizeInBits(Ty);
          if (auto AlignmentConstant =
                  dyn_cast<ConstantInt>(CI->getOperand(1 + OpOffset)))
            *Alignment = (unsigned)AlignmentConstant->getZExtValue();
          else
            *Alignment = 1; // No alignment guarantees. We probably got Undef
          PtrOperand = BasePtr;
        }
      }

      if (PtrOperand) {
        // Do not instrument acesses from different address spaces; we cannot deal
        // with them.
        Type *PtrTy = cast<PointerType>(PtrOperand->getType()->getScalarType());
        if (PtrTy->getPointerAddressSpace() != 0)
          return nullptr;

        // Ignore swifterror addresses.
        // swifterror memory addresses are mem2reg promoted by instruction
        // selection. As such they cannot have regular uses like an instrumentation
        // function and it makes no sense to track them as memory.
        if (PtrOperand->isSwiftError())
          return nullptr;
      }
      if (auto AI = dyn_cast_or_null<AllocaInst>(PtrOperand))
	      return isInterestingAlloca(*AI) ? AI : nullptr;

      return PtrOperand;
    }

  uint64_t getAllocaSizeInBytes(const AllocaInst &AI) const {
    uint64_t ArraySize = 1;
    if (AI.isArrayAllocation()) {
      const ConstantInt *CI = dyn_cast<ConstantInt>(AI.getArraySize());
      assert(CI && "non-constant array size");
      ArraySize = CI->getZExtValue();
    }
    Type *Ty = AI.getAllocatedType();
    uint64_t SizeInBytes =
        AI.getModule()->getDataLayout().getTypeAllocSize(Ty);
    return SizeInBytes * ArraySize;
  }

    bool isInterestingAlloca(const AllocaInst &AI) {
	  bool IsInteresting =
	      (AI.getAllocatedType()->isSized() &&
	       // alloca() may be called with 0 size, ignore it.
	       ((!AI.isStaticAlloca()) || getAllocaSizeInBytes(AI) > 0) &&
	       // We are only interested in allocas not promotable to registers.
	       // Promotable allocas are common under -O0.
	       (!isAllocaPromotable(&AI)) &&
	       // inalloca allocas are not treated as static, and we don't want
	       // dynamic alloca instrumentation for them as well.
	       !AI.isUsedWithInAlloca() &&
	       // swifterror allocas are register promoted by ISel
	       !AI.isSwiftError());
	  return IsInteresting;
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
