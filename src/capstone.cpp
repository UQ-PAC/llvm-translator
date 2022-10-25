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
    BasicBlock& entry = newEntryBlock(f);
    Instruction* insert = entry.getFirstNonPHI();

    auto capstone = internaliseGlobals(m, f);
    auto unified = generateGlobalState(m);

    for (auto cap : capstone) {
        auto stateOpt = discriminateGlobal(*cap);
        if (stateOpt.has_value()) {
            StateReg value = *stateOpt;

            auto nm = value.name();
            GlobalVariable* glo = m.getNamedGlobal(nm);
            assert(glo != nullptr && "unified global variable not found");
            
            nm.append("__load");
            auto load = new LoadInst(cap->getType(), glo, nm, insert);
            new StoreInst(load, cap, insert);
        }
    }

}
