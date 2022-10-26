#pragma once 

#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

/**
 * Unified state representation: 
 * x0..x31 : bits(64) (alias w)
 * v0..v31 : bits(128) (alias q d s h b)
 * nf, zf, cf, vf : bits(1)
 * pc
 */

std::vector<GlobalVariable*> generateGlobalState(Module& m);

BasicBlock& newEntryBlock(Function& f);
std::vector<AllocaInst*> internaliseGlobals(Module& module, Function& f);
std::vector<AllocaInst*> internaliseParams(Function& f);
ReturnInst& uniqueReturn(Function&);

enum StateType {
    X, V, STATUS, PC
};

struct StateReg {
    StateType type;
    int num;

    std::string name() const;
    size_t size() const;
    Type* ty() const;
};
