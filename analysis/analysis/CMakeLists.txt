add_llvm_pass_plugin(livePass
    LiveAnalysis.cc
)

target_link_libraries(livePass PRIVATE
    LLVMSupport
    LLVMCore
    LLVMIRReader
    LLVMTransformUtils
    LLVMAnalysis
)