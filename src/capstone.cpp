#include "state.h"

#include <optional>
#include <string>

/**
 * Translation rules for retdec's capstone2llvmir tool.
 * 
 * This uses a flattened list of all registers inside
 * the state.
 * Notably, register aliases are repeated separately instead
 * of aliasing properly.
 * 
 * Memory load/store is done directly by casting the address
 * to a pointer and dereferencing that pointer.
 */

std::optional<StateReg> discriminateGlobal(AllocaInst& inst) {
    std::string nm = inst.getName().str();
    if (nm.size() == 0) {
        return std::nullopt;
    }
    char first = nm.at(0);

    if (first == 'x' && nm.size() <= 3) {
        return StateReg{X, std::stoi(nm.substr(1))};
    } else if (first == 'v' && nm.size() <= 3) {
        return StateReg{V, std::stoi(nm.substr(1))};
    } else if (nm == "pc") {
        return StateReg{PC};
    } else if (nm.starts_with("cpsr_")) {
        return StateReg{STATUS, toupper(nm.at(5))};
    } else {
        return std::nullopt;
    }
}

void capstone(Module& m) {
    assert(m.getFunctionList().size() > 0);

    Function& f = *m.begin();
    // BasicBlock& entry = newEntryBlock(f);
    assert(!f.empty() && "function empty");
    Instruction* front = f.getEntryBlock().getFirstNonPHI();

    ReturnInst* back = &uniqueReturn(f);

    auto capstone = internaliseGlobals(m, f);
    auto unified = generateGlobalState(m);

    // here, cap refers to the capstone-specific variable
    // glo is the allocated global
    // reg is the abstract description

    for (auto* cap : capstone) {
        auto stateOpt = discriminateGlobal(*cap);
        if (stateOpt.has_value()) {
            StateReg reg = *stateOpt;

            auto nm = reg.name();
            Type* ty = reg.ty();
            GlobalVariable* glo = m.getNamedGlobal(nm);
            assert(glo != nullptr && "unified global variable not found");
            
            assert(glo->getValueType() == ty);
            assert(cap->getAllocatedType() == ty);


            auto loadPre = new LoadInst(ty, glo, nm + "_pre", front);
            new StoreInst(loadPre, cap, front);

            auto loadPost = new LoadInst(ty, cap, nm + "_post", back);
            new StoreInst(loadPost, glo, back);
        } else {
            errs() << *cap << "\n";
            assert(0 && "use of unhandled capstone variable");
        }
    }

}
