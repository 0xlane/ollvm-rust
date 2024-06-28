add_library(LLVMObfuscationx SHARED 
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

target_include_directories(LLVMObfuscationx PRIVATE ${CMAKE_SOURCE_DIR}/obfuscation)

add_dependencies(LLVMObfuscationx intrinsics_gen LLVMLinker)

llvm_map_components_to_libnames(llvm_libs support core irreader linker)
target_link_libraries(LLVMObfuscationx PRIVATE ${llvm_libs})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_target_properties(LLVMObfuscationx PROPERTIES
        LINK_FLAGS "-static -static-libgcc -Wl,-Bstatic,--whole-archive -lwinpthread -lstdc++ -Wl,--no-whole-archive -Wl,-Bdynamic"
    )
endif()
