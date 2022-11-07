#include "context.h"
#include "state.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Attributes.h>

#include <optional>
#include <string>
#include <span>
#include <algorithm>

StateReg translateStateAccess(Module& m, GetElementPtrInst& gep) {
  std::vector<int> indices;
  for (auto& v : gep.indices()) {
    ConstantInt* c = cast<ConstantInt>(v);
    indices.push_back(c->getValue().getSExtValue());
  }

  auto matches = [&](std::initializer_list<int> prefix = {}, std::initializer_list<int> suffix = {}) { 
    assert(prefix.size() <= indices.size());
    assert(suffix.size() <= indices.size());

    for (ulong i = 0; i < prefix.size(); i++) {
      if (prefix.begin()[i] != indices[i])
        return false;
    }

    for (ulong i = 0; i < suffix.size(); i++) {
      if (suffix.end()[-i-1] != indices.end()[-i-1])
        return false;
    }

    return true;
  };

  int len = indices.size();
  if (len == 6 && matches({0,0,3}, {0,0})) {

    int k = indices.at(3);
    if (k == 65)
      return StateReg{PC};
    else if (k == 63)
      assert(false && "SP");
    else if (0 <= k && k <= 61 && k % 2 == 1)
      return StateReg{X, k/2};

  } else if (len == 4 && matches({0,0,9}, {})) {

    int k = indices.at(3);
    if (5 <= k && k <= 11 && k % 2 == 1) {
      int i = (k-5) / 2;
      char flags[] = {'N','Z','C','V'};
      return StateReg{STATUS, flags[i]};
    }

  } else if (len == 8 && matches({0,0,1,0}, {0,0,0})) {
    int k = indices.at(4);
    return StateReg{V, k};
  }

  errs() << "failed: " << gep << '\n';
  assert(false && "unhandled state getelementptr");
}


void replaceRemillStateAccess(Module& m, Function& f) {
  assert(f.arg_size() == 3 && f.getArg(0)->getName() == "state");

  auto& entry = f.getEntryBlock();
  auto* state = f.getArg(0);
  auto* pc = f.getArg(1);
  auto* mem = f.getArg(2);
  
  for (auto* u : clone_it(state->users())) {
    StoreInst* store; 
    CallInst* call;

    if (auto* gep = dyn_cast<GetElementPtrInst>(u)) {
      StateReg reg = translateStateAccess(m, *gep);
      GlobalVariable* glo = m.getNamedGlobal(reg.name());
      gep->replaceAllUsesWith(glo);

      assert(gep->isSafeToRemove());
      gep->eraseFromParent();

    // } else if ((call = dyn_cast<CallInst>(u)) && call->getFunction()->getName() == "__remill_missing_block") {
    } else if ((store = dyn_cast<StoreInst>(u)) && store->getValueOperand() == state) {
      // remill inserts a superfluous store of %state into %STATE:
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

  auto* globalpc = m.getNamedGlobal(StateReg{PC}.name());
  // remill has a %program_counter argument which is just the initial program counter.
  auto* pcload = new LoadInst(
    pc->getType(), globalpc, "program_counter", &*entry.getFirstInsertionPt());
  pc->replaceAllUsesWith(pcload);
  assert(pc->getNumUses() == 0);

  mem->replaceAllUsesWith(UndefValue::get(mem->getType()));

  // for (auto* u : clone_it(mem->users())) {
  //   if (auto* inst = dyn_cast<Instruction>(u)) {
  //     inst->eraseFromParent();
  //   }
  // }

  // for (Instruction& inst : f.getEntryBlock()) {
  //   AllocaInst* alloc;
  //   if ((alloc = dyn_cast<AllocaInst>(&inst)) && inst.getName() == "MEMORY") {

  //     for (User* u : clone_it(alloc->users())) { 
  //       cast<Instruction>(u)->eraseFromParent();
  //     }

  //   }

  // }

}

void replaceRemillTailCall(Module& m, Function& f) {
  Function* missing_block = findFunction(m, "__remill_missing_block");
  assert(missing_block);

  // ReturnInst::Create(Context, UndefValue::get(PointerType::get(Context, 0)), 
  //   BasicBlock::Create(Context, "", &missing_block));
  assert(missing_block->getNumUses() == 1);

  auto* call = cast<CallInst>(*missing_block->user_begin());
  assert(call->getNumUses() == 1);
  auto* ret = cast<ReturnInst>(*call->user_begin());

  auto* retblock = ret->getParent();
  assert(retblock->getParent() == &f);

  ret->eraseFromParent();
  call->eraseFromParent();

  ReturnInst::Create(Context, retblock);
}

Function* replaceRemillFunctionSignature(Module& m, Function& f) {
  std::vector<BasicBlock*> blocks;
  for (auto& bb : f) {
    blocks.push_back(&bb);
  }

  Function* f2 = Function::Create(
    FunctionType::get(Type::getVoidTy(Context), false),
    f.getLinkage(), entry_function_name, m
  );

  for (auto* bb : blocks) {
    bb->removeFromParent();
    bb->insertInto(f2);
  }

  // f.eraseFromParent();

  return f2;
}

void remill(Module& m) {
  m.setTargetTriple("");

  std::vector<std::string> flag_funcs = {
    "__remill_flag_computation_sign",
    "__remill_flag_computation_zero",
    "__remill_flag_computation_overflow"
  };

  for (auto& nm : flag_funcs) {
    Function* f = findFunction(m, nm);
    if (!f) continue;
    for (auto* u : clone_it(f->users())) {
      // errs() << "USER: " << *u << '\n';
      if (auto* call = dyn_cast<CallInst>(u)) {
        call->replaceAllUsesWith(call->getArgOperand(0));
        call->eraseFromParent();
      }
    }
  }


  for (auto& f : m.functions()) {
    f.removeFnAttr(Attribute::AttrKind::OptimizeNone);
    f.removeFnAttr(Attribute::AttrKind::NoInline);
    f.addFnAttr(Attribute::get(Context, Attribute::AttrKind::AlwaysInline));
  }

  Function* root = &*m.begin();
  
  generateGlobalState(m);

  replaceRemillTailCall(m, *root);
  replaceRemillStateAccess(m, *root);

  root = replaceRemillFunctionSignature(m, *root);
}
