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


    std::string lifter {argc >= 2 ? argv[1] : ""};
    const char* fname = argc >= 3 ? argv[2] : "/dev/stdin";

    Context.enableOpaquePointers(); // llvm 14 specific

    std::function<void(Module&)> translator{};

    if (lifter == "cap") {
        translator = capstone;
    } else if (lifter == "rem") {
        translator = remill;
    } else {
        errs() << "unsupported lifter, expected cap or rem.\n";
        return 1;
    }


    SMDiagnostic Err{};
    errs() << "loading IR file " << fname << '\n';
    std::unique_ptr<Module> ModPtr = parseIRFile(fname, Err, Context);
    if (!ModPtr) {
        Err.print(argv[0], errs());
        return 1;
    }
    Module& Mod = *ModPtr;

    auto& funcs = Mod.getFunctionList();
    assert(funcs.size() >= 1);

    assert(translator);
    translator(Mod);


    // generateGlobalState(Mod);

    // after internalising global variables, delete old global declarations.
    // for (auto& g : Mod.getGlobalList()) {
    //     assert(g.user_empty());
    // }
    // Mod.getGlobalList().clear();


    outs() << Mod;

    bool err = verifyModule(Mod, &errs());
    if (err) {
        errs() << "\n### MODULE VERIFY FAILED ###\n";
        return -1;
    }
    return 0;
}
