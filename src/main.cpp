#include <cstdlib>
#include <ranges>
#include <iostream>


#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

#include "llvm/IR/Instructions.h"

#include "context.h"
#include "state.h"

using namespace llvm;


BasicBlock& newEntryBlock(Function& f) {
    BasicBlock* oldEntry = f.empty() ? nullptr : &f.getEntryBlock();
    BasicBlock* newEntry = BasicBlock::Create(Context, "new_entry", &f, oldEntry);
    if (oldEntry) {
        BranchInst::Create(oldEntry, newEntry);
    }
    return f.getEntryBlock();
}

void internaliseGlobals(Module& module, Function& f) {

    Instruction& insertion = *f.getEntryBlock().getFirstInsertionPt();

    for (auto& g : (module.getGlobalList())) {

        Instruction* alloc = new AllocaInst(
            g.getInitializer()->getType(), /*addrspace*/0, /*arraysize*/nullptr,
            g.getName(), &insertion);

        g.replaceAllUsesWith(alloc);
    }

    // module.getGlobalList().clear();
}

void internaliseParams(Function& f) {
    Instruction& insertion = *f.getEntryBlock().getFirstInsertionPt();

    for (auto& a : f.args()) {

        std::string nm {"arg"};
        nm.append(a.getName());

        Instruction* alloc = new AllocaInst(
            a.getType(), /*addrspace*/0, /*arraysize*/nullptr,
            nm, &insertion);

        a.replaceAllUsesWith(alloc);
    }
}


int main(int argc, char** argv)
{
    std::vector<std::string> args{};
    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }


    if (argc < 2) {
        errs() << "Expected an argument - IR file name\n";
        exit(1);
    }

    Context.enableOpaquePointers(); // llvm 14 specific 

    SMDiagnostic Err{};
    std::unique_ptr<Module> ModPtr = parseIRFile(argv[1], Err, Context);
    if (!ModPtr) {
        Err.print(argv[0], errs());
        return 1;
    }
    Module& Mod = *ModPtr;

    // outs() << "==============\n";


    // outs() << "==============\n";


    auto& funcs = Mod.getFunctionList();
    assert(funcs.size() >= 1);
    Function& f = funcs.front();
    newEntryBlock(f);
    internaliseGlobals(Mod, f);
    internaliseParams(f);

    generateGlobalState(Mod);

    // after internalising global variables, delete old global declarations.
    for (auto& g : Mod.getGlobalList()) {
        assert(g.user_empty());
    }
    // Mod.getGlobalList().clear();

    verifyModule(Mod);

    Mod.print(outs(), nullptr);
    return 0;
}
