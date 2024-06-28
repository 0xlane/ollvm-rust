#ifndef OBFUSCATION_OBFUSCATIONPASSMANAGER_H
#define OBFUSCATION_OBFUSCATIONPASSMANAGER_H

#include "include/Flattening.h"
#include "include/IndirectBranch.h"
#include "include/IndirectCall.h"
#include "include/IndirectGlobalVariable.h"
#include "include/StringEncryption.h"
#include "llvm/Passes/PassBuilder.h"

// Namespace
namespace llvm {
class ModulePass;
class PassRegistry;

ModulePass *createObfuscationPassManager();
void initializeObfuscationPassManagerPass(PassRegistry &Registry);

class ObfuscationPassManagerPass
    : public PassInfoMixin<ObfuscationPassManagerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    ModulePass *OPM = createObfuscationPassManager();
    bool Changed = OPM->runOnModule(M);
    OPM->doFinalization(M);
    delete OPM;
    if (Changed) {
      return PreservedAnalyses::none();
    }
    return PreservedAnalyses::all();
  }
};

} // namespace llvm

#endif
