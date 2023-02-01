#include "state.h"
#include "context.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include <map>

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

AllocaInst* findLocalVariable(Function& f, std::string const& name) {
    for (auto& bb : f) {
        for (auto& inst : bb) {
            if (auto* alloc = dyn_cast<AllocaInst>(&inst)) {
                if (alloc->getName() == name)
                    return alloc;
            }
        }
    }
    return nullptr; 
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
    regs.push_back({SP});

    std::vector<GlobalVariable*> globals{};
    for (auto& reg : regs) {
        globals.push_back(variable(m, reg.size(), reg.name()));
    }

    return globals;
}

void noundef(LoadInst* load) {
    assert(load);
    load->setMetadata("noundef", MDTuple::get(Context, {}));
}

void correctGetElementPtr(GlobalVariable* glo, User* gep, int offset) {
    Type* gloTy = glo->getValueType();
    
    for (User* u2 : clone_it(gep->users())) {
        if (auto* load = dyn_cast<LoadInst>(u2)) {
            IRBuilder irb{load};
            auto* load2 = irb.CreateLoad(gloTy, glo, "");
            auto* shift = offset > 0 ? irb.CreateLShr(load2, offset) : load2;
            auto* trunc = irb.CreateTruncOrBitCast(shift, load->getType());
            load->replaceAllUsesWith(trunc);
            load->eraseFromParent();
        } else if (auto* store = dyn_cast<StoreInst>(u2)) {
            IRBuilder irb{store};

            auto* value = store->getValueOperand();
            auto* valueTy = value->getType();
            value = irb.CreateZExtOrBitCast(value, gloTy);
            value = offset > 0 ? irb.CreateShl(value, offset) : value;

            auto* load2 = irb.CreateLoad(gloTy, glo, "");
            auto focus = APInt::getZero(valueTy->getIntegerBitWidth());
            auto mask = APInt::getAllOnes(gloTy->getIntegerBitWidth());
            mask.insertBits(focus, offset);

            auto* andd = irb.CreateAnd(load2, ConstantInt::get(gloTy, mask));
            auto* orr = irb.CreateOr(andd, value);
            store->replaceAllUsesWith(irb.CreateStore(orr, glo));
            store->eraseFromParent();
        } else {
            errs() << *u2 << '\n';
            assert(false && "unsupported use of getelementptr of global register");
        }
    }
    assert(gep->getNumUses() == 0 && "uses of gep not fully eliminated");
}


void correctGlobalAccesses(const std::vector<GlobalVariable*>& globals) {
    for (auto* glo : globals) {
        auto* gloTy = glo->getValueType();
        auto gloWd = gloTy->getIntegerBitWidth();
        for (User* u : clone_it(glo->users())) {
            if (auto* load = dyn_cast<LoadInst>(u)) {
                auto* valTy = load->getType(); 
                unsigned valWd = valTy->getPrimitiveSizeInBits(); 


                if (valWd == gloWd) {
                    // width matches
                } else if (valWd < gloWd) {
                    auto* next = load->getNextNode();
                    auto* load2 = new LoadInst(gloTy, glo, "", next);
                    auto* trunc = new TruncInst(load2, valTy, "", next);
                    load->replaceAllUsesWith(trunc);
                    load = load2;
                } else {
                    auto* load2 = new LoadInst(gloTy, glo, "", load);
                    auto* ext = new ZExtInst(load2, load->getType(), "", load);
                    load->replaceAllUsesWith(ext);
                    load = load2;
                    // errs() << *load << " <- " << *glo << '\n';
                    // assert(valWd == gloWd && "attempt to load a value from a smaller register");
                }

                if (!load->getName().startswith("_")) {
                    load->setName(glo->getName());
                }
                noundef(load);
            } else if (auto* store = dyn_cast<StoreInst>(u)) {
                auto* val = store->getValueOperand();
                auto* valTy = val->getType();
                unsigned valWd = valTy->getPrimitiveSizeInBits();

                if (store->getAlign().value() > 8) {
                    store->setAlignment(llvm::Align(8));
                }

                if (valWd == gloWd) {
                    // widths match
                } else if (valWd > gloWd) {
                    store->setOperand(0, new TruncInst(val, gloTy, "", store));
                } else if (valWd < gloWd) {
                    store->setOperand(0, new ZExtInst(val, gloTy, "", store));
                }
            } else if (auto* gep = dyn_cast<GetElementPtrInst>(u)) {
                assert(gep->getNumIndices() == 1 && "too many indices for global register getelementptr");
                int wd = gep->getResultElementType()->getPrimitiveSizeInBits();
                int index = cast<ConstantInt>(gep->idx_begin())->getSExtValue();
                correctGetElementPtr(glo, gep, wd*index);
            } else if (auto* gep2 = dyn_cast<GEPOperator>(u)) {
                assert(gep2->getNumIndices() == 1 && "too many indices for global register getelementptr");
                int wd = gep2->getResultElementType()->getPrimitiveSizeInBits();
                int index = cast<ConstantInt>(gep2->idx_begin())->getSExtValue();
                correctGetElementPtr(glo, gep2, wd*index);
            } else if (auto* phi = dyn_cast<PHINode>(u)) {
                // ignore for now
            } else {
                errs() << *u << '\n';
                assert(false && "unsupported use of unified global variable");
            }
        }
    }
}

