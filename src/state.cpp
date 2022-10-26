#include "state.h"
#include "context.h"

using namespace llvm;

static constexpr int XS_COUNT = 32;
static constexpr int XS_SIZE = 64;
static constexpr int VS_COUNT = 32;
static constexpr int VS_SIZE = 128;

GlobalVariable* variable(Module& m, int size, const std::string nm) {
    IntegerType* ty = Type::getIntNTy(Context, size);

    Constant* val = ConstantInt::get(ty, 0);

    return new GlobalVariable(m, 
        ty, false, GlobalValue::LinkageTypes::InternalLinkage,
        val, nm);
}

std::vector<GlobalVariable*> generateGlobalState(Module& m) {

    std::vector<StateReg> regs{};

    for (int i = 0; i < XS_COUNT; i++) {
        regs.emplace_back(X, i);
    }
    for (int i = 0; i < VS_COUNT; i++) {
        regs.emplace_back(V, i);
    }

    regs.emplace_back(STATUS, 'N');
    regs.emplace_back(STATUS, 'Z');
    regs.emplace_back(STATUS, 'C');
    regs.emplace_back(STATUS, 'V');

    regs.emplace_back(PC);

    std::vector<GlobalVariable*> globals{};
    for (auto& reg : regs) {
        globals.push_back(variable(m, reg.size(), reg.name()));
    }

    return globals;
}

BasicBlock& newEntryBlock(Function& f) {
    assertm(!f.empty(), "analysed function must not be empty");
    BasicBlock* oldEntry = f.empty() ? nullptr : &f.getEntryBlock();
    BasicBlock* newEntry = BasicBlock::Create(Context, "new_entry", &f, oldEntry);
    if (oldEntry) {
        BranchInst::Create(oldEntry, newEntry);
    }
    return f.getEntryBlock();
}

std::vector<AllocaInst*> internaliseGlobals(Module& module, Function& f) {

    Instruction& insertion = *f.getEntryBlock().getFirstInsertionPt();

    std::vector<AllocaInst*> allocs{};

    for (auto& g : module.getGlobalList()) {

        auto alloc = new AllocaInst(
            g.getInitializer()->getType(), /*addrspace*/0, /*arraysize*/nullptr,
            "" + g.getName(), &insertion);
        allocs.push_back(alloc);

        g.replaceAllUsesWith(alloc);
    }

    return allocs;
}

std::vector<AllocaInst*> internaliseParams(Function& f) {
    Instruction& insertion = *f.getEntryBlock().getFirstInsertionPt();

    std::vector<AllocaInst*> allocs{};

    for (auto& a : f.args()) {

        auto alloc = new AllocaInst(
            a.getType(), /*addrspace*/0, /*arraysize*/nullptr,
            "" + a.getName(), &insertion);

        allocs.push_back(alloc);
        a.replaceAllUsesWith(alloc);
    }

    return allocs;
}

ReturnInst& uniqueReturn(Function& f) {
    // assumes there is exactly one return 
    ReturnInst* ret = nullptr;
    for (auto& b : f) {
        for (auto& i : b) {
            if (auto* r = dyn_cast<ReturnInst>(&i)) {
                assert(ret == nullptr && "multiple returns in function");
                ret = r;
            }
        }
    }
    assert(ret != nullptr && "no returns in function");
    return *ret;
}

std::string StateReg::name() const {
    std::string prefix;
    std::string suffix;
    auto t = this->type;
    int n = this->num;

    if (t == X) {
        prefix = "X";
        suffix = std::to_string(n);
    } else if (t == V) {
        prefix = "V";
        suffix = std::to_string(n);
    } else if (t == STATUS) {
        suffix = "F";
        prefix = {(char)n};
    } else if (t == PC) {
        prefix = "PC";
    } else {
        llvm_unreachable("name()");
    }

    prefix.append(suffix);
    return prefix;
}

size_t StateReg::size() const {
    auto t = this->type;
    if (t == X) {
        return 64;
    } else if (t == V) {
        return 128;
    } else if (t == STATUS) {
        return 1;
    } else if (t == PC) {
        return 64;
    } else {
        llvm_unreachable("size()");
    }
}

Type* StateReg::ty() const {
    return Type::getIntNTy(Context, this->size());
}
