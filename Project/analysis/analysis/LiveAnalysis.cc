#include <map>
#include <fstream> // For ofstream
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Analysis/CFG.h"

#include "llvm/IR/IntrinsicInst.h" //debug stuff
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;

namespace {

bool isBackEdge(SmallVector<std::pair<const BasicBlock*, const BasicBlock*>, 16> backedges, BasicBlock* src, BasicBlock* dst) {
    for (const auto &edge : backedges) {
        if (edge.first == src && edge.second == dst) {
            return true;
        }
    }
    return false;
}

// void outputCFGwithLiveVariables1(Function &F, std::map<Instruction*, std::set<StringRef>> &InLive, std::map<Instruction*, std::set<StringRef>> &OutLive) {
//     // Create and open a text file
//     std::string filename = "cfg_" + F.getName().str() + ".dot";
//     std::ofstream ofile;
//     ofile.open(filename);
//     ofile << "digraph \"CFG for '" << F.getName() << "' function\" {\n";
//     ofile << "label=\"CFG for '" << F.getName() << "' function\";\n";
//     ofile << "labelloc=top;\n";
//     ofile << "node [shape=rectangle];\n";

//     // Iterate over all basic blocks in the function
//     for (BasicBlock &bb : F) {
//         // Create a label for the basic block
//         std::string bbLabel = "";
//         for (Instruction &inst : bb) {
//             std::string instStr;
//             raw_string_ostream rso(instStr);
//             inst.print(rso);
//             rso.flush();
//             bbLabel += instStr + "\\l"; // \l is for left-justified line break in dot
//         }

//         // Add InLive and OutLive information to the label
//         bbLabel += "InLive: {";
//         for (const auto &var : InLive[&bb.front()]) {
//             bbLabel += var.str() + ", ";
//         }
//         if (!InLive[&bb.front()].empty()) {
//             bbLabel.pop_back(); // Remove last space
//             bbLabel.pop_back(); // Remove last comma
//         }
//         bbLabel += "}\\l";

//         bbLabel += "OutLive: {";
//         for (const auto &var : OutLive[&bb.back()]) {
//             bbLabel += var.str() + ", ";
//         }
//         if (!OutLive[&bb.back()].empty()) {
//             bbLabel.pop_back(); // Remove last space
//             bbLabel.pop_back(); // Remove last comma
//         }
//         bbLabel += "}\\l";

//         // Write the basic block node to the file
//         ofile << "\"" << &bb << "\" [label=\"" << bbLabel << "\"];\n";

//         // Write edges to successor basic blocks
//         for (BasicBlock *succ : successors(&bb)) {
//             ofile << "\"" << &bb << "\" -> \"" << succ << "\";\n";
//         }
//     }
// }

void outputCFGwithLiveVariables(Function &F, std::map<Instruction*, std::set<StringRef>> &InLive, std::map<Instruction*, std::set<StringRef>> &OutLive, int index) {
    // Generate a CFG in .dot format for visualization
    SmallVector<std::pair<const BasicBlock*, const BasicBlock*>, 16> backedges;
    std::string filename = "./cfg_images/cfg_" + F.getName().str() + std::to_string(index) + ".dot";
    std::ofstream ofile;
    ofile.open(filename);
    llvm::raw_os_ostream llvm_stream(ofile);

    ofile << "digraph \"CFG for '" << F.getName().str();
    ofile << "' function\" {\n";
    for (BasicBlock &bb : F) {
        std::string bbName;
        if (bb.hasName()) bbName = bb.getName().str();
        else bbName = std::to_string((unsigned long)&bb);
        ofile << "  \"" << &bb << "\" [shape=record,color=\"#b70d28ff\", style=filled, fillcolor=\"#b70d2870\", fontname=\"Courier\",label=\"{" << bbName << " \\l|";
        
        ofile << "Live: ";
        for(auto &entry : InLive[&bb.front()]) {
            ofile << entry.str() << ", ";
        }
        ofile << "\\l";
        ofile << "|";
        
        for (const auto &Inst : bb) {
            ofile << "    ";
            Inst.print(llvm_stream); 
            ofile << "\\l";
        }
        ofile << "}\"];\n";
        for (BasicBlock *Succ : successors(&bb)) {
            ofile << "  \"" << &bb << "\" -> \"" << Succ << "\";\n";
        }
    }
    // Compute the backedges using LLVM's built-in function
    FindFunctionBackedges(F, backedges);
    for (const auto &edge : backedges) {
        ofile << "  \"" << edge.first << "\" -> \"" << edge.second << "\" [style=dashed,color=red];\n";
    }

    ofile << "}\n";
    ofile.close();
}
///
// Intra-procedural Data flow Analysis
//
void analyseFunction(Function &F) {
    //
    // Print the results
    //
    errs() << "Function " << F.getName() << "\n";

    // The below code snippet iterates over the Debug Info records to find out the names
    // of local variables, and adds then to programVariables map
    std::map<StringRef, Value*> programVariables;
    for (BasicBlock &bb : F)
        for (Instruction &inst: bb) {
            // if(auto * dbginst = dyn_cast<DbgVariableIntrinsic>(&inst)) {
            //     auto* var = dbginst -> getVariable();
            //     StringRef name = var->getName();
            //     Value* addr = dbginst->getOperand(0);
            //     // ddr = addr -> stripPointerCasts();
            //     programVariables[name] = dyn_cast<AllocaInst>(addr);
            // }
            for (DbgVariableRecord &DVR : filterDbgVars(inst.getDbgRecordRange()))
                programVariables[DVR.getVariable()->getName()] = DVR.getAddress();
        }

    for(auto x : programVariables) {
        errs() << "Variable: " << x.first << " Address: " << *(x.second) << "\n";
    }
    errs() << "\n";

    // THE ALGORITHM STARTS WITH EMPTY_Live RN. THAT IS SLIGHTLY WRONG. SHOULD START WITH FULL Live AND MERGE FROM PREDS, NOT USING PREVIOUS Live IN THAT PLACE!!!
    std::map<Instruction*, std::set<StringRef>> InLive_old;
    std::map<Instruction*, std::set<StringRef>> OutLive_old;
    std::map<Instruction*, std::set<StringRef>> InLive_new;
    std::map<Instruction*, std::set<StringRef>> OutLive_new;
    for(BasicBlock &bb: F) {
        for (Instruction &inst: bb) {
            InLive_new[&inst] = std::set<StringRef>();
            OutLive_new[&inst] = std::set<StringRef>();
            InLive_old[&inst] = std::set<StringRef>();
            OutLive_old[&inst] = std::set<StringRef>();
        }
    }

    // Generate live variable information
    int iterationCount = 0;
    bool changed = true;
    outputCFGwithLiveVariables(F, InLive_new, OutLive_new, iterationCount);
    while (changed) {
        iterationCount++;
        changed = false;

        for(BasicBlock &bb: F){
            for(Instruction &inst: bb) {
                Instruction* nextInstruction = inst.getNextNode();
                if (nextInstruction) {
                    for(auto var : programVariables) {
                        for(auto var : InLive_new[nextInstruction]) {
                            OutLive_new[&inst].insert(var);
                        }
                    }
                }


                if(auto* alloc = dyn_cast<AllocaInst>(&inst)) {
                    InLive_new[&inst] = OutLive_new[&inst];
                } 
                else if(auto *store = dyn_cast<StoreInst>(&inst)) {
                    Value *ptrOperand = store->getPointerOperand();
                    StringRef variableName;
                    for (auto &entry : programVariables) {
                        if (entry.second == ptrOperand) {
                            variableName = entry.first; // match found
                        }
                    }
                    InLive_new[&inst] = OutLive_new[&inst];
                    InLive_new[&inst].erase(variableName); // KILL
                } else if(auto *load = dyn_cast<LoadInst>(&inst)) {
                    Value *ptrOperand = load->getPointerOperand();
                    StringRef variableName;
                    for (auto &entry : programVariables) {
                        if (entry.second == ptrOperand) {
                            variableName = entry.first; // match found
                        }
                    }
                    InLive_new[&inst] = OutLive_new[&inst];
                    InLive_new[&inst].insert(variableName);
                    errs() << "Load's variable name " << variableName << "\n";
                    errs() << "Load Instruction: " << inst << "\n";
                } else if(auto *call = dyn_cast<CallInst>(&inst)){
                    InLive_new[&inst] = OutLive_new[&inst];

                    for (unsigned i = 0; i < call->arg_size(); i++) { 
                    // CHECK IF arg_size() works. Online doc showed getNumArgOperands()
                        Value *arg = call->getArgOperand(i);

                        for (auto &entry : programVariables) {
                            StringRef varName = entry.first;
                            Value *varallocer  = entry.second;

                            if (arg == varallocer) {
                                InLive_new[&inst].insert(varName);
                            }
                        }
                    }
                } else if(auto *ret = dyn_cast<ReturnInst>(&inst)) {
                    InLive_new[&inst] = OutLive_new[&inst];
                } else if(auto *binop = dyn_cast<BinaryOperator>(&inst)) {
                    InLive_new[&inst] = OutLive_new[&inst];

                    // Not sure -- to remove
                    for (auto &entry : programVariables) {
                        StringRef varName = entry.first;
                        Value *varallocer  = entry.second;

                        if (binop == varallocer) {
                            InLive_new[&inst].insert(varName);
                        }
                    }


                } else if(auto* br = dyn_cast<BranchInst>(&inst)) {
                    for(BasicBlock *succ : successors(br->getParent())) {
                        Instruction* firstInst = &succ->front();
                        for(auto var : InLive_new[firstInst]) {
                            OutLive_new[&inst].insert(var);
                        }
                    }
                    InLive_new[&inst] = OutLive_new[&inst];
                } 
                else 
                {
                    InLive_new[&inst] = OutLive_new[&inst];
                }
            }
        }

        bool change = false;
        for (BasicBlock &bb : F){
            Instruction& finst = *(bb.getFirstInsertionPt());
            if(InLive_new[&finst] != InLive_old[&finst])
                change = true; 
        }
        if(change)
            outputCFGwithLiveVariables(F, InLive_new, OutLive_new, iterationCount);

        if(InLive_new == InLive_old && OutLive_new == OutLive_old)break;
        else changed = true;
        InLive_old = InLive_new;
        OutLive_old = OutLive_new;

        errs() << "-----------------\n";
    }
    
    errs() << "Generated CFG in files \n";
}

//
// Registering the Function Pass (Don't change any code below)
//
class LiveAnalysisPass : public PassInfoMixin<LiveAnalysisPass> {
public:
    static bool isRequired() { return true; }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        analyseFunction(F);
        return PreservedAnalyses::all();
    };
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "Live Analysis Pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "live-analysis") {
                        FPM.addPass(LiveAnalysisPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}
