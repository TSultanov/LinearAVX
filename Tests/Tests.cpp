#include "Harness.h"
#include "TestCompiler.h"

#include <cstdio>

#include "../Instructions/Metadata.h"
#include "../Instructions/VSUBPS.h"

int main() {
    TestCompiler compiler(VSUBPS::Metadata);
    auto thunks = compiler.getThunks();
    for (auto const& thunk : thunks) {
        Harness harness(thunk);
        auto result = harness.runTests();
    }
}