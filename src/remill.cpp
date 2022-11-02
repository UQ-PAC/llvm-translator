#include "context.h"
#include "state.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Attributes.h>

#include <optional>
#include <string>
#include <span>

void translateStateAccess(Module& m, GetElementPtrInst& gep) {
  std::vector<Value*> vals{gep.idx_begin(), gep.idx_end()};
  std::vector<int> indices;
  for (auto& v : vals) {
    ConstantInt* c = cast<ConstantInt>(v);
    indices.push_back(c->getValue().getSExtValue());
  }

  auto prefix = [&](std::initializer_list<int> idx) { 
    assert(idx.size() <= indices.size());
    for (int i = 0; i < idx.size(); i++) {
      if (idx.begin()[i] != indices[i]) {
        return false;
      }
    }
    return true;
  };

  if (prefix({1, 2})) {
  }




}


void translate(Module& m, Function& f) {
  assert(f.arg_size() == 3 && f.getArg(0)->getName() == "state");
  auto* state = f.getArg(0);
  auto* pc = f.getArg(1);
  auto* mem = f.getArg(2);
  
  for (auto* u : std::vector<User*> {state->user_begin(), state->user_end()}) {
    StoreInst* store; 

    if (auto* gep = dyn_cast<GetElementPtrInst>(u)) {
      outs() << "gep" << *u << '\n';
    } else if (auto* call = dyn_cast<CallInst>(u)) {
      outs() << "call" << *u << '\n';
    } else if ((store = dyn_cast<StoreInst>(u)) && store->getValueOperand() == state) {
      // remill inserts a superfluous use of %state into %STATE:
      // %STATE = alloca ptr, align 8
      // store ptr %state, ptr %STATE, align 8

      // get alloc of %STATE
      auto* alloc = cast<AllocaInst>(store->getPointerOperand());
      
      assert(store->isSafeToRemove());
      store->eraseFromParent();

      assert(alloc->isSafeToRemove());
      alloc->eraseFromParent();
    } else {
      errs() << *u << '\n';
      assert(false && "unsupported user of remill state");
    }
  }
}

void remill(Module& m) {
  std::vector<std::string> flag_funcs = {
    "__remill_flag_computation_sign",
    "__remill_flag_computation_zero",
    "__remill_flag_computation_overflow"
  };

  for (auto& nm : flag_funcs) {
    Function& f = findFunction(m, nm);
    ReturnInst::Create(Context, f.getArg(0), 
      BasicBlock::Create(Context, "", &f));
  }

  Function& missing_block = findFunction(m, "__remill_missing_block");
  ReturnInst::Create(Context, UndefValue::get(PointerType::get(Context, 0)), 
    BasicBlock::Create(Context, "", &missing_block));

  for (auto& f : m.functions()) {
    f.removeFnAttr(Attribute::AttrKind::OptimizeNone);
    f.removeFnAttr(Attribute::AttrKind::NoInline);
    f.addFnAttr(Attribute::get(Context, Attribute::AttrKind::AlwaysInline));
  }
  Function& root = *m.begin();
  translate(m, root);
}
