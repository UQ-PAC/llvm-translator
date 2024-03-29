#include "context.h"
#include "state.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/TypeSize.h"

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

    std::string vectorRegs = "vqds";
    if ((first == 'x' )&& nm.size() <= 3) {
        return StateReg{X, std::stoi(nm.substr(1))};
    } else if (nm == "pc") {
        return StateReg{PC};
    } else if (nm == "sp") {
        return StateReg{SP};
    } else if (vectorRegs.find(first) != std::string::npos && nm.size() <= 3) {
        return StateReg{V, std::stoi(nm.substr(1))};
    } else if (nm.starts_with("cpsr_")) {
        return StateReg{STATUS, toupper(nm.at(5))};
    } else {
        return std::nullopt;
    }
}

void capstoneMakeBranchCond(Module& m, GlobalVariable& pc) {
    Function* f2 = findFunction(m, "capstone_branch_cond");
    if (f2 == NULL) return;
    Function& f = *f2;
    assert(f.arg_size() == 2);

    Type* ty = pc.getValueType();
    auto* four = ConstantInt::get(ty, 4);

    using BOp = llvm::BinaryOperator;

    auto* bb = BasicBlock::Create(Context, "", &f);
    auto* load = new LoadInst(ty, &pc, "", bb);
    auto* dest = BOp::Create(BOp::Add, load, f.getArg(1), "", bb);
    auto* sel = SelectInst::Create(
        f.getArg(0),
        BOp::Create(BOp::Sub, dest, four, "", bb),
        load,
        "", bb);
    new StoreInst(sel, &pc, bb);
    ReturnInst::Create(Context, bb);
}

void capstoneMakeBranch(Module& m, GlobalVariable& pc) {
    Function* f2 = findFunction(m, "capstone_branch");
    if (f2 == NULL) return;
    Function& f = *f2;
    assert(f.arg_size() == 1);

    auto* bb = BasicBlock::Create(Context, "", &f);
    auto* tru = ConstantInt::getTrue(Context);

    Function& cond = *findFunction(m, "capstone_branch_cond");
    ArrayRef<Value*> args{tru, f.getArg(0)};
    CallInst::Create(cond.getFunctionType(), &cond, args, "", bb);
    ReturnInst::Create(Context, bb);
}

void capstoneMakeReturn(Module& m, GlobalVariable& pc) {
    Function* f2 = findFunction(m, "capstone_return");
    if (f2 == NULL) return;
    Function& f = *f2;
    auto* bb = BasicBlock::Create(Context, "", &f);

    // a return is just a branch.
    Function& cond = *findFunction(m, "capstone_branch");
    CallInst::Create(cond.getFunctionType(), &cond, {f.getArg(0)}, "", bb);
    ReturnInst::Create(Context, bb);

}

void capstone(Module& m) {
    assert(m.getFunctionList().size() > 0);

    for (const auto del : {"capstone_asm2llvm", "0"}) {
        if (auto* capVar = m.getNamedGlobal(del)) {
            for (auto* user : clone_it(capVar->users())) {
                if (auto* inst = dyn_cast<Instruction>(user)) {
                    errs() << "deleting: " << *inst << '\n';
                    assert(inst->isSafeToRemove());
                    inst->eraseFromParent();
                }
            }
            assert(capVar->getNumUses() == 0);
            capVar->eraseFromParent();
        }
    }

    Function& f = *m.begin();
    f.setName(entry_function_name);
    // BasicBlock& entry = newEntryBlock(f);
    assert(!f.empty() && "function empty");

    ReturnInst* back = &uniqueReturn(f);

    auto capstone = internaliseGlobals(m, f);
    auto unified = generateGlobalState(m, f);

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
            if (cap->getAllocatedType() != ty) {
                auto size = cap->getAllocatedType()->getPrimitiveSizeInBits().getFixedSize();
                Type* capIntTy = IntegerType::get(Context, size);
                for (auto* use : cap->users()) {
                    if (auto* load = dyn_cast<LoadInst>(use)) {
                        IRBuilder irb{load};
                        auto* load2 = irb.CreateLoad(capIntTy, glo);
                        auto* cast = irb.CreateBitCast(load2, cap->getAllocatedType());
                        load->replaceAllUsesWith(cast);
                    } else if (auto* stor = dyn_cast<StoreInst>(use)) {
                        IRBuilder irb{stor};
                        auto* cast = irb.CreateBitCast(stor->getValueOperand(), capIntTy);
                        auto* stor2 = irb.CreateStore(cast, glo);
                        stor->replaceAllUsesWith(stor2);
                    } else {
                        errs() << *use << '\n';
                        assert(false && "unsupported use of capstone alias register");
                    }

                }
            } else {
                cap->replaceAllUsesWith(glo);
            }
        } else if (!cap->hasName() && cap->getNumUses() <= 1) {
            // eliminate: store volatile i64 0, i64* @0
            for (auto* use : clone_it(cap->users())) {
                if (auto* inst = dyn_cast<Instruction>(use)) {
                    assert(inst->isSafeToRemove());
                    inst->eraseFromParent();
                }
            }
            assert(cap->getNumUses() == 0);
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

    correctGlobalAccesses(unified);
    correctMemoryAccesses(m, f);
}
