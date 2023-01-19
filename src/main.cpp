#include <cstdlib>
#include <ranges>
#include <map>
#include <iostream>
#include <fstream>


#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
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

int force_vars(std::vector<std::string>& argv) {
    std::map<std::string, Type*> globals;
    std::map<std::string, std::unique_ptr<Module>> Modules;

    auto fnames = std::ranges::subrange(argv.begin() + 2, argv.end());

    for (auto& fname : fnames) {
        SMDiagnostic Err{};
        auto Module = parseIRFile(fname, Err, Context);
        assert(Module && "failed to parse module");
        Modules[fname] = std::move(Module);
    }

    for (auto& [_, Module] : Modules) {
        for (auto& var : Module->getGlobalList()) {
            if (var.hasNUsesOrMore(1)) {
                std::string name{var.getName()};
                globals[name] = var.getValueType();
            }
        }
    }

    for (auto& [fname, Module] : Modules) {
        auto* root = findFunction(*Module, "root");
        assert(root && "root function not found");
        auto* entry = &root->getEntryBlock();
        auto* entry2 = BasicBlock::Create(Context, "forced_vars", root, entry);

        IRBuilder irb{entry2, entry2->begin()};
        for (auto& [nm, ty] : globals) {
            auto* glo = Module->getNamedGlobal(nm);
            if (glo->getNumUses() == 0) {
                auto* load = irb.CreateLoad(ty, glo, "_" + nm);
                noundef(load);
            }
        }
        if (entry2->empty()) {
            entry2->eraseFromParent();
        } else {
            irb.CreateBr(entry);
        }

        std::vector<GlobalVariable*> globals; 
        for (auto& glo : Module->getGlobalList()) {
            globals.push_back(&glo);
        }
        correctGlobalAccesses(globals);

        bool err = verifyModule(*Module, &errs());
        assert(!err && "verify module failed");

        std::error_code Err; 
        llvm::raw_fd_ostream file{fname, Err};
        file << *Module;
    }


    return 0;
}

int main(int argc, char** argv)
{
    std::vector<std::string> args{argv, argv + argc};

    std::string lifter {argc >= 2 ? argv[1] : ""};
    const char* fname = argc >= 3 ? argv[2] : "/dev/stdin";

    Context.enableOpaquePointers(); // llvm 14 specific

    std::function<void(Module&)> translator{};

    if (lifter == "cap") {
        translator = capstone;
    } else if (lifter == "rem") {
        translator = remill;
    } else if (lifter == "asl") {
        translator = asl;
    } else if (lifter == "vars") {
        return force_vars(args);
    } else {
        errs() << "unsupported lifter, expected cap or rem or asl.\n";
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
    Mod.setSourceFileName(fname);

    auto& funcs = Mod.getFunctionList();
    assert(funcs.size() >= 1);

    assert(translator);
    translator(Mod);

    outs() << Mod;

    bool err = verifyModule(Mod, &errs());
    if (err) {
        errs() << "\n### MODULE VERIFY FAILED ###\n";
        return -1;
    }
    return 0;
}
