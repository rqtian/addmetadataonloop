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

#include <iostream>   
#include <string> 

// #define DEBUG_MODE

#ifdef DEBUG_MODE
#define myerrs() errs()
#else
#define myerrs() nulls()
#endif

using namespace llvm;

void add_metadata(Module & Mod, Instruction & Inst);

namespace {

  struct AddMetadataOnLoop : public ModulePass{
    static char ID;
    AddMetadataOnLoop() : ModulePass(ID) {}    

    virtual void getAnalysisUsage(AnalysisUsage& AU) const override;

    virtual bool runOnModule(Module &Mod) override;

  };//end of struct AddMetadataOnLoop
}//end of anonymous namespace

void AddMetadataOnLoop::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
}

bool AddMetadataOnLoop::runOnModule(Module &Mod){

  errs() << "AddMetadataOnLoop: ";
  errs().write_escaped(Mod.getName()) << '\n';

  for(Function &Func : Mod){
    if(! Func.empty ()){
      //Call LoopInfoWrapperPass to get the Loop in current Function
      //Call DominatorTreeWrapperPass to get the back edge infomation
      
      LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>(Func).getLoopInfo();       
      DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(Func).getDomTree();

      for(Loop* loop : LI->getLoopsInPreorder()){
        myerrs() << "*** Add Metadata On Loop ***\n";

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

char AddMetadataOnLoop::ID = 0;

/*Register to "opt" */
static RegisterPass<AddMetadataOnLoop> X("addmetadataonloop", "Add Metadata On Loop pass",                       false /* Only looks at CFG */,
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
