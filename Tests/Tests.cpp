#include "Harness.h"
#include "TestCompiler.h"

#include <cstdio>

#include "../Instructions/Metadata.h"
#include "../Instructions/VSUBPS.h"
#include "xed/xed-iform-enum.h"

int main() {
    xed_tables_init();
    TestCompiler compiler(VSUBPS::Metadata);
    auto thunks = compiler.getThunks();
    for (auto const& thunk : thunks) {
        printf("Test %s\n", xed_iform_enum_t2str(thunk.iform));
        Harness harness(thunk);
        TestResult result = harness.runTests();
        result.printResult();
    }
}