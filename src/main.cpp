#include <cstdlib>
#include <ranges>
#include <iostream>


#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

#include "llvm/IR/Instructions.h"

using namespace llvm;

LLVMContext Context{};

BasicBlock& newEntryBlock(Function& f) {
    BasicBlock* oldEntry = f.empty() ? nullptr : &f.getEntryBlock();
    BasicBlock* newEntry = BasicBlock::Create(Context, "new_entry", &f, oldEntry);
    if (oldEntry) {
        BranchInst::Create(oldEntry, newEntry);
    }
    return f.getEntryBlock();
}

void internalise(Module& module, Function& f) {

    BasicBlock& entry = newEntryBlock(f);
    Instruction& insertion = *entry.getFirstInsertionPt();

    for (auto& g : (module.getGlobalList())) {

        std::string nm{"state__"};
        nm.append(g.getName());

        Instruction* alloc = new AllocaInst(
            g.getInitializer()->getType(), /*addrspace*/0, /*arraysize*/nullptr,
            nm, &insertion);

        g.replaceAllUsesWith(alloc);
    }

    // module.getGlobalList().clear();
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

    SMDiagnostic Err{};
    std::unique_ptr<Module> Mod = parseIRFile(argv[1], Err, Context);
    if (!Mod) {
        Err.print(argv[0], errs());
        return 1;
    }
    //outs() << *Mod;

    // outs() << "==============\n";


    // outs() << "==============\n";


    auto& funcs = Mod->getFunctionList();
    assert(funcs.size() >= 1);
    Function& f = funcs.front();
    internalise(*Mod, f);

    // after internalising global variables, delete old global declarations.
    for (auto& g : Mod->getGlobalList()) {
        assert(g.user_empty());
    }
    Mod->getGlobalList().clear();

    Mod->print(outs(), nullptr);
    return 0;
}
