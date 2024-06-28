#include "include/ObfuscationPassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "include/ObfuscationOptions.h"

#define DEBUG_TYPE "ir-obfuscation"

using namespace llvm;

static cl::opt<bool>
    EnableIRObfusaction("irobf", cl::init(false), cl::NotHidden,
                        cl::desc("Enable IR Code Obfuscation."),
                        cl::ZeroOrMore);
static cl::opt<bool>
    EnableIndirectBr("irobf-indbr", cl::init(false), cl::NotHidden,
                     cl::desc("Enable IR Indirect Branch Obfuscation."),
                     cl::ZeroOrMore);

static cl::opt<bool>
    EnableIndirectCall("irobf-icall", cl::init(false), cl::NotHidden,
                       cl::desc("Enable IR Indirect Call Obfuscation."),
                       cl::ZeroOrMore);

static cl::opt<bool> EnableIndirectGV(
    "irobf-indgv", cl::init(false), cl::NotHidden,
    cl::desc("Enable IR Indirect Global Variable Obfuscation."),
    cl::ZeroOrMore);

static cl::opt<bool> EnableIRFlattening(
    "irobf-cff", cl::init(false), cl::NotHidden,
    cl::desc("Enable IR Control Flow Flattening Obfuscation."), cl::ZeroOrMore);

static cl::opt<bool>
    EnableIRStringEncryption("irobf-cse", cl::init(false), cl::NotHidden,
                             cl::desc("Enable IR Constant String Encryption."), cl::ZeroOrMore);

static cl::opt<std::string> GoronConfigure("goron-cfg",
                                           cl::desc("Goron configuration file"),
                                           cl::Optional);

namespace llvm {

struct ObfuscationPassManager : public ModulePass {
  static char ID; // Pass identification
  SmallVector<Pass *, 8> Passes;

  ObfuscationPassManager() : ModulePass(ID) {
    initializeObfuscationPassManagerPass(*PassRegistry::getPassRegistry());
  };

  StringRef getPassName() const override { return "Obfuscation Pass Manager"; }

  bool doFinalization(Module &M) override {
    bool Change = false;
    for (Pass *P : Passes) {
      Change |= P->doFinalization(M);
      delete (P);
    }
    return Change;
  }

  void add(Pass *P) { Passes.push_back(P); }

  bool run(Module &M) {
    bool Change = false;
    for (Pass *P : Passes) {
      switch (P->getPassKind()) {
      case PassKind::PT_Function:
        Change |= runFunctionPass(M, (FunctionPass *)P);
        break;
      case PassKind::PT_Module:
        Change |= runModulePass(M, (ModulePass *)P);
        break;
      default:
        continue;
      }
    }
    return Change;
  }

  bool runFunctionPass(Module &M, FunctionPass *P) {
    bool Changed = false;
    for (Function &F : M) {
      Changed |= P->runOnFunction(F);
    }
    return Changed;
  }

  bool runModulePass(Module &M, ModulePass *P) { return P->runOnModule(M); }

  static ObfuscationOptions *getOptions() {
    ObfuscationOptions *Options = nullptr;
    if (sys::fs::exists(GoronConfigure.getValue())) {
      Options = new ObfuscationOptions(GoronConfigure.getValue());
    } else {
      SmallString<128> ConfigurePath;
      if (sys::path::home_directory(ConfigurePath)) {
        sys::path::append(ConfigurePath, "goron.yaml");
        Options = new ObfuscationOptions(ConfigurePath);
      } else {
        Options = new ObfuscationOptions();
      }
    }
    return Options;
  }

  bool runOnModule(Module &M) override {
    
    if (EnableIndirectBr || EnableIndirectCall || EnableIndirectGV ||
        EnableIRFlattening || EnableIRStringEncryption) {
      EnableIRObfusaction = true;
    }

    if (!EnableIRObfusaction) {
      return false;
    }

    std::unique_ptr<ObfuscationOptions> Options(getOptions());
    unsigned pointerSize = M.getDataLayout().getTypeAllocSize(PointerType::getUnqual(M.getContext()));
    if (EnableIRStringEncryption || Options->EnableCSE) {
      add(llvm::createStringEncryptionPass(true, Options.get()));
    }

    add(llvm::createFlatteningPass(pointerSize, EnableIRFlattening || Options->EnableCFF, Options.get()));
    add(llvm::createIndirectBranchPass(pointerSize,
        EnableIndirectBr || Options->EnableIndirectBr, Options.get()));
    add(llvm::createIndirectCallPass(pointerSize, EnableIndirectCall ||
                                         Options->EnableIndirectCall, Options.get()));
    add(llvm::createIndirectGlobalVariablePass(pointerSize,
        EnableIndirectGV || Options->EnableIndirectGV, Options.get()));

    bool Changed = run(M);

    return Changed;
  }
};
} // namespace llvm

char ObfuscationPassManager::ID = 0;
ModulePass *llvm::createObfuscationPassManager() {
  return new ObfuscationPassManager();
}
INITIALIZE_PASS_BEGIN(ObfuscationPassManager, "irobf", "Enable IR Obfuscation",
                      false, false)
INITIALIZE_PASS_END(ObfuscationPassManager, "irobf", "Enable IR Obfuscation",
                    false, false)

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getObfPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Obfuscation", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement> InnerPipeline) {
                    if (Name == EnableIRObfusaction.ArgStr) {
                      EnableIRObfusaction = true;
                      for (const auto &Element : InnerPipeline) {
                        if (Element.Name == EnableIndirectBr.ArgStr) {
                          EnableIndirectBr = true;
                        } else if (Element.Name == EnableIndirectCall.ArgStr) {
                          EnableIndirectCall = true;
                        } else if (Element.Name == EnableIndirectGV.ArgStr) {
                          EnableIndirectGV = true;
                        } else if (Element.Name == EnableIRFlattening.ArgStr) {
                          EnableIRFlattening = true;
                        } else if (Element.Name == EnableIRObfusaction.ArgStr) {
                          EnableIRObfusaction = true;
                        } else if (Element.Name == EnableIRStringEncryption.ArgStr) {
                          EnableIRStringEncryption = true;
                        }
                      }

                      FPM.addPass(ObfuscationPassManagerPass());
                      return true;
                    }
                    return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getObfPassPluginInfo();
}