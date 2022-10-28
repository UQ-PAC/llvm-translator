#include "context.h"
#include "state.h"

#include <llvm/IR/Instructions.h>
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

std::optional<StateReg> discriminateGlobal(std::string nm) {
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

void capstoneMakeBranchCond(Module& m, GlobalVariable& pc) {
    Function& f = findFunction(m, "capstone_branch_cond");
    assert(f.arg_size() == 2);

    Type* ty = pc.getValueType();
    auto* four = ConstantInt::get(ty, 4);

    auto* bb = BasicBlock::Create(Context, "", &f);
    auto* sel = SelectInst::Create(
        f.getArg(0),
        BinaryOperator::Create(Instruction::BinaryOps::Sub, f.getArg(1), four, "", bb),
        new LoadInst(ty, &pc, "", bb),
        "", bb);
    new StoreInst(sel, &pc, bb);
    ReturnInst::Create(Context, bb);
}

void capstoneMakeBranch(Module& m, GlobalVariable& pc) {
    Function& f = findFunction(m, "capstone_branch");
    assert(f.arg_size() == 1);

    Type* ty = pc.getValueType();

    auto* bb = BasicBlock::Create(Context, "", &f);
    auto* tru = ConstantInt::getTrue(Context);

    Function& cond = findFunction(m, "capstone_branch_cond");
    ArrayRef<Value*> args{tru, f.getArg(0)};
    auto* call = CallInst::Create(cond.getFunctionType(), &cond, args, "", bb);
    ReturnInst::Create(Context, bb);
}

void capstoneMakeReturn(Module& m, GlobalVariable& pc) {
    Function& f = findFunction(m, "capstone_return");
    auto* bb = BasicBlock::Create(Context, "", &f);

    // a return is just a branch.
    Function& cond = findFunction(m, "capstone_branch");
    auto* call = CallInst::Create(cond.getFunctionType(), &cond, {f.getArg(0)}, "", bb);
    ReturnInst::Create(Context, bb);

}

void capstone(Module& m) {
    assert(m.getFunctionList().size() > 0);

    if (auto* capVar = m.getNamedGlobal("capstone_asm2llvm")) {
        std::vector<User*> users (capVar->user_begin(), capVar->user_end());
        for (auto* user : users) {
            if (auto* inst = dyn_cast<Instruction>(user)) {
                errs() << "deleting: " << *inst << '\n';
                assert(inst->isSafeToRemove());
                inst->eraseFromParent();
            }
        }
        assert(capVar->getNumUses() == 0);
        capVar->eraseFromParent();
    }

    Function& f = *m.begin();
    f.setName("main");
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
        if (cap->user_empty()) {
            // errs() << "erased unused capstone: " << *cap << '\n';
            cap->eraseFromParent();
            continue;
        }
        std::string nm = cap->getName().str();
        auto stateOpt = discriminateGlobal(nm);
        if (stateOpt.has_value()) {
            // capstone variable is exactly a unified register.
            // replace all uses directly.
            StateReg reg = *stateOpt;
            auto nm = reg.name();
            Type* ty = reg.ty();

            GlobalVariable* glo = m.getNamedGlobal(nm);
            assert(glo != nullptr && "unified global variable not found");

            assert(glo->getValueType() == ty);
            assert(cap->getAllocatedType() == ty);

            cap->replaceAllUsesWith(glo);
        } else {
            errs() << *cap << "\n";
            assert(0 && "unhandled capstone variable");
        }
    }

    GlobalVariable* pc = m.getNamedGlobal("PC");
    assert(pc);
    Type* ty = pc->getValueType();
    auto* load = new LoadInst(ty, pc, "", back);
    auto* four = ConstantInt::get(ty, 4);
    auto* inc = BinaryOperator::Create(
        Instruction::BinaryOps::Add, load, four, "", back);
    new StoreInst(inc, pc, back);

    capstoneMakeBranchCond(m, *pc);
    capstoneMakeBranch(m, *pc);
    capstoneMakeReturn(m, *pc);
}
