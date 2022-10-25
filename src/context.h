#pragma once 

#include "llvm/IR/LLVMContext.h"

#define assertm(exp, msg) assert(((void)msg, exp))


extern llvm::LLVMContext Context;
