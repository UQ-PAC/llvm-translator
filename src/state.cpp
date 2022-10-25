/**
 * Unified state representation: 
 * x0..x31 : bits(64) (alias w)
 * v0..v31 : bits(128) (alias q d s h b)
 * nf, zf, cf, vf : bits(1)
 * pc
 */

#include "state.h"
#include "context.h"

using namespace llvm;

static constexpr int XS_COUNT = 32;
static constexpr int XS_SIZE = 64;
static constexpr int VS_COUNT = 32;
static constexpr int VS_SIZE = 128;


void generateGlobalState(Module& m) {
    auto make = [&](int size, std::string prefix, int n) {
        std::string nm{prefix};
        nm.append(std::to_string(n));
        Type* ty = Type::getIntNTy(Context, size);

        return new GlobalVariable(m, 
            ty, false, GlobalValue::LinkageTypes::InternalLinkage,
            nullptr, nm);
    };

    for (int i = 0; i < XS_COUNT; i++) {
        make(XS_SIZE, "x", i);
    }
    for (int i = 0; i < VS_COUNT; i++) {
        make(VS_SIZE, "v", i);
    }
}
