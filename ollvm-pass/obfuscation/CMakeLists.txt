add_library(LLVMObfuscationx MODULE 
    CryptoUtils.cpp
    Utils.cpp
    ObfuscationPassManager.cpp
    ObfuscationOptions.cpp
    IndirectBranch.cpp
    IndirectCall.cpp
    IndirectGlobalVariable.cpp
    Flattening.cpp
    StringEncryption.cpp
    LegacyLowerSwitch.cpp
    obfuscation.def
    )

add_dependencies(LLVMObfuscationx intrinsics_gen LLVMLinker)

llvm_map_components_to_libnames(llvm_libs support core irreader linker)
target_link_libraries(LLVMObfuscationx PRIVATE ${llvm_libs})
