#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/IVDescriptors.h"

#include "llvm/IR/Value.h"

#include <iostream>   
#include <string> 

// #define DEBUG_MODE

#ifdef DEBUG_MODE
#define myerrs() errs()
#else
#define myerrs() nulls()
#endif

using namespace llvm;

/*For reigister pass, then invoke through command option */
// namespace llvm {
//   ModulePass *createAddMatedataOnLoop();
// }
void add_metadata(Module & Mod, Instruction & Inst);

namespace {

  struct AddMatedataOnLoop : public ModulePass{
    static char ID;
    AddMatedataOnLoop() : ModulePass(ID) {}    

    virtual void getAnalysisUsage(AnalysisUsage& AU) const override;

    virtual bool runOnModule(Module &Mod) override;

  };//end of struct AddMatedataOnLoop
}//end of anonymous namespace

void AddMatedataOnLoop::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
}

bool AddMatedataOnLoop::runOnModule(Module &Mod){

  errs() << "AddMatedataOnLoop: ";
  errs().write_escaped(Mod.getName()) << '\n';

  for(Function &Func : Mod){
    if(! Func.empty ()){
      //Call LoopInfoWrapperPass to get the Loop in current Function
      //Call DominatorTreeWrapperPass to get the back edge infomation
      //Call ScalarEvolutionWrapperPass, try to get the loop induction variable update instruction, i.e. step Instruction. note:Fail to get the information

      // getAnalysis<DominatorTreeWrapperPass>(Func); 
      LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>(Func).getLoopInfo();       
      DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(Func).getDomTree();
      ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>(Func).getSE();

      for(Loop* loop : LI->getLoopsInPreorder()){
        myerrs() << "*** Add Matedata On Loop ***\n";

        // FIX ME:
        // Try to get the step instruction for the current loop,
        // But failed when the induction variable is not PHINode
        InductionDescriptor IndDesc;
        if(loop->getInductionDescriptor(*SE, IndDesc)){;
          Instruction *StepInst = IndDesc.getInductionBinOp();
          myerrs() << "**** step Inst: " << *StepInst << "\n";

          add_metadata(Mod, *(StepInst));
        }
        else{
          myerrs() << "**** not found step Inst\n";
        }

        for(BasicBlock * blk : loop->getBlocks () ){
          for(Instruction & inst : *blk){
            // Check whether a br instruction represents back edge for current loop
            // Add metadata if it is.
            if (llvm::BranchInst* br = dyn_cast<llvm::BranchInst>(&inst)){
              BasicBlock * current_block = br->getParent();
              for(unsigned i = 0; i < br->getNumSuccessors(); i++){

                BasicBlock * 	successor_block = br->getSuccessor(i);
                bool is_backedge = DT->dominates(successor_block, current_block);

                if(is_backedge){
                  add_metadata(Mod, inst);
                }
              }
            }
          }
        }

      }
    }        

  }
  
  return false;     
} 

char AddMatedataOnLoop::ID = 0;

/*Register to "opt" */
static RegisterPass<AddMatedataOnLoop> X("addmatedataonloop", "Add Matedata On Loop pass",                       false /* Only looks at CFG */,
                             false /* Analysis Pass */);  


/*
** Add metadata to Instruction inst, which is in Module Mod
*/
void add_metadata(Module & Mod, Instruction & inst){
  const std::string & sourcefilename = Mod.getSourceFileName();

  std::string filename_line_content="";

  const DebugLoc &DL = inst.getDebugLoc();
  if(DL.get()){
    unsigned line = DL.get()->getLine();
    filename_line_content = "sourcefilename: " + sourcefilename + ", line: " + std::to_string(line)+"\00"; 
  }else{
    filename_line_content = "sourcefilename: " + sourcefilename + ", line: UNKOWN \00"; 
  } 
  myerrs() << "filename_line_content: " << filename_line_content << "\n";

  LLVMContext& C = inst.getContext();
  MDNode* N = MDNode::get(C, MDString::get(C, filename_line_content));
  inst.setMetadata("filename_line", N);
}