void correctMemoryAccesses(Module& m, Function& root) {
  std::initializer_list<int> sizes = { 8, 16, 32, 64 };

  std::map<int, Function*> loads;
  std::map<int, Function*> stores;

  for (int sz : sizes) {
    std::string loadName = "load_" + std::to_string(sz);
    std::string storeName = "store_" + std::to_string(sz);

    Type* i64 = IntegerType::get(Context, 64);
    Type* valTy = IntegerType::get(Context, sz);
    FunctionType* loadTy = FunctionType::get(valTy, {i64}, false);
    FunctionType* storeTy = FunctionType::get(Type::getVoidTy(Context), {i64, valTy}, false);

    Function* load = Function::Create(loadTy,
      GlobalValue::LinkageTypes::ExternalLinkage, 0, loadName, &m);
    Function* store = Function::Create(storeTy,
      GlobalValue::LinkageTypes::ExternalLinkage, 0, storeName, &m);

    for (Function* fn : {load, store}) {
      using enum Attribute::AttrKind;
      auto attr = [](Attribute::AttrKind kind) {
        return Attribute::get(Context, kind);
      };

      fn->addParamAttr(0, attr(NoUndef));
      if (!fn->getReturnType()->isVoidTy()) {
        // fn->addRetAttr(attr(NoUndef));
      }
      fn->addFnAttr(attr(InaccessibleMemOnly));
      if (fn == load) {
        fn->addFnAttr(attr(ReadOnly));
      }
      fn->addFnAttr(attr(WillReturn));
      fn->addFnAttr(attr(NoUnwind));
    }

    loads[sz] = load;
    stores[sz] = store;
  }

  for (BasicBlock& bb : root) for (Instruction& inst : bb) {
    IntToPtrInst* i2p = dyn_cast<IntToPtrInst>(&inst);
    if (!i2p) continue; 
    for (User* u : clone_it(i2p->users())) {
      auto* addr = i2p->getOperand(0);
      if (auto* load = dyn_cast<LoadInst>(u)) {
        
        int sz = load->getType()->getIntegerBitWidth();
        CallInst* call = CallInst::Create(loads[sz]->getFunctionType(), loads.at(sz), {addr}, "", load);
        load->replaceAllUsesWith(call);
        load->eraseFromParent();

      } else if (auto* stor = dyn_cast<StoreInst>(u)) {
        auto* val = stor->getValueOperand();
        int sz = stor->getValueOperand()->getType()->getIntegerBitWidth();
        CallInst* call = CallInst::Create(stores[sz]->getFunctionType(), stores.at(sz), {addr, val}, "", stor);
        stor->replaceAllUsesWith(call);
        stor->eraseFromParent();

      } else {
        errs() << *u << '\n';
        assert(0 && "unsupported use of int2ptr cast");
      }
    }

    assert(i2p->isSafeToRemove() && "unable to remove i2p instruction");
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

std::vector<std::reference_wrapper<ReturnInst>> functionReturns(Function& f) {
    std::vector<std::reference_wrapper<ReturnInst>> rets{};
    for (auto& b : f) {
        for (auto& i : b) {
            if (auto* r = dyn_cast<ReturnInst>(&i)) {
                rets.push_back(*r);
            }
        }
    }

    return rets;
}

ReturnInst& uniqueReturn(Function& f) {
    auto rets = functionReturns(f);
    assert(rets.size() > 0 && "no returns in function");
    assert(rets.size() == 1 && "multiple returns in function");
    return rets[0];
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
    } else if (t == SP) {
        prefix = "SP";
    } else {
        llvm_unreachable("name()");
    }

    prefix.append(suffix);
    return prefix;
}

size_t StateReg::size() const {
    auto t = this->type;
    if (t == V) {
        return 128;
    } else if (t == STATUS) {
        return 1;
    } else if (t == PC || t == SP || t == X) {
        return 64;
    } else {
        llvm_unreachable("size()");
    }
}

Type* StateReg::ty() const {
    return Type::getIntNTy(Context, this->size());
}
