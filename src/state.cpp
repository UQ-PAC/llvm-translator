#include "state.h"
#include "context.h"

using namespace llvm;

const std::string entry_function_name = "root";

static constexpr int XS_COUNT = 32;
static constexpr int XS_SIZE = 64;
static constexpr int VS_COUNT = 32;
static constexpr int VS_SIZE = 128;

Function* findFunction(Module& m, std::string const& name) {
    auto it = std::find_if(m.begin(), m.end(), 
        [&name](Function& f) { return f.getName() == name; });
    // assert((it != m.end() || (errs() << name, 0)) 
    //     && "unable to find function matching name");
    return it != m.end() ? &*it : nullptr;
}

GlobalVariable* variable(Module& m, int size, const std::string nm) {
    IntegerType* ty = Type::getIntNTy(Context, size);

    Constant* val = ConstantInt::get(ty, 0);

    return new GlobalVariable(m, 
        ty, false, GlobalValue::LinkageTypes::ExternalLinkage,
        val, nm);
}

std::vector<GlobalVariable*> generateGlobalState(Module& m, Function& f) {

    std::vector<StateReg> regs{};

    for (int i = 0; i < XS_COUNT; i++) {
        regs.push_back({X, i});
    }
    for (int i = 0; i < VS_COUNT; i++) {
        regs.push_back({V, i});
    }

    regs.push_back({STATUS, 'N'});
    regs.push_back({STATUS, 'Z'});
    regs.push_back({STATUS, 'C'});
    regs.push_back({STATUS, 'V'});

    regs.push_back({PC});

    std::vector<GlobalVariable*> globals{};
    for (auto& reg : regs) {
        globals.push_back(variable(m, reg.size(), reg.name()));
    }

    return globals;
}

void assumeGlobalsWellDefined(std::vector<GlobalVariable*>& globals) {
    for (auto* glo : globals) {
        auto* gloTy = glo->getValueType();
        auto gloWd = gloTy->getIntegerBitWidth();
        for (User* u : glo->users()) {
            if (auto* load = dyn_cast<LoadInst>(u)) {
                auto* valTy = load->getType(); 
                unsigned valWd = valTy->getIntegerBitWidth(); 


                if (valWd == gloWd) {
                    // width matches
                } else if (valWd < gloWd) {
                    auto* next = load->getNextNode();
                    auto* load2 = new LoadInst(gloTy, glo, "", next);
                    auto* trunc = new TruncInst(load2, valTy, "", next);
                    load->replaceAllUsesWith(trunc);
                    load = load2;
                } else {
                    assert(valWd == gloWd && "attempt to load a value from a smaller register");
                }

                load->setMetadata("noundef", MDTuple::get(Context, {}));
            } else if (auto* store = dyn_cast<StoreInst>(u)) {
                auto* val = store->getValueOperand();
                auto* valTy = val->getType();
                unsigned valWd = valTy->getIntegerBitWidth();

                if (valWd == gloWd) {
                    // widths match
                } else if (valWd > gloWd) {
                    store->setOperand(0, new TruncInst(val, gloTy, "", store));
                } else {
                    assert(valWd == gloWd && "attempt to store a value in larger global register");
                }
            } else {
                errs() << *u << '\n';
                assert(0 && "unsupported use of unified global variable");
            }
        }
    }
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

    module.getGlobalList().clear();

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
    auto d = this->data;

    if (t == X) {
        prefix = "X";
        suffix = std::to_string(d.num);
    } else if (t == V) {
        prefix = "V";
        suffix = std::to_string(d.num);
    } else if (t == STATUS) {
        suffix = "F";
        prefix = {d.flag};
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
