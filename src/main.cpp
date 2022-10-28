#include <cstdlib>
#include <ranges>
#include <iostream>


#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"


#include "context.h"
#include "state.h"
#include "translate.h"

using namespace llvm;

const char *__asan_default_options() {
    return "disable_coredump=0";
}

int main(int argc, char** argv)
{
    std::vector<std::string> args{};
    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }


    const char* fname = argc >= 2 ? argv[1] : "/dev/stdin";

    Context.enableOpaquePointers(); // llvm 14 specific

    SMDiagnostic Err{};
    std::unique_ptr<Module> ModPtr = parseIRFile(fname, Err, Context);
    if (!ModPtr) {
        Err.print(argv[0], errs());
        return 1;
    }
    Module& Mod = *ModPtr;

    // outs() << "==============\n";


    // outs() << "==============\n";


    auto& funcs = Mod.getFunctionList();
    assert(funcs.size() >= 1);

    capstone(Mod);
    // internaliseParams(f);

    // generateGlobalState(Mod);

    // after internalising global variables, delete old global declarations.
    // for (auto& g : Mod.getGlobalList()) {
    //     assert(g.user_empty());
    // }
    // Mod.getGlobalList().clear();

    verifyModule(Mod);

    outs() << Mod;
    return 0;
}
