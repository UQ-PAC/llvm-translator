#include "context.h"
#include "state.h"
#include "translate.h"

#include <vector>

void asl(Module& m) {
  std::vector<GlobalVariable*> globals;
  for (auto& glo : m.globals())
    globals.push_back(&glo);

  correctGlobalAccesses(globals);
  Function* root = findFunction(m, "root");
  assert(root && "failed to find root function in asl");
  correctMemoryAccesses(m, *root);
}
