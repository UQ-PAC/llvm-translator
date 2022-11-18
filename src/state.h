#pragma once 

#include <variant>

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

extern const std::string entry_function_name; 

Function* findFunction(Module& m, std::string const& name);
std::vector<GlobalVariable*> generateGlobalState(Module& m, Function& f);
void correctGlobalAccesses(std::vector<GlobalVariable*>& globals);
void correctMemoryAccesses(Module& m, Function& root);
void noundef(LoadInst*);

BasicBlock& newEntryBlock(Function& f);
std::vector<AllocaInst*> internaliseGlobals(Module& module, Function& f);
std::vector<AllocaInst*> internaliseParams(Function& f);
ReturnInst& uniqueReturn(Function&);

enum StateType {
    X, // data is register num, as unsigned
    V, // data is register num, as unsigned
    STATUS, // data is uppercase ascii character, as char
    PC // data unused
};

struct StateReg {
    StateType type;
    union {
        int num;
        char flag;
    } data;

    std::string name() const;
    size_t size() const;
    Type* ty() const;
};


template<std::ranges::range T>
std::vector<std::ranges::range_value_t<T>> clone_it(T&& range) {

    using namespace std::ranges;
    using V = range_value_t<T>;
    using I = iterator_t<T>;

    I itbegin = begin(range);
    I itend = end(range);

    std::vector<V> vec{};
    for (I i = itbegin; i != itend; i++) {
        vec.push_back(*i);
    }
    return vec;
}
